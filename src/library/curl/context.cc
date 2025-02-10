/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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
 #include <udjat/tools/url.h>
 #include <udjat/tools/url/handler.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/exception.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/socket.h>
 #include <udjat/tools/value.h>
 #include <udjat/tools/http/mimetype.h>
 #include <private/context.h>

 #include <errno.h>
 #include <curl/curl.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <system_error>

 #ifdef DEBUG
	#define TRACE_DEFAULT true
 #else
	#define TRACE_DEFAULT false
 #endif  // DEBUG

 using namespace std;

 namespace Udjat {

 	class UDJAT_PRIVATE CurlException : public Udjat::Exception {
	public:
		CurlException(CURLcode res, const char *message, const char *url) : Udjat::Exception{res,curl_easy_strerror(res)} {
			info.title = _( "HTTP operation failed" );
			info.url = url;
			if(message && *message) {
				info.body = message;
			}
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

	class UDJAT_PRIVATE CurlSingleton {
	private:
		CurlSingleton() {
			Logger::String{"Intializing curl version ", curl_version_info(CURLVERSION_NOW)->version}.trace(PACKAGE_NAME);
			curl_global_init(CURL_GLOBAL_ALL);
		}

	public:
		~CurlSingleton() {
			curl_global_cleanup();
		}

		static CurlSingleton &instance() {
			static CurlSingleton instance;
			return instance;
		}	

	};

	HTTP::Context::Context(HTTP::Handler &h, const std::function<bool(uint64_t current, uint64_t total, const char *data, size_t len)> &w) 
		: handler{h}, write{w} {

		CurlSingleton::instance();

		hCurl = curl_easy_init();
		if(!hCurl) {
			throw CurlException(CURLE_FAILED_INIT,"Failed to initialize curl",handler.url.c_str());
		}

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);

		curl_easy_setopt(hCurl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(hCurl, CURLOPT_FORBID_REUSE, 1L);
		curl_easy_setopt(hCurl, CURLOPT_URL, handler.url.c_str());

		curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, error.message);

		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_OPENSOCKETFUNCTION, open_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);

		curl_easy_setopt(hCurl, CURLOPT_CLOSESOCKETDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_CLOSESOCKETFUNCTION, close_socket_callback);

		curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_HEADERFUNCTION, header_callback);

		curl_easy_setopt(hCurl, CURLOPT_READDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_READFUNCTION, read_callback);

		// Load heaers
		if(!handler.headers.request.empty()) {
			for(const auto &header : handler.headers.request) {
				headers.request = curl_slist_append(headers.request,String{header.name,": ",header.value}.c_str());
			}
		}

		

	}
	
	HTTP::Context::~Context() {
		curl_easy_cleanup(hCurl);
		if(headers.request) {
			curl_slist_free_all(headers.request);
		}
	}

	void HTTP::Context::set(const HTTP::Method method) {

		switch(method) {
		case HTTP::Get:
			curl_easy_setopt(hCurl, CURLOPT_HTTPGET, 1L);
			break;

		case HTTP::Post:
			curl_easy_setopt(hCurl, CURLOPT_POST, 1L);
			break;

		case HTTP::Put:
			curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "PUT");
			break;

		case HTTP::Head:
			curl_easy_setopt(hCurl, CURLOPT_NOBODY, 1L);
			break;

		case HTTP::Delete:
			curl_easy_setopt(hCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;

		default:
			throw std::system_error(EINVAL,std::system_category(),"Unsupported HTTP verb");
		}

		if(Config::Value<bool>("http","trace",TRACE_DEFAULT).get()) {
			curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGDATA, this);
			curl_easy_setopt(hCurl, CURLOPT_DEBUGFUNCTION, trace_callback);
		}

	}

	int HTTP::Context::test(const HTTP::Method method, const char *pl) noexcept {

		set(method);
		payload.text = pl;

		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, no_write_callback);

		return perform(false);
	}

	int HTTP::Context::perform(const HTTP::Method method, const char *pl) {

		set(method);
		payload.text = pl;

		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_callback);

		return perform(true);

	}

	int HTTP::Context::perform(bool except) {

		debug(__FUNCTION__," handler=",handler.c_str());

		handler.headers.response.clear();
		payload.ptr = nullptr;

		if(headers.request) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, headers.request);
		}

		CURLcode res = curl_easy_perform(hCurl);

		debug("length=",total," message='",error.message,"' syserror=",error.system);

		if(error.message[0]) {
			handler.status.message = error.message;
		} else if(error.system) {
			handler.status.message = strerror(error.system);
		}

		if(res == CURLE_OK) {
			long response_code = 0;
			curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);
			debug("result=CURLE_OK, response_code=",response_code," except=",except);	
			handler.status.code = (int) response_code;
			return response_code;
		}

		debug("Curl response=",res," '",curl_easy_strerror(res),"' message='",handler.status.message.c_str(),"'");

		if(except) {
			if(error.system) {
				throw CurlException(
						res, 
						error.message[0] ? error.message : strerror(error.system),
						handler.c_str()
					);
			}
			throw CurlException(
					res, 
					error.message, 
					handler.c_str()
				);
		}

		debug(__FUNCTION__,"=",(error.system ? error.system : res));
		return error.system ? error.system : res;
		
	}

	size_t HTTP::Context::read_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept {

		size_t realsize = size * nitems;

		if(!context->payload.ptr) {
			
			if(context->payload.text.empty()) {
				return 0;
			}

			// TODO: Expand payload getting connection data from socket.


			// Point to start of payload.
			context->payload.ptr = context->payload.text.c_str();
		}

		if(*context->payload.ptr) {

			size_t len = strlen(context->payload.ptr);
			if(len > realsize) {
				len = realsize;
			}

			memcpy(buffer,context->payload.ptr,len);
			context->payload.ptr += len;

			return len;

		}

		return 0;

	}

	size_t HTTP::Context::no_write_callback(void *, size_t size, size_t nmemb, Context *) noexcept {
		return size * nmemb;
	}

	size_t HTTP::Context::write_callback(void *contents, size_t size, size_t nmemb, Context *context) noexcept {

		size_t realsize = size * nmemb;

		debug("----> realsize=",realsize,"\n",std::string{(const char *) contents,realsize}.c_str(),"\n");

		try {

			if(context->write(context->current,context->total,(const char *) contents, realsize)) {
				context->system_error(ECANCELED);
			} else {
				context->current += realsize;
				return realsize;
			}

		} catch(const std::exception &e) {

			context->exception(e);

		} catch(...) {

			context->error.system = 0;

		}

#ifdef CURL_WRITEFUNC_ERROR
		return CURL_WRITEFUNC_ERROR;
#else
		return -1;
#endif // CURL_WRITEFUNC_ERROR

	}

	int HTTP::Context::trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Context *context) noexcept {

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

	curl_socket_t HTTP::Context::open_socket_callback(Context *context, curlsocktype purpose, struct curl_sockaddr *address) noexcept {

		debug("Context=",((unsigned long long) context)," handler=",context->handler.c_str());

		Logger::String{"Connecting to ",context->handler.c_str()}.trace("curl");

		if(purpose != CURLSOCKTYPE_IPCXN) {
			Logger::String{"Invalid purpose '",purpose,"' in curl_opensocket"}.error();
			return CURL_SOCKET_BAD;
		}

		int sockfd = ::socket(address->family,address->socktype,address->protocol);
		if(sockfd < 0) {
			context->system_error();
			return CURL_SOCKET_BAD;
		}

		try {

			Socket::blocking(sockfd,false);

			if(!connect(sockfd,(struct sockaddr *)(&(address->addr)),address->addrlen)) {
				Logger::String{"Connected to host using socket '",sockfd,"'"}.trace("curl");
				return (curl_socket_t) sockfd;
			}

			if(errno != EINPROGRESS) {
				context->system_error();
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			if(Socket::wait_for_connection(sockfd,0) < 0) {
				context->system_error();
				::close(sockfd);
				return CURL_SOCKET_BAD;
			}

			Socket::blocking(sockfd,true);

		} catch(const std::exception &e) {

			context->exception(e);
			::close(sockfd);
			return CURL_SOCKET_BAD;

		} catch(...) {
			
			Logger::String{"Unexpected error setting socket to non-blocking"}.error("curl");
			::close(sockfd);
			return CURL_SOCKET_BAD;

		}

		Logger::String{"Connected to host using socket '",sockfd,"'"}.trace("curl");

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
	
		context->sock = sockfd;
		return (curl_socket_t) sockfd;

	}

	int HTTP::Context::close_socket_callback(Context *, curl_socket_t item) noexcept {

		try {

			Socket::close(item);

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("curl");

		} catch(...) {

			Logger::String{"Unexpected error closing socket"}.error("curl");

		}
		
				return 0;

	}

	int HTTP::Context::sockopt_callback(Context *, curl_socket_t curlfd, curlsocktype purpose) noexcept {
		return CURL_SOCKOPT_ALREADY_CONNECTED;
	}

	size_t HTTP::Context::header_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept {

		String header{(const char *) buffer,(size_t) (size*nitems)};
		debug("header=",header);

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

					strncpy(context->error.message,str,CURL_ERROR_SIZE);
				
				}


			} else if(strncasecmp(header.c_str(),"Content-Length:",15) == 0 && header.size()) {

				char *end = nullptr;
				context->total = strtoull(header.c_str()+16,&end,10);
				context->write(0,context->total,nullptr,0);

			} else if(!header.strip().empty()) {

				const char *from = header.c_str();
				const char *delimiter = strchr(from,':');

				if(delimiter) {
					context->handler.headers.response.emplace_back(
						String{from,(size_t) (delimiter-from)}.strip().c_str(),
						String{delimiter+1}.strip().c_str()
					);
				}

			}

		} catch(const std::exception &e) {

			context->exception(e);
			return CURL_WRITEFUNC_ERROR;

		} catch(...) {

			Logger::String{"Unexpected error processing header"}.error("curl");
			return CURL_WRITEFUNC_ERROR;

		}

		return size*nitems;

	}

 }

