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

 #include <config.h>
 #include <udjat/tools/http/worker.h>
 #include <udjat/tools/http/timestamp.h>
 #include <udjat/tools/http/exception.h>
 #include <private/engine.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/file.h>
 #include <udjat/tools/file/temporary.h>
 #include <iostream>
 #include <sstream>

 #include <sys/types.h>
 #include <sys/stat.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace std;

 namespace Udjat {

	HTTP::Worker & HTTP::Worker::credentials(const char *user, const char *passwd) {
		auth.user = user;
		auth.passwd = passwd;
		return *this;
	}

	String HTTP::Worker::get(const std::function<bool(double current, double total)> &progress) {

		class Engine : public Udjat::HTTP::Engine, public stringstream {
		private:
			const std::function<bool(double current, double total)> &progress;
			double current = 0;
			double total = 0;

		public:
			Engine(HTTP::Worker &worker, const std::function<bool(double current, double total)> &p)
				: Udjat::HTTP::Engine{worker}, progress{p} {
				progress(0.0,0.0);
			}

			void response(const char *name, const char *value) override {
				worker.response(name,value);
			}

			void content_length(unsigned long long length) override {
				total = (double) length;
				progress(current,total);
			}

			void write(const void *contents, size_t length) override {
				stringstream::write((const char *) contents, length);
				current += length;
				progress(current,total);
			}

		};

		Engine engine{*this,progress};
		status_code = engine.perform();
		if(status_code < 200 || status_code > 299) {
#if UDJAT_CHECK_VERSION(1,2,0)
			throw HTTP::Exception(status_code,engine.response_message());
#else
			throw HTTP::Exception(status_code,this->url().c_str(),engine.response_message());
#endif
		}
		return engine.str();

	}

	int HTTP::Worker::test(const std::function<bool(double current, double total)> &progress) noexcept {

		class Engine : public Udjat::HTTP::Engine {
		private:
			const std::function<bool(double current, double total)> &progress;
			double current = 0;
			double total = 0;

		public:
			Engine(HTTP::Worker &worker, const std::function<bool(double current, double total)> &p)
				: Udjat::HTTP::Engine{worker,HTTP::Head}, progress{p} {
				progress(0.0,0.0);
			}

			void response(const char *name, const char *value) override {
				worker.response(name,value);
			}

			void content_length(unsigned long long length) override {
				total = (double) length;
				debug(__FUNCTION__,"(",length,")");
				progress(current,total);
			}

			void write(const void *, size_t length) override {
				current += length;
				progress(current,total);
			}

		};

		try {

			status_code = Engine{*this,progress}.perform(false);
			return (int) status_code;

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("http");

		} catch(...) {

			Logger::String{"Unexpected error"}.error("http");
		}

		return -1;

	}

	void HTTP::Worker::response(const char *name, const char *value) {

		debug(name,"=",value);

		for(auto &header : headers.response) {
			if(header == name) {
				header.Protocol::Header::assign(value);
				return;
			}
		}

		headers.response.emplace_back(name);
		headers.response.back().Protocol::Header::assign(value);

	}

#if UDJAT_CHECK_VERSION(1,2,0)
#else
	Protocol::Header & HTTP::Worker::header(const char *name) {

		for(auto &header : headers.request) {
			if(header == name) {
				return header;
			}
		}

		headers.request.emplace_back(name);
		return headers.request.back();

	}
#endif
	Protocol::Header & HTTP::Worker::request(const char *name) {

		for(auto &header : headers.request) {
			if(header == name) {
				return header;
			}
		}

		headers.request.emplace_back(name);
		return headers.request.back();

	}

	const Protocol::Header & HTTP::Worker::response(const char *name) {

		for(auto &header : headers.response) {
			if(header == name) {
				return header;
			}
		}

		headers.response.emplace_back(name);
		return headers.response.back();

	}

	bool HTTP::Worker::get(const char *filename) {
		return get(filename,[](double,double){
			return true;
		});
	}

	bool HTTP::Worker::get(const char *filename,const std::function<bool(double current, double total)> &progress) {
		File::Handler file{filename};
		return save(file,progress);
	}

	bool HTTP::Worker::save(File::Handler &file, const std::function<bool(double current, double total)> &progress) {

		class Engine : public Udjat::HTTP::Engine {
		private:
			const std::function<bool(double current, double total)> &progress;
			File::Handler &file;
			double current = 0;
			double total = 0;

		public:
			Engine(HTTP::Worker &worker, File::Handler &f, const std::function<bool(double current, double total)> &p)
				: Udjat::HTTP::Engine{worker}, progress{p}, file{f} {
				progress(0.0,0.0);
			}

			void response(const char *name, const char *value) override {
				worker.response(name,value);
			}

			void content_length(unsigned long long length) override {
				debug("Setting file '",((int) file),"' size to ",length," bytes");
				total = (double) length;
				if(length) {
					try {
						file.allocate(length);
					} catch(const std::system_error &e) {
						if(e.code().value() == ENOSPC) {
							throw;
						} else {
							Logger::String{"Error '",e.code().value(),"' setting file size to ",length," bytes: ",e.what()," (fd=",(int) file,")"}.warning("curl");
						}
					} catch(const std::exception &e) {
						Logger::String{"Error setting file size to ",length," bytes: ",e.what()," (fd=",(int) file,")"}.warning("curl");
					}
				}
				progress(current,total);
			}

			void write(const void *contents, size_t length) override {
				debug("Saving ",current,"/",total);
				file.write(current,contents,length);
				current += length;
				progress(current,total);
			}

		};

		// Check timestamp of last file modification.
#if UDJAT_CHECK_VERSION(1,2,0)
		{
			time_t mtime = file.mtime();
			if(mtime) {
				request("If-Modified-Since") = HTTP::TimeStamp(mtime).to_string();
				debug("modified-time=",request("If-Modified-Since").c_str());
			}
		}
#endif

		debug("Saving file");
		status_code = Engine{*this, file, progress}.perform(true);
		debug("rc=",status_code);

		if(status_code >= 200 && status_code <= 299) {
			return true;
		}

		return false;
	}

	bool HTTP::Worker::save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) {

		// Get file to temporary; rename if got contents return false if not.
		{
			File::Temporary handler{filename};
			if(!save(handler,progress)) {
				debug("Not saved, returning false");
				return false;
			}
			Logger::String{"Updating ",filename}.trace("http");
			handler.save(replace);
		}

		// Set file modification time according to the host's modification time.
#if UDJAT_CHECK_VERSION(1,2,0)
		HTTP::TimeStamp timestamp{response("Last-Modified").value()};
		debug("timestamp=",timestamp.to_string());
		if(timestamp) {
			Logger::String{"Timestamp of ",filename," set to ",timestamp.to_string()}.trace("http");
			File::mtime(filename,(time_t) timestamp);
		}
#endif

		return true;

	}

	void HTTP::Worker::save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer) {

		class Engine : public Udjat::HTTP::Engine {
		private:
			const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer;
			double current = 0;
			double total = 0;

		public:
			Engine(HTTP::Worker &worker, const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &w)
				: Udjat::HTTP::Engine{worker}, writer{w} {
			}

			void content_length(unsigned long long length) override {
				total = (double) length;
			}

			void response(const char *, const char *) override {
			}

			void write(const void *contents, size_t length) override {
				writer(current,total,contents,length);
				current += length;
			}

		};

		status_code = Engine{*this, writer}.perform(true);

	}

 }

