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

 #include <internals.h>
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>

 namespace Udjat {
 #ifdef HAVE_CURL

	#ifdef DEBUG
		#define TRACE_DEFAULT true
	#else
		#define TRACE_DEFAULT false
	#endif  // DEBUG

	/*
	class CurlException : HTTP::Exception {
	public:
		CurlException(CURLcode code, const string &url) : HTTP::Exception(url.c_str(),curl_easy_strerror(code)) {
		}
	};
	*/

	HTTP::Client::Worker * HTTP::Client::Worker::getInstance(HTTP::Client *client) {
		return new HTTP::Client::Worker(client);
	}

	HTTP::Client::Worker::Worker(HTTP::Client *c) : client(c) {

		memset(error,0,CURL_ERROR_SIZE);

		hCurl = curl_easy_init();

#ifdef DEBUG
		cout << "URL= '" << client->url << "'" << endl;
#endif  // DEBUG

		curl_easy_setopt(hCurl, CURLOPT_URL, client->url.c_str());
		curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

		curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, error);

		curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, (void *) this);
		curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, header_callback);

		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

		// https://curl.se/libcurl/c/CURLOPT_FORBID_REUSE.html
		curl_easy_setopt(hCurl, CURLOPT_FORBID_REUSE, 1L);

		// Set timeout
		{
			int timeout = Config::Value<int>("http","timeout",5).get();
			if(timeout) {
				curl_easy_setopt(hCurl, CURLOPT_TIMEOUT, timeout);
			}
		}

		if(!client->credentials.username.empty()) {
			// Set credentials.
			curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
			curl_easy_setopt(hCurl, CURLOPT_USERNAME, client->credentials.username.c_str());
			curl_easy_setopt(hCurl, CURLOPT_PASSWORD, client->credentials.password.c_str());
		}

	}

	HTTP::Client::Worker::~Worker() {

		curl_easy_cleanup(hCurl);

	}

	std::string HTTP::Client::Worker::perform() {

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

		CURLcode res = curl_easy_perform(hCurl);

		if(res != CURLE_OK) {
#ifdef DEBUG
			cout << "CURL-Error=" << res << endl;
#endif // DEBUG
			throw HTTP::Exception(this->client->url.c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		if(response_code != 200) {
			cerr << "http\t" << this->client->url << " " << message << endl;
			if(message.empty()) {
				throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str());
			} else {
				throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str(), message.c_str());
			}
		}

		return buffers.in.str();

	}

	std::string HTTP::Client::Worker::call(const char *verb, const char *payload) {

		if(!strcasecmp(verb,"post")) {
			curl_easy_setopt(hCurl, CURLOPT_POST, 1);
		} else if(!strcasecmp(verb,"put")) {
			curl_easy_setopt(hCurl, CURLOPT_PUT, 1);
		} else if(!strcasecmp(verb,"delete")) {
			curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
		} else if(strcasecmp(verb,"get")) {
			throw system_error(EINVAL,system_category(),string{"Invalid or unsupported http verb: '"} + verb + "'");
		}

		if(Config::Value<bool>("http","trace",TRACE_DEFAULT).get()) {
			curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, this);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, trace_callback);
		}

		if(payload) {

			// https://stackoverflow.com/questions/11600130/post-data-with-libcurl
			// https://curl.se/libcurl/c/http-post.html

			buffers.payload = payload;

			curl_easy_setopt(hCurl, CURLOPT_READDATA, (void *) this);
			curl_easy_setopt(hCurl, CURLOPT_READFUNCTION, read_callback);

		}

		return perform();
	}

	size_t HTTP::Client::Worker::write_callback(void *contents, size_t size, size_t nmemb, Worker *worker) noexcept {

		size_t realsize = size * nmemb;
		worker->buffers.in.write((const char *) contents, realsize);
		return realsize;

	}

	size_t HTTP::Client::Worker::read_callback(char *outbuffer, size_t size, size_t nitems, Worker *worker) noexcept {

		size_t length = 0;

		try {

			if(!worker->buffers.out) {
				// worker->client->getPostPayload(worker->buffers.payload);
				worker->buffers.out = worker->buffers.payload.c_str();

				if(Config::Value<bool>("http","trace-payload",true).get()) {
					cout << "http\tPosting to " << worker->client->url << endl << worker->buffers.out << endl;
				}
			}

			if(!*worker->buffers.out) {
				return 0;
			}

			length = std::min((size * nitems), strlen(worker->buffers.out));

			memcpy(outbuffer,worker->buffers.out,length);
			worker->buffers.out += length;

		} catch(const std::exception &e) {

			cerr << "curl\tError '" << e.what() << "' getting post payload" << endl;
			return CURL_READFUNC_ABORT;

		} catch(...) {

			cerr << "curl\tUnexpected error getting post payload" << endl;
			return CURL_READFUNC_ABORT;

		}

		return length;
	}

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	int HTTP::Client::Worker::trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Worker *worker) noexcept {

		cout << "***" << endl;

		switch (type) {
		case CURLINFO_TEXT:
			cout << data << endl;
			return 0;

		case CURLINFO_HEADER_OUT:
			cout << "=> Send header" << endl;
			break;

		case CURLINFO_DATA_OUT:
			cout << "=> Send data" << endl;
			break;

		case CURLINFO_SSL_DATA_OUT:
			cout << "=> Send SSL data" << endl;
			break;

		case CURLINFO_HEADER_IN:
			cout <<  "<= Recv header" << endl;
			break;

		case CURLINFO_DATA_IN:
			cout << "<= Recv data" << endl;
			break;

		case CURLINFO_SSL_DATA_IN:
			cout << "<= Recv SSL data" << endl;
			break;

		default:
			return 0;

		}

		return 0;

	}
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	int HTTP::Client::Worker::sockopt_callback(Worker *worker, curl_socket_t curlfd, curlsocktype purpose) noexcept {
		return CURL_SOCKOPT_ALREADY_CONNECTED;
	}
	#pragma GCC diagnostic pop

	size_t HTTP::Client::Worker::header_callback(char *buffer, size_t size, size_t nitems, Worker *worker) noexcept {

		string header(buffer,size*nitems);

		if(strncasecmp(header.c_str(),"HTTP/",5) == 0 && header.size()) {
			unsigned int v[2];
			unsigned int code;
			char str[201];
			memset(str,0,sizeof(str));

			if(sscanf(header.c_str()+5,"%u.%u %d %200c",&v[0],&v[1],&code,str) == 4) {

				for(size_t ix = 0; ix < sizeof(str);ix++) {
					if(str[ix] < ' ') {
						str[ix] = 0;
						break;
					}
				}

				worker->message = str;
			}

			cout << "http\t" << worker->client->url << " " << header << endl;

		} else if(strncasecmp(header.c_str(),"Last-Modified:",14) == 0 && header.size()) {

			try {

				const char *ptr = header.c_str()+15;
				while(*ptr && isspace(*ptr)) {
					ptr++;
				}

				if(*ptr) {
					worker->timestamp.set(ptr);
#ifdef DEBUG
					cout << "Server time: " << worker->timestamp << endl;
#endif // DEBUG
				}

			} catch(const std::exception &e) {

				cerr << "http\tError '" << e.what() << "' processing transfer date" << endl;

			} catch(...) {

				cerr << "http\tunexpected error processing transfer date" << endl;

			}

		}
#ifdef DEBUG
		else {
			cout << "Header: " << header << endl;
		}
#endif // DEBUG

		return size*nitems;
	}

	curl_socket_t HTTP::Client::Worker::open_socket_callback(Worker *worker, curlsocktype purpose, struct curl_sockaddr *address) noexcept {

		// https://curl.se/libcurl/c/externalsocket.html

		if(purpose != CURLSOCKTYPE_IPCXN) {
			cerr << "curl\tInvalid type in curl_opensocket" << endl;
			return CURL_SOCKET_BAD;
		}

		int sockfd = socket(address->family,address->socktype,address->protocol);
		if(sockfd < 0) {
			cerr << "curl\tError '" << strerror(errno) << "' creating socket to " << worker->client->url << endl;
			return CURL_SOCKET_BAD;
		}

		//
		// Setup socket timeouts.
		//
		struct timeval tv;
		memset(&tv,0,sizeof(tv));

		tv.tv_sec = Config::Value<unsigned int>("http","socket_rcvtimeo",30).get();
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));

		tv.tv_sec = Config::Value<unsigned int>("http","socket_sndtimeo",30).get();
		setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv,sizeof(struct timeval));

		//
		// Connect to host.
		//
		if(connect(sockfd,(struct sockaddr *)(&(address->addr)),address->addrlen)) {
			cerr << "curl\Error '" << strerror(errno) << "' connecting to " << worker->client->url << endl;
			::close(sockfd);
			return CURL_SOCKET_BAD;
		}

		return (curl_socket_t) sockfd;

	}


 #endif // HAVE_CURL
 }
