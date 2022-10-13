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
 #include <udjat/tools/http/worker.h>
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
			chunk = curl_slist_append(chunk,(string(header.name()) + ":" + header.value()).c_str());
		}
		return chunk;
	}

	Udjat::String HTTP::Worker::perform() {

#ifdef DEBUG
		cout << "curl\t*** Current URL is '" << url() << "'" << endl;
#endif // DEBUG

		curl_easy_setopt(hCurl, CURLOPT_URL, url().c_str());
		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

		struct curl_slist *chunk = headers();
		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}
		CURLcode res = curl_easy_perform(hCurl);
		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {
			throw HTTP::Exception(url().c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		Logger::String{"",url().c_str()," ",response_code," ",message}.write(Logger::Trace,"http");

		if(response_code >= 200 && response_code <= 299) {
			return Udjat::String(buffers.in.str().c_str());
		}

		if(message.empty()) {
			throw HTTP::Exception((unsigned int) response_code, url().c_str());
		} else {
			throw HTTP::Exception((unsigned int) response_code, url().c_str(), message.c_str());
		}

	}

	Udjat::String HTTP::Worker::get(const std::function<bool(double current, double total)> &progress) {

		switch(method()) {
		case HTTP::Get:
			break;

		case HTTP::Post:
			curl_easy_setopt(hCurl, CURLOPT_POST, 1);
			break;

		case HTTP::Put:
			curl_easy_setopt(hCurl, CURLOPT_PUT, 1);
			break;

		case HTTP::Delete:
			curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;

		default:
			throw system_error(EINVAL,system_category(),"Invalid or unsupported http verb");
		}

		if(Config::Value<bool>("http","trace",TRACE_DEFAULT).get()) {
			curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, this);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, trace_callback);
		}

		if(!out.payload.empty()) {

			// https://stackoverflow.com/questions/11600130/post-data-with-libcurl
			// https://curl.se/libcurl/c/http-post.html
			curl_easy_setopt(hCurl, CURLOPT_READDATA, (void *) this);
			curl_easy_setopt(hCurl, CURLOPT_READFUNCTION, read_callback);

		}

		return perform();

	}

	size_t HTTP::Worker::write_callback(void *contents, size_t size, size_t nmemb, Worker *worker) noexcept {

		size_t realsize = size * nmemb;
		worker->buffers.in.write((const char *) contents, realsize);
		return realsize;

	}

	size_t HTTP::Worker::read_callback(char *outbuffer, size_t size, size_t nitems, Worker *worker) noexcept {

		size_t length = 0;

		try {

			if(!worker->buffers.out) {
				worker->buffers.out = worker->out.payload.c_str();
				if(Config::Value<bool>("http","trace-payload",false).get()) {
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
	int HTTP::Worker::trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Worker *worker) noexcept {

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

		logger.write(Logger::Trace,"curl");

		return 0;

	}
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	int HTTP::Worker::sockopt_callback(Worker *worker, curl_socket_t curlfd, curlsocktype purpose) noexcept {
		return CURL_SOCKOPT_ALREADY_CONNECTED;
	}
	#pragma GCC diagnostic pop

	size_t HTTP::Worker::header_callback(char *buffer, size_t size, size_t nitems, Worker *worker) noexcept {

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

			Logger::String("",worker->url()," ",header).write(Logger::Trace,"http");

		} else if(strncasecmp(header.c_str(),"Last-Modified:",14) == 0 && header.size()) {

			try {

				const char *ptr = header.c_str()+15;
				while(*ptr && isspace(*ptr)) {
					ptr++;
				}

				if(*ptr) {
					worker->in.modification = HTTP::TimeStamp(ptr);
#ifdef DEBUG
					cout << "last-modified: " << worker->in.modification << endl;
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

	static int non_blocking(int sock, bool on) {

		int f;

		if ((f = fcntl(sock, F_GETFL, 0)) == -1) {
			cerr << "fcntl() error '" << strerror(errno) << "' when getting socket state." << endl;
			return -1;
		}

		if (on) {
			f |= O_NDELAY;
		} else {
			f &= ~O_NDELAY;
		}

		if (fcntl(sock, F_SETFL, f) < 0) {
			cerr << "fcntl() error '" << strerror(errno) << "' when setting socket state." << endl;
			return -1;
		}

		return 0;

	}

	curl_socket_t HTTP::Worker::open_socket_callback(Worker *worker, curlsocktype purpose, struct curl_sockaddr *address) noexcept {

		// https://curl.se/libcurl/c/externalsocket.html
		debug("Connecting to ",worker->url());

		if(purpose != CURLSOCKTYPE_IPCXN) {
			cerr << "curl\tInvalid type in curl_opensocket" << endl;
			return CURL_SOCKET_BAD;
		}

		int sockfd = socket(address->family,address->socktype,address->protocol);
		if(sockfd < 0) {
			cerr << "curl\tError '" << strerror(errno) << "' creating socket to " << worker->url() << endl;
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
				cerr << "curl\tError '" << strerror(errno) << "' (" << errno << ") connecting to " << worker->url() << endl;
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

					cerr << "curl\tError '" << strerror(errno) << "' connecting to " << worker->url() << endl;
					::close(sockfd);
					return CURL_SOCKET_BAD;

				} else if(rc == 1) {

					if(pfd.revents & POLLERR) {

						int error = EINVAL;
						socklen_t errlen = sizeof(error);
						if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0) {
							error = errno;
						}

						cerr << "curl\tError '" << strerror(error) << "' connecting to " << worker->url() << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLHUP) {

						cerr << "curl\tError '" << strerror(ECONNRESET) << "' connecting to " << worker->url() << endl;
						::close(sockfd);
						return CURL_SOCKET_BAD;

					}

					if(pfd.revents & POLLOUT) {
						debug("Connected to ",worker->url());
						break;
					}

				} else if(!mainloop) {

					cerr << "curl\tMainLoop disabled, aborting connect to " << worker->url() << endl;
					::close(sockfd);
					return CURL_SOCKET_BAD;

				} else {

					timer--;

				}

			}

			if(!timer) {
				cerr << "curl\tTimeout connecting to " << worker->url() << endl;
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

		/*
		//
		// Connect to host.
		//
		if(connect(sockfd,(struct sockaddr *)(&(address->addr)),address->addrlen)) {
			cerr << "curl\tError '" << strerror(errno) << "' (" << errno << ") connecting to " << worker->url() << endl;
			::close(sockfd);
			return CURL_SOCKET_BAD;
		}
		*/

		return (curl_socket_t) sockfd;

	}

 #endif // HAVE_CURL
 }
