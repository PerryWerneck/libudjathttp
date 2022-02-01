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

 #include <internals.h>
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>

 namespace Udjat {

	void HTTP::Client::Worker::get(const char *filename, time_t timestamp) {

		struct curl_slist *chunk = NULL;

		if(timestamp) {
			string hdr{"If-Modified-Since: "};
			hdr += HTTP::TimeStamp(timestamp).to_string();
			hdr += ";";
#ifdef DEBUG
			cout << hdr << endl;
#endif // DEBUG
			chunk = curl_slist_append(chunk, hdr.c_str());
		}

		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}

		CURLcode res = curl_easy_perform(hCurl);

		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {
#ifdef DEBUG
			cout << "CURL-Error=" << res << endl;
#endif // DEBUG
			throw HTTP::Exception(this->client->url.c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

#ifdef DEBUG
		cout << "Response code: " << response_code << endl;
#endif // DEBUG


	}


 }

