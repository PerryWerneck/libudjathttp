/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <internals.h>
#include <udjat/tools/url.h>
#include <udjat/tools/http/worker.h>
#include <udjat/tools/configuration.h>

namespace Udjat {

	void wchar_t_cleanup(wchar_t **buf) {
		if(*buf) {
			free(*buf);
			*buf = NULL;
		}
	}

	void hinternet_t_cleanup(HINTERNET *handle) {
		if(*handle) {
			WinHttpCloseHandle(*handle);
			*handle = 0;
		}
	}

	HTTP::Worker::Worker(const char *url, const HTTP::Method method, const char *payload) : Protocol::Worker(url,method,payload) {

		// Open HTTP session
		// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest
		static const char * userAgent = STRINGIZE_VALUE_OF(PRODUCT_NAME) "-winhttp/" PACKAGE_VERSION;
		wchar_t wUserAgent[256];
		mbstowcs(wUserAgent, userAgent, strlen(userAgent)+1);

		// https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpopen
		this->session =
			WinHttpOpen(
				wUserAgent,
#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
				WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
#else
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
#endif //
				WINHTTP_NO_PROXY_NAME,
				WINHTTP_NO_PROXY_BYPASS,
				0
			);

		if(!this->session) {
			throw Win32::Exception("Error opening HTTP session");
		}

		// Set timeouts.
		// https://docs.microsoft.com/en-us/windows/win32/api/winhttp/nf-winhttp-winhttpsettimeouts
		Config::Value<int> timeout("http","Timeout",6000);

		if(!WinHttpSetTimeouts(
				this->session,
				Config::Value<int>("http","ResolveTimeout",timeout.get()).get(),
				Config::Value<int>("http","ConnectTimeout",timeout.get()).get(),
				Config::Value<int>("http","SendTimeout",timeout.get()).get(),
				Config::Value<int>("http","ReceiveTimeout",timeout.get()).get()
			)) {

			throw Win32::Exception("Error setting HTTP session timeouts");
		}

	}

	HTTP::Worker::~Worker() {
		WinHttpCloseHandle(this->session);
	}

	HTTP::Worker & HTTP::Worker::credentials(const char *user, const char *passwd) {
		throw runtime_error("Not implemented");
		return *this;
	}

	static void CrackUrl(wchar_t *pwszUrl, URL_COMPONENTS &urlComp) {

		ZeroMemory(&urlComp, sizeof(urlComp));
		urlComp.dwStructSize 		= sizeof(urlComp);
		urlComp.dwSchemeLength    	= (DWORD)-1;
		urlComp.dwHostNameLength  	= (DWORD)-1;
		urlComp.dwUrlPathLength   	= (DWORD)-1;
		urlComp.dwExtraInfoLength 	= (DWORD)-1;

		if (!WinHttpCrackUrl(pwszUrl, (DWORD)wcslen(pwszUrl), 0, &urlComp)) {
			throw Udjat::Win32::Exception("Invalid URL on HTTP request");
		}
	}

	HINTERNET HTTP::Worker::connect() {

		URL_COMPONENTS urlComp;
		URL url = this->url();

		INTERNET_TEXT pwszUrl = (wchar_t *) malloc(url.size()*3);
		mbstowcs(pwszUrl, url.c_str(), url.size()+1);

		CrackUrl(pwszUrl, urlComp);
		wstring hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);

#ifdef DEBUG
		cout << "Connecting hostname='";
		wcout << hostname;
		cout << "' (" << urlComp.dwHostNameLength << ") port='" << urlComp.nPort << "'" << endl;
#endif // DEBUG

		HINTERNET connection =
			WinHttpConnect(
				this->session,
				hostname.c_str(),
				urlComp.nPort,
				0
			);

		if(!connection) {
			throw Win32::Exception(string{"Can't connect to "} + url);
		}

