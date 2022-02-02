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

#include <config.h>
#include <udjat/defs.h>
#include <udjat/tools/configuration.h>
#include <udjat/tools/logger.h>
#include <udjat/tools/http.h>
#include <udjat/tools/http/timestamp.h>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <sstream>

#if defined(_WIN32)

	#include <winhttp.h>
	#include <udjat/win32/exception.h>

#elif defined(HAVE_CURL)

	#include <curl/curl.h>

#endif

using namespace std;

namespace Udjat {

	namespace HTTP {

		class Client::Worker {
		private:
			HTTP::Client *client;

			/// @brief The request timestamp.
			HTTP::TimeStamp timestamp;

#if defined(_WIN32)

			/// @brief WinHTTP session handle.
			HINTERNET session;

			/// @brief Connect to HTTP host.
			HINTERNET connect();

			/// @brief Open HTTP Request.
			HINTERNET open(HINTERNET connection, const LPCWSTR pwszVerb);

			/// @brief Send request.
			void send(HINTERNET request, const char *payload = nullptr);

			/// @brief Wait for response.
			std::string wait(HINTERNET req);

#elif defined(HAVE_CURL)

			CURL * hCurl;

			char error[CURL_ERROR_SIZE];

			static size_t read_callback(char *buffer, size_t size, size_t nitems, Worker *session) noexcept;
			static size_t write_callback(void *contents, size_t size, size_t nmemb, Worker *worker) noexcept;
			static int trace_callback(CURL *handle, curl_infotype type, char *data, size_t size, Worker *worker) noexcept;
			static curl_socket_t open_socket_callback(Worker *worker, curlsocktype purpose, struct curl_sockaddr *address) noexcept;
			static int sockopt_callback(Worker *worker, curl_socket_t curlfd, curlsocktype purpose) noexcept;
			static size_t header_callback(char *buffer, size_t size, size_t nitems, Worker *worker) noexcept;

			struct {
				const char *out = nullptr;
				std::ostringstream in;
				std::string payload;
			} buffers;

			std::string message;
			std::string perform();

#else

			#error Cant determine HTTP engine

#endif // _WIN32

			Worker(HTTP::Client *s);

		public:
			Worker(const Worker *s) = delete;
			Worker(const Worker &s) = delete;

			~Worker();

			static Worker * getInstance(HTTP::Client *client);

			std::string call(const char *verb, const char *payload = nullptr);

			void get(const char *filename, time_t timestamp);

		};


	}

}
