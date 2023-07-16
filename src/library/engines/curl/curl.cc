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
 #include <udjat/defs.h>
 #include <private/engine.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/mainloop.h>
 #include <unistd.h>
 #include <fcntl.h>
 #include <iostream>
 #include <poll.h>
 #include <udjat/tools/configuration.h>

 using namespace std;

 namespace Udjat {

	HTTP::Engine::Engine(const char *url, int timeout) : hCurl{curl_easy_init()} {

		curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);

		memset(error,0,CURL_ERROR_SIZE+1);
		curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, error);

		curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, (void *) this);
		curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, header_callback);

		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

		// https://curl.se/libcurl/c/CURLOPT_FORBID_REUSE.html
		curl_easy_setopt(hCurl, CURLOPT_FORBID_REUSE, 1L);

		// Maximum time the transfer is allowed to complete.
		if(timeout) {
			curl_easy_setopt(hCurl, CURLOPT_TIMEOUT, timeout);
		}

		curl_easy_setopt(hCurl, CURLOPT_URL, url);
		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

	}

	HTTP::Engine::~Engine() {
		curl_easy_cleanup(hCurl);
	}

	int HTTP::Engine::response_code() {
		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);
		return (int) response_code;
	}

	size_t HTTP::Engine::write_callback(void *contents, size_t size, size_t nmemb, Engine *engine) noexcept {

		size_t realsize = size * nmemb;

		try {

			engine->write(contents,realsize);
			return realsize;

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("curl");
			strncpy(engine->error,e.what(),CURL_ERROR_SIZE);

		} catch(...) {

			Logger::String{"Unexpected error"}.error("curl");

		}

		return CURL_WRITEFUNC_ERROR;
	}

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	int HTTP::Engine::trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Engine *engine) noexcept {

		Logger::String logger{""};

		switch (type) {
		case CURLINFO_TEXT:
			logger.append(data);
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
			logger.append("<= Recv header");
			break;

		case CURLINFO_DATA_IN:
			logger.append("<= Recv data");
			break;

		case CURLINFO_SSL_DATA_IN:
			logger.append("<= Recv SSL data");
			break;

		default:
			return 0;

		}

		logger.write(Logger::Debug,"curl");

		return 0;

	}
	#pragma GCC diagnostic pop

	void HTTP::Engine::socket(int) {
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
			Logger::String{"Invalid type '",purpose,"' in curl_opensocket"}.error("curl");
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
					strncpy(engine->error,strerror(errno),sizeof(engine->error));
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

						strncpy(engine->error,strerror(errno),sizeof(engine->error));
						cerr << "curl\tError '" << engine->error << "' (" << errno << ") while connecting" << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLHUP) {

						strncpy(engine->error,strerror(ECONNRESET),sizeof(engine->error));
						cerr << "curl\tError '" << engine->error << "' (" << errno << ") while connecting" << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLOUT) {
						debug("Connected");
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
				strncpy(engine->error,strerror(ETIMEDOUT),sizeof(engine->error));
				cerr << "curl\tError '" << engine->error << "' (" << errno << ") on connect" << endl;
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			// Set blocking
			if(non_blocking(sockfd,false)) {
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

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

			engine->socket(sockfd);
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

 }


 /*
 #include <config.h>
 #include <private/worker.h>
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/logger.h>
 #include <iostream>
 #include <fcntl.h>
 #include <udjat/tools/mainloop.h>
 #include <poll.h>
 #include <udjat/tools/string.h>

 using namespace std;

 namespace Udjat {
 #ifdef HAVE_CURL

	#ifdef DEBUG
		#define TRACE_DEFAULT true
	#else
		#define TRACE_DEFAULT false
	#endif  // DEBUG

	HTTP::Worker::Worker(const char *url, const HTTP::Method method, const char *payload) : Protocol::Worker(url,method,payload) {

		memset(error,0,CURL_ERROR_SIZE);

		hCurl = curl_easy_init();


	}

	HTTP::Worker & HTTP::Worker::credentials(const char *user, const char *passwd) {
		curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(hCurl, CURLOPT_USERNAME, user);
		curl_easy_setopt(hCurl, CURLOPT_PASSWORD, passwd);
		return *this;
	}

	HTTP::Worker::~Worker() {
		curl_easy_cleanup(hCurl);
	}

	curl_slist * HTTP::Worker::headers() const noexcept {
		struct curl_slist *chunk = NULL;
		for(const Header &header : headerlist) {
			debug(header.name(),":",header.value());
			chunk = curl_slist_append(chunk,(string(header.name()) + ":" + header.value()).c_str());
		}
		return chunk;
	}

	Udjat::String HTTP::Worker::perform() {

		debug("Current URL is '",(std::string &) url(),"'");

		curl_easy_setopt(hCurl, CURLOPT_URL, url().c_str());
		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

		struct curl_slist *chunk = headers();
		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}

		this->error[0] = 0;
		CURLcode res = curl_easy_perform(hCurl);
		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {
			debug("Error='",this->error,"'");
			throw HTTP::Exception(url().c_str(),(this->error[0] ? this->error : curl_easy_strerror(res)));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		Logger::String{"",url().c_str()," ",response_code," ",message}.write(Logger::Trace,"curl");

		if(response_code >= 200 && response_code <= 299) {
			return Udjat::String(buffers.in.str().c_str());
		}

		if(message.empty()) {
			throw HTTP::Exception((unsigned int) response_code, url().c_str());
		} else {
			throw HTTP::Exception((unsigned int) response_code, url().c_str(), message.c_str());
		}

	}

	size_t HTTP::Worker::read_callback(char *outbuffer, size_t size, size_t nitems, Worker *worker) noexcept {

		size_t length = 0;

		try {

			if(!worker->buffers.out) {
				worker->buffers.out = worker->out.payload.c_str();
				if(Config::Value<bool>("http","trace-payload",TRACE_DEFAULT).get()) {
					Logger::String("Posting to ",worker->url().c_str()).write(Logger::Trace,"http");
					Logger::String("",worker->buffers.out).write(Logger::Trace);
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
	int HTTP::Worker::sockopt_callback(Worker *worker, curl_socket_t curlfd, curlsocktype purpose) noexcept {
		return CURL_SOCKOPT_ALREADY_CONNECTED;
	}
	#pragma GCC diagnostic pop

	size_t HTTP::Worker::header_callback(char *buffer, size_t size, size_t nitems, Worker *worker) noexcept {

		Udjat::String header{(const char *) buffer,(size_t) (size*nitems)};

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

			Logger::String{worker->url().c_str()," ",header}.write(Logger::Debug,"curl");


		} else if(strncasecmp(header.c_str(),"Content-Length:",15) == 0 && header.size()) {

				char *end = nullptr;
				worker->content_length = strtoull(header.c_str()+16,&end,10);
				Logger::String{worker->url().c_str()," has ",String{}.set_byte(worker->content_length)," bytes"}.write(Logger::Debug,"curl");

		} else if(strncasecmp(header.c_str(),"Last-Modified:",14) == 0 && header.size()) {

			try {

				const char *ptr = header.c_str()+15;
				while(*ptr && isspace(*ptr)) {
					ptr++;
				}

				if(*ptr) {
					worker->in.modification = HTTP::TimeStamp(ptr);
					Logger::String{"last-modified (from server): ",worker->in.modification.to_string()}.write(Logger::Debug,"curl");
				}

			} catch(const std::exception &e) {

				Logger::String{"Unable to process '",header.c_str(),"': ",e.what()}.warning("curl");

			} catch(...) {

				Logger::String{"Unexpected error processing '",header.c_str(),"'"}.warning("curl");

			}

		} else if(!header.strip().empty()) {

			const char *from = header.c_str();
			const char *delimiter = strchr(from,':');
			if(delimiter) {

				String name{from,(size_t) (delimiter-from)};
				String value{delimiter+1};

				name.strip();
				value.strip();

				worker->header(name.c_str()) = value;
			}

		}

		return size*nitems;
	}


 #endif // HAVE_CURL
 }
 */
