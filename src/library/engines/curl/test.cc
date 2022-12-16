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

	static size_t dummy_write(void UDJAT_UNUSED(*contents), size_t size, size_t nmemb, void UDJAT_UNUSED(*dunno)) noexcept {
		return size * nmemb;
	}

	int HTTP::Worker::test(const std::function<bool(double current, double total)> UDJAT_UNUSED(&progress)) noexcept {

		switch(method()) {
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
			cerr << "Invalid or unsupported http verb" << endl;
			return EINVAL;
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

		curl_easy_setopt(hCurl, CURLOPT_URL, url().c_str());

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, dummy_write);

		struct curl_slist *chunk = headers();
		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}
		CURLcode res = curl_easy_perform(hCurl);
		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {
			cerr << url().c_str() << ": " << curl_easy_strerror(res);
			return 500;
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		return (int) response_code;

	}

 #endif // HAVE_CURL
 }
