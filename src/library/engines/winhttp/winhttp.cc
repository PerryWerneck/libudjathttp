/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2023 Perry Werneck <perry.werneck@gmail.com>
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

 /**
  * @brief Brief description of this source.
  */

 #include <config.h>
 #include <winsock2.h>
 #include <winsock.h>

 #include <udjat/defs.h>
 #include <private/engine.h>
 #include <udjat/tools/logger.h>
 #include <udjat/win32/exception.h>
 #include <udjat/tools/configuration.h>
 #include <sstream>

 using namespace std;

 namespace Udjat {

	static HINTERNET open() {

		// Open HTTP session

		// https://docs.microsoft.com/en-us/windows/desktop/api/winhttp/nf-winhttp-winhttpopenrequest
		static const char * userAgent = STRINGIZE_VALUE_OF(PRODUCT_NAME) "/" PACKAGE_VERSION " (Windows)";
		wchar_t wUserAgent[256];
		mbstowcs(wUserAgent, userAgent, strlen(userAgent)+1);

		return WinHttpOpen(
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

	}

	HTTP::Engine::Engine(HTTP::Worker &w, const HTTP::Method m, time_t timeout) : session{open()}, method{m}, pwszUrl{new wchar_t[w.url().size()*3]}, worker{w} {

		if(!timeout) {
			timeout = 60;
		}

		if(!WinHttpSetTimeouts(
				(HINTERNET) session,
				Config::Value<int>("http","ResolveTimeout",timeout).get(),
				Config::Value<int>("http","ConnectTimeout",timeout).get(),
				Config::Value<int>("http","SendTimeout",timeout).get(),
				Config::Value<int>("http","ReceiveTimeout",timeout).get()
			)) {

			throw Win32::Exception("Error setting HTTP session timeouts");
		}

		mbstowcs(pwszUrl, w.url().c_str(), w.url().size()+1);

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

	HTTP::Engine::~Engine() {
		delete[](pwszUrl);
	}

	/*
	static HINTERNET connect(HINTERNET session, URL_COMPONENTS &urlComp) {

		debug("Connecting");

		HINTERNET connection =
			WinHttpConnect(
				session,
				wstring{urlComp.lpszHostName, urlComp.dwHostNameLength}.c_str(),
				urlComp.nPort,
				0
			);

		if(!connection) {
			throw Win32::Exception("Can't connect to host");
		}

		return connection;
	}
	*/

	static wstring get_verb(const HTTP::Method id) {

		static const struct {
			HTTP::Method id;
			const char *verb;
		} methods[] = {
			{ HTTP::Get,	"GET"		},
			{ HTTP::Head,	"HEAD"		},
			{ HTTP::Post,	"POST"		},
			{ HTTP::Put,	"PUT"		},
			{ HTTP::Delete,	"DELETE"	}
		};

		for(auto &method : methods) {
			if(method.id == id) {
				wchar_t buffer[100];
				mbstowcs(buffer, method.verb, strlen(method.verb)+1);
				return wstring{buffer};
			}
		}

		throw system_error(EINVAL,system_category(),"Invalid or unsupported http verb");

	}

	int HTTP::Engine::perform(bool except) {

		try {

			// Connect
			HTTP::Handle<HINTERNET> connection{
				WinHttpConnect(
					(HINTERNET) session,
					wstring{urlComp.lpszHostName, urlComp.dwHostNameLength}.c_str(),
					urlComp.nPort,
					0
				)
			};

			// Create request
			HTTP::Handle<HINTERNET> request{
				WinHttpOpenRequest(
					(HINTERNET) connection,
					get_verb(method).c_str(),
					urlComp.lpszUrlPath,
					NULL,
					WINHTTP_NO_REFERER,
					WINHTTP_DEFAULT_ACCEPT_TYPES,
					(urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
				)
			};

			WinHttpSetOption(
				(HINTERNET) request,
				WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
				WINHTTP_NO_CLIENT_CERT_CONTEXT,
				0
			);

			// Build headers.
			wstring headers;
			{
				auto &requests = worker.requests();

				if(requests.size()) {

					ostringstream hdr;

					for(auto &request : requests) {
						hdr << request.name() << ": " << request.value() << "\r\n";
					}

					auto text = hdr.str();
					wchar_t buffer[text.size()*3];
					mbstowcs(buffer, text.c_str(), text.size()+1);

					headers = buffer;

				}

			}

			// Get payload
			{
				const char * payload = worker.payload();
				size_t sz = strlen(payload);

				if(WinHttpSendRequest(
						request,
						(headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str()),
						headers.size(),
						(LPVOID) (sz ? payload : WINHTTP_NO_REQUEST_DATA),
						sz,
						sz,
						0
					)) {

					throw Win32::Exception();
				}
			}

			// wait

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("http");
			if(except) {
				throw;
			}
			return -1;

		} catch(...) {

			Logger::String{"Unexpected error"}.error("http");
			if(except) {
				throw;
			}
			return -1;

		}

		return 0;
	}

 }