		return connection;

	}

	Udjat::String HTTP::Worker::wait(HINTERNET request, const std::function<bool(double current, double total)> &progress) {

#ifdef DEBUG
		cout << "Waiting for response" << endl;
#endif // DEBUG

		// Wait for response
		if(!WinHttpReceiveResponse(request, NULL)) {
			throw Win32::Exception(string{"Error receiving response from "} + url());
		}

#ifdef DEBUG
		cout << "Got response" << endl;
#endif // DEBUG

		// Get status code.
		{
			DWORD dwStatusCode = 0;
			DWORD dwSize = sizeof(DWORD);

			if(!WinHttpQueryHeaders(
					request,
					WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&dwStatusCode,
					&dwSize,
					WINHTTP_NO_HEADER_INDEX
				)) {
					throw Win32::Exception("Cant get HTTP status code");
				}

#ifdef DEBUG
			cout << "The status code was " << dwStatusCode << endl;
#endif // DEBUG

			if(dwStatusCode != 200) {

				wchar_t buffer[1024];
				dwSize = 1023;

				ZeroMemory(buffer, sizeof(buffer));

				WinHttpQueryHeaders(
					request,
					WINHTTP_QUERY_STATUS_TEXT,
					WINHTTP_HEADER_NAME_BY_INDEX,
                    buffer,
                    &dwSize,
					WINHTTP_NO_HEADER_INDEX
				);

				char text[1024];
				ZeroMemory(text, sizeof(text));
				wcstombs(text, buffer, 1023);

				throw HTTP::Exception((unsigned int) dwStatusCode, url().c_str(), text);
			}

		}

		//
		// Get file length
		//
		double total = 0;
		{
			DWORD fileLength = 0;
			DWORD size = sizeof(DWORD);
			if( WinHttpQueryHeaders( request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER , NULL, &fileLength, &size, NULL ) == TRUE ) {
				total = (double) fileLength;
			}
			progress(0,total);
		}

		stringstream response;
		char buffer[4096] = {0};
		DWORD length = 0;
		double current = 0;

		while(WinHttpReadData(request, buffer, sizeof(buffer), &length) && length > 0) {
			response.write(buffer,length);
			current += length;
			progress(current,total);
			length = 0;
		}

		return Udjat::String(response.str().c_str());

	}

	HINTERNET HTTP::Worker::open(HINTERNET connection) {

		URL_COMPONENTS urlComp;

		const char *verb = std::to_string(method());
		URL url = this->url();

		INTERNET_TEXT pwszVerb = (wchar_t *) malloc(strlen(verb)*3);
		mbstowcs(pwszVerb, verb, strlen(verb)+1);

		INTERNET_TEXT pwszUrl = (wchar_t *) malloc(url.size()*3);
		mbstowcs(pwszUrl, url.c_str(), url.size()+1);

		CrackUrl(pwszUrl, urlComp);

#ifdef DEBUG
		cout << "Requesting '";
		wcout << urlComp.lpszUrlPath;
		cout << "'" << endl;
#endif // DEBUG

		HINTERNET req =
			WinHttpOpenRequest(
				connection,
				pwszVerb,
				urlComp.lpszUrlPath,
				NULL,
				WINHTTP_NO_REFERER,
				WINHTTP_DEFAULT_ACCEPT_TYPES,
				(urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
			);

		if(!req) {
			throw Win32::Exception(string{"Can't create request for "} + url);
		}

		WinHttpSetOption(req, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);

		return req;

	}

	void HTTP::Worker::send(HINTERNET request) {

		INTERNET_TEXT	lpszHeaders = NULL;
		DWORD			dwHeadersLength = 0;

		if(!headers.empty()) {

			ostringstream headers;

			for(const Protocol::Header & header : this->headers) {
				headers << header.name() << ": " << header.value() << "\r\n";
			}

			auto text = headers.str();

			lpszHeaders = (wchar_t *) malloc(text.size()*3);
			dwHeadersLength = (DWORD) mbstowcs(lpszHeaders, text.c_str(), text.size()+1);

		}

		size_t sz = 0;
		const char *payload = this->out.payload.c_str();
		if(payload) {
			sz = strlen(payload);
			if(Config::Value<bool>("http","trace-payload",true).get()) {
				cout << "http\t" << method() << " " << url() << endl << payload << endl;
			}
		}

		if(!WinHttpSendRequest(
				request,
				(lpszHeaders ? lpszHeaders : WINHTTP_NO_ADDITIONAL_HEADERS), dwHeadersLength,
				(LPVOID) (payload ? payload : WINHTTP_NO_REQUEST_DATA),
				sz,
				sz,
				0)
			) {
			throw Win32::Exception(string{"Can't send request "} + url());
		}

		if(lpszHeaders) {
			free(lpszHeaders);
		}
	}

	String HTTP::Worker::get(const std::function<bool(double current, double total)> &progress) {

		progress(0,0);

		INTERNET_HANDLE	connection = connect();
		INTERNET_HANDLE	request = open(connection);

		send(request);

		return this->wait(request,progress);
	}

}
