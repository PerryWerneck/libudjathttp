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
  * @brief Brief description of this source.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <private/writer.h>
 #include <udjat/tools/logger.h>
 #include <curl/curl.h>

 using namespace Udjat;
 using namespace std;

 namespace Udjat {

	size_t HTTP::Writer::do_write(void *contents, size_t size, size_t nmemb, Writer *writer) noexcept {

		size_t length = size * nmemb;

		try {

			if(!writer->total && length) {
				writer->total = writer->worker.size();
				if(writer->total) {
					Logger::String{"Preallocating ",String{}.set_byte(writer->total)}.write(Logger::Debug,"curl");
					writer->allocate(writer->total);
				}
			}

			writer->write(contents,length);

		} catch(const std::system_error &e) {

			writer->error_message = e.what();
			cerr << "curl\t" << writer->error_message << endl;
			errno = e.code().value();
			return 0;

		} catch(const std::exception &e) {

			writer->error_message = e.what();
			cerr << "curl\t" << writer->error_message << endl;
			return 0;

		} catch(...) {

			writer->error_message = "Unexpected error";

		}

		return length;

	}

	void HTTP::Writer::get(curl_slist *chunk) {

		debug("writer::",__FUNCTION__);

		curl_easy_setopt(hCurl, CURLOPT_URL, worker.url().c_str());
		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, do_write);

		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}

		CURLcode res = curl_easy_perform(hCurl);

		curl_slist_free_all(chunk);

		if(!error_message.empty()) {
			Logger::String{curl_easy_strerror(res)," (",res,")"}.error("curl");
			throw runtime_error(error_message);
		}

		if(res != CURLE_OK) {
			if(res == CURLE_OPERATION_TIMEDOUT) {
				throw system_error(ETIMEDOUT,system_category(),worker.url().c_str());
			}
			throw HTTP::Exception(worker.url().c_str(),curl_easy_strerror(res));
		}

	}

	int HTTP::Writer::response_code() {
		long rc = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &rc);
		return (int) rc;
	}

 }
