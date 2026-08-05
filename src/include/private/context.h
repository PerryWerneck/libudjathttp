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
 #include <udjat/tools/url/handler.h>
 #include <udjat/tools/url/handler/http.h>
 #include <vector>
 #include <functional>
 
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

		class UDJAT_PRIVATE Context {
		private:
			HTTP::Handler *handler;
			const std::function<bool(uint64_t current, uint64_t total, const void *data, size_t len)> *write;

			void set_local(const sockaddr_storage &addr) noexcept;
			void set_remote(const sockaddr_storage &addr) noexcept;

#ifdef _WIN32
			/// @brief WinHTTP session handle.
			HTTP::Handle<HINTERNET> session;
			const HTTP::Method method;
			wchar_t *pwszUrl;
			DWORD dwStatusCode = 0;

#elif defined(HAVE_CURL)

			/// @brief Handle to curl.
			CURL * hCurl;
			int sock;

			uint64_t current = 0;
			uint64_t total = 0;

			struct {
				curl_slist *request = nullptr;
			} headers;

			struct {
				String text;
				const char *ptr = nullptr;
			} payload;

			struct {
				int system = 0;
				char message[CURL_ERROR_SIZE+1] = {0};
			} error;

			inline void system_error(int code = errno) noexcept {
				error.system = -code;
			}

			inline void exception(const std::exception &e) noexcept {
				error.system = 0;
				strncpy(error.message,e.what(),CURL_ERROR_SIZE);
			}

			int perform(bool except);

			static int trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Context *context) noexcept;
			static int sockopt_callback(Context *context, curl_socket_t curlfd, curlsocktype purpose) noexcept;

			static curl_socket_t open_socket_callback(Context *context, curlsocktype purpose, struct curl_sockaddr *address) noexcept;
			static int close_socket_callback(Context *engine, curl_socket_t item) noexcept;
			static size_t read_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept;
			static size_t write_callback(void *contents, size_t size, size_t nmemb, Context *context) noexcept;
			static size_t header_callback(char *buffer, size_t size, size_t nitems, Context *context) noexcept;
			static size_t no_write_callback(void *, size_t size, size_t nmemb, Context *context) noexcept;

#endif

		public:
			Context(HTTP::Handler &handler, const std::function<bool(uint64_t current, uint64_t total, const void *data, size_t len)> &writer);
			~Context();

			void set(const HTTP::Method method);

			int test(const HTTP::Method method, const char *payload) noexcept;
			int perform(const HTTP::Method method, const char *payload);

		};

	}

 }
