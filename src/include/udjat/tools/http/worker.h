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

 #pragma once

 #include <udjat/defs.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/http/exception.h>
 #include <sstream>

#if defined(_WIN32)

	#include <winhttp.h>
	#include <udjat/win32/exception.h>

#elif defined(HAVE_CURL)

	#include <curl/curl.h>

#else

	#error Cant determine HTTP backend

#endif

 namespace Udjat {

	namespace HTTP {

		class UDJAT_API Worker : public Protocol::Worker {
		private:

#if defined(_WIN32)

			/// @brief WinHTTP session handle.
			HINTERNET session;

			/// @brief Connect to HTTP host.
			HINTERNET connect();

			/// @brief Open HTTP Request.
			HINTERNET open(HINTERNET connection, const char *);

			/// @brief Send request.
			void send(HINTERNET request);

			/// @brief Wait for response.
			Udjat::String wait(HINTERNET req);

#elif defined(HAVE_CURL)

			CURL * hCurl;
			char error[CURL_ERROR_SIZE];

			struct {
				const char *out = nullptr;
				std::ostringstream in;
			} buffers;

			std::string message;
			static size_t read_callback(char *buffer, size_t size, size_t nitems, Worker *session) noexcept;
			static size_t write_callback(void *contents, size_t size, size_t nmemb, Worker *worker) noexcept;
			static int trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Worker *worker) noexcept;
			static curl_socket_t open_socket_callback(Worker *worker, curlsocktype purpose, struct curl_sockaddr *address) noexcept;
			static int sockopt_callback(Worker *worker, curl_socket_t curlfd, curlsocktype purpose) noexcept;
			static size_t header_callback(char *buffer, size_t size, size_t nitems, Worker *worker) noexcept;

			curl_slist * headers() const noexcept;

			Udjat::String perform();

#else

			#error Cant determine HTTP engine

#endif // _WIN32

		public:

			Worker(const char *url = "", const HTTP::Method method = HTTP::Get, const char *payload = "");
			Worker(const URL &url, const HTTP::Method method = HTTP::Get, const char *payload = "") : Worker(url.c_str(),method,payload) {
			}

			virtual ~Worker();

			Worker & credentials(const char *user, const char *passwd) override;
			Worker & url(const char *url) noexcept;

			inline const URL & url() const noexcept {
				return Protocol::Worker::url();
			}

			String get(const std::function<bool(double current, double total)> &progress) override;
			bool save(const char *filename, const std::function<bool(double current, double total)> &progress) override;

		};

	}

 }
