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

	class Writer {
	private:

		static size_t do_write(void *contents, size_t size, size_t nmemb, Writer *writer) noexcept {

			size_t length = size * nmemb;

			if(!writer->total) {
				double total;
				curl_easy_getinfo(writer->hCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &total);
				writer->total = (unsigned long long) total;
			}

			writer->write(contents,length);
			return length;

		}

	public:
		CURL *hCurl = nullptr;

		unsigned long long current = 0;
		unsigned long long total = 0;

		constexpr Writer(CURL *c) : hCurl{c} {
		};

		void get(const HTTP::Worker &worker, curl_slist *chunk) {

			curl_easy_setopt(hCurl, CURLOPT_URL, worker.url().c_str());
			curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, this);
			curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, do_write);

			if(chunk) {
				curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);
			}

			CURLcode res = curl_easy_perform(hCurl);

			curl_slist_free_all(chunk);

			if(res != CURLE_OK) {
				if(res == CURLE_OPERATION_TIMEDOUT) {
					throw system_error(ETIMEDOUT,system_category(),worker.url().c_str());
				}
				throw HTTP::Exception(worker.url().c_str(),curl_easy_strerror(res));
			}

		}

		virtual void write(const void *contents, size_t length) = 0;

	};

	void HTTP::Worker::save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &call) {

		class CustomWriter : public Writer {
		private:
			const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &call;

		public:
			constexpr CustomWriter(CURL *c, const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &w) : Writer{c}, call{w} {
			}

			void write(const void *contents, size_t length) override {
				call(current,total,contents,length);
				current += length;
			}

		};

		CustomWriter{hCurl,call}.get(*this,headers());

	}

	bool HTTP::Worker::save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) {

		class FileWriter : public Writer, public File::Temporary {
		public:
			const std::function<bool(double current, double total)> &progress;

			FileWriter(CURL *c, const char *filename, const std::function<bool(double current, double total)> &p) : Writer{c}, File::Temporary{filename}, progress{p} {
				progress(0,0);
			}

			virtual ~FileWriter() {
				progress(current,total);
			}

			void write(const void *contents, size_t length) override {
				File::Temporary::write(contents,length);
				this->current += length;
				progress(this->current,this->total);
			}

		};

		struct stat st;
		if(stat(filename,&st) == 0 && st.st_blocks) {

			// Got stat and the file is not empty, send modification time to host.
			Logger::String{filename," was updated on ",TimeStamp{st.st_mtime}.to_string()}.trace("curl");
			header("If-Modified-Since").assign(HTTP::TimeStamp{st.st_mtime}.to_string());

		}

		FileWriter file{hCurl,filename,progress};

		file.get(*this,headers());

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		Logger::String log{"Server response was ",response_code};
		if(!message.empty()) {
			log.append(" (",message,")");
		}

		if(response_code >= 200 && response_code <= 299) {

			log.append(" updating '",filename,"'");
			log.write(Logger::Trace,"curl");
			file.save(filename,replace);

		} else if(response_code == 304) {

			log.append(" keeping '",filename,"'");
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
