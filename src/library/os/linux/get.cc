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
 #include <udjat/tools/http/worker.h>
 #include <udjat/tools/file.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <libgen.h>
 #include <utime.h>
 #include <functional>

 namespace Udjat {

 	class Writer : public File::Temporary {
	public:
		const std::function<bool(double current, double total)> &progress;
		double current = 0;

		CURL *curl = nullptr;
		double total = 0;

		Writer(CURL *c, const char *filename, const std::function<bool(double, double)> &p ) : Temporary(filename), progress(p), curl(c) {
			progress(0,0);
		}

		virtual ~Writer() {
			progress(current,total);
		}

		void write(const void *contents, size_t length) {
			File::Temporary::write(contents,length);
			this->current += length;
			progress(this->current,this->total);
		}

 	};

	static size_t write_file(void *contents, size_t size, size_t nmemb, Writer *writer) noexcept {

		size_t length = size * nmemb;

		if(!writer->total) {
			curl_easy_getinfo(writer->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &writer->total);
		}

		writer->write(contents,length);
		return length;

	}

	bool HTTP::Worker::save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) {

		Writer tempfile(hCurl, filename, progress);

		curl_easy_setopt(hCurl, CURLOPT_URL, url().c_str());
		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, &tempfile);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_file);

		struct curl_slist *chunk = this->headers();
		if(chunk) {
			curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
		}

		CURLcode res = curl_easy_perform(hCurl);

		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {

			if(res == CURLE_OPERATION_TIMEDOUT) {
				throw system_error(ETIMEDOUT,system_category());
			}

			throw HTTP::Exception(this->url().c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		Logger::String log{"Server response was ",response_code};
		if(!message.empty()) {
			log.add(" (",message,") ");
		}

		if(response_code >= 200 && response_code <= 299) {

			log.add("updating '",filename,"'");
			log.write(Logger::Trace,"curl");
			tempfile.save(filename,replace);

		} else if(response_code == 304) {

			log.add("keeping '",filename,"'");
			log.write(Logger::Trace,"curl");

			return false;

		} else if(message.empty()) {

			log.write(Logger::Trace,"curl");
			throw HTTP::Exception((unsigned int) response_code, this->url().c_str());

		} else {

			log.write(Logger::Trace,"curl");
			throw HTTP::Exception((unsigned int) response_code, this->url().c_str(), message.c_str());

		}

		if(in.modification) {

			utimbuf ub;
			ub.actime = time(0);
			ub.modtime = (time_t) in.modification;

			if(utime(filename,&ub) == -1) {
				cerr << "http\tError '" << strerror(errno) << "' setting '" << filename << "' timestamp" << endl;
			} else if(Logger::enabled(Logger::Trace)) {
				Logger::String{"Time of '",filename,"' set to ",std::to_string(in.modification).c_str()}.trace("http");
			}

		} else {

			clog << "http\tNo modification time header in response for " << this->url() << endl;

		}

		return true;

	}

 }
