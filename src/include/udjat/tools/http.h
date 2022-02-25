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

 /*
 #include <udjat/defs.h>
 #include <udjat/tools/http/exception.h>
 #include <udjat/tools/method.h>
 #include <udjat/tools/string.h>
 #include <string>
 #include <vector>
 #include <system_error>
 #include <functional>

 namespace Udjat {

	namespace HTTP {

		/// @brief Generic HTTP client component.
		class UDJAT_API Client {
		public:
			struct Header {
				std::string name;
				std::string value;

				Header(const char *n, const char *v) : name(n), value(v) {
				}

				template<typename T>
				Header(const char *n, const T &v) : name(n), value(std::to_string(v)) {
				}
			};

		private:

			class Worker;
			friend class Worker;

			/// @brief URL.
			std::string url;

			/// @brief HTTP headers.
			std::vector<Header> headers;

#ifndef _WIN32
			struct {
				std::string username;
				std::string password;
			} credentials;
#endif // !_WIN32

		public:
			Client(const Client &src) = delete;
			Client(const Client *src) = delete;

			Client(const char *url);
			Client(const std::string &url) : Client(url.c_str()) { };

			~Client();

			void set(const Header &header);

			Client & setCredentials(const char *username, const char *password);

			Udjat::String get();
			Udjat::String post(const char *payload);

			/// @brief Download/update a file.
			/// @param filename The fullpath for the file.
			/// @param progress Callback for the download progress.
			/// @param config Name of the configuration section used to get the http headers.
			/// @return true if the file was updated.
			bool get(const char *filename, const std::function<bool(double current, double total)> &progress, const char *config = nullptr);

		};

	}

 }
	*/
