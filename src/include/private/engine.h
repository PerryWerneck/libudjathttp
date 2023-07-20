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
  * @brief Declare HTTP writer.
  */

 #pragma once
 #include <config.h>
 #include <udjat/tools/http/worker.h>
 #include <udjat/defs.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/http/timestamp.h>

#if defined(HAVE_WINHTTP)

	#include <winhttp.h>
	#include <udjat/win32/exception.h>

#elif defined(HAVE_CURL)

	#include <curl/curl.h>

#else

	#error Cant determine HTTP engine

#endif // HAVE_WINHTTP

 namespace Udjat {

 	namespace HTTP {

#ifdef _WIN32

		template <typename T>
		class UDJAT_API Handle {
		private:
			T sysptr;

		public:

			Handle(T ptr) : sysptr{ptr} {
				if(!sysptr) {
					throw Win32::Exception();
				}
			}

			~Handle() {
				WinHttpCloseHandle(sysptr);
			}

			inline operator bool() const noexcept {
				return sysptr;
			}

			inline operator T() noexcept {
				return sysptr;
			}

		};

#endif // _WIN32

		/// @brief The HTTP client engine.
		class UDJAT_PRIVATE Engine {
		private:

#if defined(HAVE_WINHTTP)

			/// @brief WinHTTP session handle.
			HTTP::Handle<HINTERNET> session;

			const HTTP::Method method;

			wchar_t *pwszUrl;

			URL_COMPONENTS urlComp;

#elif defined(HAVE_CURL)

			/// @brief Handle to curl.
			CURL * hCurl;

			/// @brief Curl error.
			char error[CURL_ERROR_SIZE+1];

			/// @brief HTTP response message.
			std::string message;

			static size_t do_write(void *contents, size_t size, size_t nmemb, Engine *engine) noexcept;
			static size_t read_callback(char *buffer, size_t size, size_t nitems, Engine *engine) noexcept;
			static size_t write_callback(void *contents, size_t size, size_t nmemb, Engine *engine) noexcept;
			static int trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Engine *engine) noexcept;
			static curl_socket_t open_socket_callback(Engine *engine, curlsocktype purpose, struct curl_sockaddr *address) noexcept;
			static int sockopt_callback(Engine *engine, curl_socket_t curlfd, curlsocktype purpose) noexcept;
			static size_t header_callback(char *buffer, size_t size, size_t nitems, Engine *engine) noexcept;

			struct curl_slist *headers = NULL;
			const char *outptr = nullptr;

#endif // HAVE_WINHTTP

		protected:

			HTTP::Worker &worker;

		public:
			Engine(HTTP::Worker &worker, const HTTP::Method method, time_t timeout = 0);

			Engine(HTTP::Worker &worker, time_t timeout = 0) : Engine(worker,worker.method(),timeout) {
			}

			~Engine();

			int response_code();

			/// @brief Perform.
			/// @param except If true launch exception if http response is not 200-299.
			/// @return HTTP status code.
			/// @retval -1 Unexpected exception.
			int perform(bool except = true);

			/// @brief Write to output file.
			virtual void write(const void *contents, size_t length) = 0;

			/// @brief Set content-length from server.
			virtual void content_length(unsigned long long length);

			/// @brief Set response header.
			virtual void response(const char *name, const char *value) = 0;

		};

 	}

 }

/*
 namespace Udjat {

 	namespace HTTP {

		class UDJAT_PRIVATE Writer {
		private:

			std::string error_message;

#ifdef HAVE_CURL
			static size_t do_write(void *contents, size_t size, size_t nmemb, Writer *writer) noexcept;
#endif // HAVE_CURL

		protected:

			const HTTP::Worker &worker;

			unsigned long long current = 0;
			unsigned long long total = 0;

#ifdef HAVE_CURL
			CURL *hCurl = nullptr;
#endif // HAVE_CURL

		public:

			Writer(const HTTP::Worker &w): worker{w} {
			}

#ifdef HAVE_CURL

			Writer(const HTTP::Worker &w, CURL *c) : worker{w}, hCurl{c} {
			};

			void get(curl_slist *chunk);

#endif // HAVE_CURL

			int response_code();

			virtual void allocate(unsigned long long length) = 0;
			virtual void write(const void *contents, size_t length) = 0;

		};

 	}

 }

*/
