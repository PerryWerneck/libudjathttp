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
 #include <udjat/defs.h>
 #include <string>
 #include <private/worker.h>

#ifdef HAVE_WINHTTP
	#include <winhttp.h>
#endif // HAVE_WINHTTP

#ifdef HAVE_CURL
	#include <curl/curl.h>
#endif // HAVE_CURL

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
