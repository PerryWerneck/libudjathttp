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
 #include <udjat/tools/file.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <libgen.h>
 #include <utime.h>

 namespace Udjat {

	static size_t write_file(void *contents, size_t size, size_t nmemb, File::Temporary *tempfile) noexcept {

		size_t length = size * nmemb;
		tempfile->write(contents,length);
		return length;

	}

	bool HTTP::Client::Worker::get(const char *filename, time_t timestamp, const char *config) {

		File::Temporary tempfile(filename);

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, &tempfile);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_file);

		struct curl_slist *chunk = NULL;

		// Set headers
		Config::for_each(
			( (config && *config) ? config : "curl-get-file-headers"),
			[&chunk](const char *key, const char *value) {
#ifdef DEBUG
				cout << "http-header\t" << key << "='" << value << "'" << endl;
#endif // DEBUG
				chunk = curl_slist_append(chunk,(string(key) + ":" + value).c_str());
				return true;
			}
		);

		if(timestamp) {

			if(!chunk) {
				clog << "http\tWarning: No headers in request for '" << filename << "', using defaults" << endl;
				chunk = curl_slist_append(chunk, "Cache-Control:public, max-age=31536000");
			}

			string hdr{"If-Modified-Since:"};
			hdr += HTTP::TimeStamp(timestamp).to_string();
			hdr += ";";
#ifdef DEBUG
			cout << "http\t" << hdr << endl;
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
			cout << "http\tCURL-Error=" << res << endl;
#endif // DEBUG
			throw HTTP::Exception(this->client->url.c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		if(response_code == 200) {

			tempfile.link(filename);

		} else if(response_code == 304) {

			cout << "http\tUsing local '" << filename << "' (not modified)" << endl;
			return false;

		} else if(message.empty()) {

			throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str());

		} else {

			throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str(), message.c_str());

		}

		if(this->timestamp) {

			utimbuf ub;
			ub.actime = time(0);
			ub.modtime = (time_t) this->timestamp;

			if(utime(filename,&ub) == -1) {
				cerr << "http\tError '" << strerror(errno) << "' setting file timestamp" << endl;
			} else {
				cout << "http\t'" << filename << "' time set to " << this->timestamp << endl;
			}

		}

		return true;

	}


 }

