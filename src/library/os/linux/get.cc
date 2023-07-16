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

 /*
 #include <internals.h>
 #include <private/writer.h>
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>
 #include <private/worker.h>
 #include <udjat/tools/file.h>
 #include <udjat/tools/string.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <libgen.h>
 #include <utime.h>
 #include <functional>

 #ifndef _GNU_SOURCE
	#define _GNU_SOURCE
 #endif // _GNU_SOURCE
 #include <fcntl.h>

 namespace Udjat {

	void HTTP::Worker::save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &call) {

		class CustomWriter : public HTTP::Writer {
		private:
			const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &call;

		public:
			CustomWriter(const Worker &worker, CURL *c, const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &w) : Writer{worker,c}, call{w} {
			}

			void write(const void *contents, size_t length) override {
				call(current,total,contents,length);
				current += length;
			}

			void allocate(unsigned long long) override {
			}

		};

		CustomWriter writer{*this,hCurl,call};

		writer.get(headers());

		int response_code = writer.response_code();

		if(response_code >= 200 && response_code <= 299) {
			Logger::String{"HTTP response for '", this->url().c_str(), "' was ",response_code}.write(Logger::Debug,"curl");
			return;
		}

		Logger::String{"HTTP response for '", this->url().c_str(), "' was ",response_code}.error("curl");
		throw HTTP::Exception((unsigned int) response_code, this->url().c_str());

	}

	bool HTTP::Worker::save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) {

		class FileWriter : public HTTP::Writer, public File::Temporary {
		public:
			const std::function<bool(double current, double total)> &progress;

			FileWriter(const Worker &worker, CURL *c, const char *filename, const std::function<bool(double current, double total)> &p) : Writer{worker,c}, File::Temporary{filename}, progress{p} {
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

			void allocate(unsigned long long length) override {
				if(fallocate(this->fd,0,0,length) < 0) {
					throw system_error(errno,system_category());
				}
			}

		};

		struct stat st;
		if(stat(filename,&st) == 0 && st.st_blocks) {

			// Got stat and the file is not empty, send modification time to host.
			Logger::String{filename," was updated on ",TimeStamp{st.st_mtime}.to_string()}.trace("curl");
			header("If-Modified-Since").assign(HTTP::TimeStamp{st.st_mtime}.to_string());

		}

		FileWriter file{*this,hCurl,filename,progress};

		file.get(headers());

		int response_code = file.response_code();

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
				Logger::String{"Unable to set timestamp of '",filename,"': ",strerror(errno)," (rc=",errno,")"}.warning("curl");
			} else if(Logger::enabled(Logger::Trace)) {
				Logger::String{"Time of '",filename,"' set to ",std::to_string(in.modification).c_str()}.trace("curl");
			}

		} else {

			Logger::String{"No modification time header in response for ",this->url().c_str()}.warning("curl");

		}

		return true;

	}

 }
 */
