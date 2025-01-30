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

 /**
  * @brief Declare curl based url handler.
  */

 #pragma once
 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/url.h>
 #include <vector>
 
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
		class UDJAT_PRIVATE Handler : public Udjat::URL::Handler {
		private:

#if defined(HAVE_WINHTTP)

			/// @brief WinHTTP session handle.
			HTTP::Handle<HINTERNET> session;

			const HTTP::Method method;

			wchar_t *pwszUrl;

			URL_COMPONENTS urlComp;

			DWORD dwStatusCode = 0;

#elif defined(HAVE_CURL)

			/// @brief Handle to curl.
			CURL * hCurl;

			static int trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Handler *handler) noexcept;
			static int close_socket_callback(Handler *engine, curl_socket_t item);
			static int sockopt_callback(Handler *handler, curl_socket_t curlfd, curlsocktype purpose) noexcept;

			/// @brief Current download session.
			struct Context {
				
				int sock;

				HTTP::Handler &handler;
				const std::function<bool(uint64_t current, uint64_t total, const char *data, size_t len)> &write;

				uint64_t current = 0;
				uint64_t total = 0;

				struct {
					String text;
					const char *ptr = nullptr;
				} payload;

				/// @brief Curl error.
				char error[CURL_ERROR_SIZE+1] = {0};

				Context(HTTP::Handler *h, const std::function<bool(uint64_t current, uint64_t total, const char *data, size_t len)> &w) : handler{*h}, write{w} {
				}

			};

			static curl_socket_t open_socket_callback(Context *context, curlsocktype purpose, struct curl_sockaddr *address) noexcept;
			static size_t read_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept;
			static size_t write_callback(void *contents, size_t size, size_t nmemb, Context *context) noexcept;
			static size_t header_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept;

			const char *outptr = nullptr;

			void set(const HTTP::Method method);

			struct Header {
				std::string name;
				std::string value;
				Header(const char *n, const char *v) : name{n}, value{v} {
				}
			};

			std::vector<Header> response_headers;

#endif // HAVE_WINHTTP

			/// @brief HTTP response message.
			std::string message;

		protected:

			/// @brief Check result code, launch exception.
			/// @param except If true launch exception on error code.
			int check_result(int status_code, bool except = true);

		public:
			Handler(const URL &url);

			virtual ~Handler();

#if defined(HAVE_CURL)
			inline operator CURL *() noexcept {
				return hCurl;
			}
#endif // HAVE_CURL

			int test(const HTTP::Method method, const char *payload) override;

			int perform(const HTTP::Method method, const char *payload, const std::function<bool(uint64_t current, uint64_t total, const char *data, size_t len)> &progress) override;

			/*
			int response_code();

			inline const char * response_message() const noexcept {
				return message.c_str();
			}

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
			*/

		};

 	}

 }

