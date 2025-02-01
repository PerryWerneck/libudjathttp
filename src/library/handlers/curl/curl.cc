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

#error deprecated

/*
 #include <config.h>
 #include <udjat/defs.h>
 #include <private/engine.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/mainloop.h>
 #include <udjat/tools/exception.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <iostream>
 #include <poll.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/intl.h>

 #ifdef DEBUG
	#define TRACE_DEFAULT true
 #else
	#define TRACE_DEFAULT false
 #endif  // DEBUG

 using namespace std;

 namespace Udjat {

 	class UDJAT_PRIVATE CurlException : public Udjat::Exception {
	public:
		CurlException(CURLcode res, const char *message, const char *url = "") : Udjat::Exception{res,curl_easy_strerror(res)} {
			info.title = _( "Network operation failed" );
			info.url = url;
			info.body = message;
			info.domain = "curl";
		}

		void write(const Logger::Level level = Logger::Error) const noexcept override {
			debug("Title: ",info.title.c_str());
			debug("what:  ",what());
			debug("body:  ",info.body.c_str());
			Logger::String{info.body.c_str()}.write(level,"curl");
		}

	};

	int HTTP::Engine::perform(bool except) {

		outptr = nullptr;
		this->error[0] = 0;
		CURLcode res = curl_easy_perform(hCurl);

		if(res != CURLE_OK) {
			if(this->error[0]) {
				throw CurlException(res,this->error);
			} else {
				throw runtime_error(curl_easy_strerror(res));
			}
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

	

		return check_result((int) response_code, except);
	}

	HTTP::Engine::Engine(HTTP::Worker &w, const HTTP::Method method, time_t timeout) : hCurl{curl_easy_init()}, worker{w} {

		Logger::String{"Starting worker ",(unsigned long long) this}.write(Logger::Debug,"curl");

		curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

		memset(error,0,CURL_ERROR_SIZE+1);
		curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, error);

		curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, (void *) this);
		curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, header_callback);

		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_CLOSESOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_CLOSESOCKETFUNCTION, close_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

		// https://curl.se/libcurl/c/CURLOPT_FORBID_REUSE.html
		curl_easy_setopt(hCurl, CURLOPT_FORBID_REUSE, 1L);

		// Maximum time the transfer is allowed to complete.
		if(timeout) {
			curl_easy_setopt(hCurl, CURLOPT_TIMEOUT, timeout);
		}

		debug("URL='",worker.url().c_str());
		curl_easy_setopt(hCurl, CURLOPT_URL, worker.url().c_str());

		switch(method) {
		case HTTP::Get:
			curl_easy_setopt(hCurl, CURLOPT_HTTPGET, 1L);
			break;

		case HTTP::Head:
			curl_easy_setopt(hCurl, CURLOPT_NOBODY, 1L);
			break;

		case HTTP::Post:
			curl_easy_setopt(hCurl, CURLOPT_POST, 1);
			break;

		case HTTP::Put:
			curl_easy_setopt(hCurl, CURLOPT_UPLOAD, 1);
			break;

		case HTTP::Delete:
			curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;

		default:
			throw system_error(EINVAL,system_category(),_( "Invalid or unsupported http verb" ));
		}

		if(Logger::enabled(Logger::Debug) && Config::Value<bool>("http","trace",TRACE_DEFAULT).get()) {
			debug("Trace is enabled");
			curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, this);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, trace_callback);
		}

		if(*worker.payload()) {

			// https://stackoverflow.com/questions/11600130/post-data-with-libcurl
			// https://curl.se/libcurl/c/http-post.html
			debug("Has payload, enabling it");
			curl_easy_setopt(hCurl, CURLOPT_READDATA, (void *) this);
			curl_easy_setopt(hCurl, CURLOPT_READFUNCTION, read_callback);

		}

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

		{
			const char *user = worker.user();
			const char *passwd = worker.passwd();

			if(user && *user && passwd && *passwd) {
				curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
				curl_easy_setopt(hCurl, CURLOPT_USERNAME, user);
				curl_easy_setopt(hCurl, CURLOPT_PASSWORD, passwd);
			}

		}

		for(auto &header : worker.requests()) {
			debug(header.name(),": ",header.value());
			headers = curl_slist_append(headers,String{header.name(),":",header.value()}.c_str());
		}

		if(headers) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, headers);
		}

	}

	HTTP::Engine::~Engine() {

		Logger::String{"Stopping worker ",(unsigned long long) this}.write(Logger::Debug,"curl");

		if(headers) {
			curl_slist_free_all(headers);
		}
		curl_easy_cleanup(hCurl);
	}

	int HTTP::Engine::response_code() {
		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);
		return (int) response_code;
	}

	size_t HTTP::Engine::write_callback(void *contents, size_t size, size_t nmemb, Engine *engine) noexcept {

		size_t realsize = size * nmemb;

		debug("realsize=",realsize);

		try {

			engine->write(contents,realsize);
			return realsize;

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("curl");
			strncpy(engine->error,e.what(),CURL_ERROR_SIZE);

		} catch(...) {

			Logger::String{"Unexpected error"}.error("curl");

		}

#ifdef CURL_WRITEFUNC_ERROR
		return CURL_WRITEFUNC_ERROR;
#else
		return -1;
#endif // CURL_WRITEFUNC_ERROR
	}

	int HTTP::Engine::trace_callback(CURL *, curl_infotype type, char *data, size_t size, Engine *) noexcept {

		Logger::String logger{""};

		switch (type) {
		case CURLINFO_TEXT:
			logger.append(data,size);
			return 0;

		case CURLINFO_HEADER_OUT:
			logger.append("=> Send header");
			break;

		case CURLINFO_DATA_OUT:
			logger.append("=> Send data");
			break;

		case CURLINFO_SSL_DATA_OUT:
			logger.append("=> Send SSL data");
			break;

		case CURLINFO_HEADER_IN:
			logger.append("<= Recv header (",size," bytes)");
			break;

		case CURLINFO_DATA_IN:
			logger.append("<= Recv data (",size," bytes)");
			break;

		case CURLINFO_SSL_DATA_IN:
			logger.append("<= Recv SSL data");
			break;

		default:
			debug("Unexpected type '",type,"'");
			return 0;

		}

		logger.write(Logger::Debug,"curl");

		return 0;

	}

	static int non_blocking(int sock, bool on) {

		int f;

		if ((f = fcntl(sock, F_GETFL, 0)) == -1) {
			cerr << "curl\tfcntl() error '" << strerror(errno) << "' when getting socket state." << endl;
			return -1;
		}

		if (on) {
			f |= O_NDELAY;
		} else {
			f &= ~O_NDELAY;
		}

		if (fcntl(sock, F_SETFL, f) < 0) {
			cerr << "curl\tfcntl() error '" << strerror(errno) << "' when setting socket state." << endl;
			return -1;
		}

		return 0;

	}

	curl_socket_t HTTP::Engine::open_socket_callback(Engine *engine, curlsocktype purpose, struct curl_sockaddr *address) noexcept {

		// https://curl.se/libcurl/c/externalsocket.html
		debug("Connecting to host");

		if(purpose != CURLSOCKTYPE_IPCXN) {
			Logger::String{"Invalid purpose '",purpose,"' in curl_opensocket"}.error("curl");
			return CURL_SOCKET_BAD;
		}

		int sockfd = ::socket(address->family,address->socktype,address->protocol);
		if(sockfd < 0) {
			Logger::String{"Error '",strerror(errno),"' creating socket"}.error("curl");
			return CURL_SOCKET_BAD;
		}

		//
		// Non blocking connect
		//
		{
			MainLoop &mainloop = MainLoop::getInstance();

			// Set non blocking & connect
			if(non_blocking(sockfd,true)) {
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			// Connect to host.
			if(!connect(sockfd,(struct sockaddr *)(&(address->addr)),address->addrlen)) {
				Logger::String{"Connected to host using socket '",sockfd,"'"}.trace("curl");
				return (curl_socket_t) sockfd;
			}

			if(errno != EINPROGRESS) {
				strncpy(engine->error,strerror(errno),CURL_ERROR_SIZE);
				Logger::String{"Error '",engine->error,"' (",errno,") connecting to host"}.error("curl");
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			// Wait
			struct pollfd pfd;
			unsigned long timer{ Config::Value<unsigned long>("http","socket_cnctimeo",30) * 100 };
			while(timer > 0) {

				pfd.fd = sockfd;
				pfd.revents = 0;
				pfd.events = POLLOUT|POLLERR|POLLHUP;

				auto rc = poll(&pfd,1,10);

				if(rc == -1) {
					strncpy(engine->error,strerror(errno),sizeof(engine->error)-1);
					cerr << "curl\tError '" << engine->error << "' (" << errno << ") on connect" << endl;
					::close(sockfd);
					return CURL_SOCKET_BAD;

				} else if(rc == 1) {

					if(pfd.revents & POLLERR) {

						int error = EINVAL;
						socklen_t errlen = sizeof(error);
						if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {
							error = errno;
						}

						strncpy(engine->error,strerror(error),sizeof(engine->error)-1);
						cerr << "curl\tError '" << engine->error << "' (" << error << ") while connecting" << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLHUP) {

						strncpy(engine->error,strerror(ECONNRESET),sizeof(engine->error)-1);
						cerr << "curl\tError '" << engine->error << "' (" << errno << ") while connecting" << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLOUT) {
						break;
					}

				} else if(!mainloop) {

					cerr << "curl\tMainLoop disabled, aborting connect" << endl;
					::close(sockfd);
					return CURL_SOCKET_BAD;

				} else {

					timer--;

				}

			}

			if(!timer) {
				strncpy(engine->error,strerror(ETIMEDOUT),sizeof(engine->error)-1);
				cerr << "curl\tError '" << engine->error << "' (" << errno << ") on connect" << endl;
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			// Set blocking
			if(non_blocking(sockfd,false)) {
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			Logger::String{"Connected to host using socket '",sockfd,"'"}.trace("curl");

		}

		//
		// Setup socket timeouts.
		//
		{
			struct timeval tv;
			memset(&tv,0,sizeof(tv));

			tv.tv_sec = Config::Value<unsigned int>("http","socket_rcvtimeo",30).get();
			setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));

			tv.tv_sec = Config::Value<unsigned int>("http","socket_sndtimeo",30).get();
			setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
		}

		// Update worker
		try {

			engine->worker.socket(sockfd);
			return (curl_socket_t) sockfd;

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("curl");
			strncpy(engine->error,e.what(),CURL_ERROR_SIZE);

		} catch(...) {

			Logger::String{"Unexpected error"}.error("curl");

		}

		::close(sockfd);
		return CURL_SOCKET_BAD;

	}

	int HTTP::Engine::close_socket_callback(Engine *, curl_socket_t item) {

#ifdef _WIN32
		if(sockclose(item)) {
			Logger::String{"Error '",WSAGetLastError(),"' closing socket ",item}.warning("curl");
			return 1;
		}
#else
		int rc = ::close(item);
		if(rc) {
			Logger::String{"Error '",strerror(rc),"' closing socket ",item}.warning("curl");
			return 1;
		}
#endif // _WIN32

		Logger::String{"Socket ",item," was closed"}.trace("curl");
		return 0;

	}

	size_t HTTP::Engine::header_callback(char *buffer, size_t size, size_t nitems, Engine *engine) noexcept {

		Udjat::String header{(const char *) buffer,(size_t) (size*nitems)};

		try {

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

					engine->message = str;
				}


			} else if(strncasecmp(header.c_str(),"Content-Length:",15) == 0 && header.size()) {

				char *end = nullptr;
				engine->content_length(strtoull(header.c_str()+16,&end,10));

			} else if(!header.strip().empty()) {

				const char *from = header.c_str();
				const char *delimiter = strchr(from,':');

				if(delimiter) {
					engine->response(
						String{from,(size_t) (delimiter-from)}.strip().c_str(),
						String{delimiter+1}.strip().c_str()
					);
				}

			}

			return size*nitems;

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("curl");
			strncpy(engine->error,e.what(),CURL_ERROR_SIZE);

		} catch(...) {

			Logger::String{"Unexpected error processing http header"}.error("curl");

		}

		// TODO: Abort on error.
		return size*nitems;

	}

	int HTTP::Engine::sockopt_callback(Engine *, curl_socket_t, curlsocktype) noexcept {
		return CURL_SOCKOPT_ALREADY_CONNECTED;
	}

	size_t HTTP::Engine::read_callback(char *outbuffer, size_t size, size_t nitems, Engine *engine) noexcept {

		size_t length = 0;

		try {

			if(!engine->outptr) {

				// Get payload.
				engine->outptr = engine->worker.payload();
				if(*engine->outptr && Logger::enabled(Logger::Trace) && Config::Value<bool>("http","trace-payload",TRACE_DEFAULT).get()) {
					Logger::String{"Payload\n",engine->outptr}.trace("curl");
				}

			}

			if(*engine->outptr) {
				size_t szpayload = strlen(engine->outptr);

				length = (size*nitems);
				if(szpayload < length) {
					length = szpayload;
				}

				memcpy(outbuffer,engine->outptr,length);
				engine->outptr += length;
			}

		} catch(const std::exception &e) {

			cerr << "curl\tError '" << e.what() << "' getting post payload" << endl;
			return CURL_READFUNC_ABORT;

		} catch(...) {

			cerr << "curl\tUnexpected error getting post payload" << endl;
			return CURL_READFUNC_ABORT;

		}

		return length;
	}

 }

*/
