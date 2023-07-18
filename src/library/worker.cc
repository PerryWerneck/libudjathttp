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
 #include <private/engine.h>
 #include <udjat/tools/logger.h>
 #include <iostream>
 #include <sstream>

 using namespace std;

 namespace Udjat {

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

			void content_length(unsigned long long length) override {
				total = (double) length;
				progress(current,total);
			}

			void write(const void *contents, size_t length) override {
				stringstream::write((const char *) contents, length);
				current += length;
				progress(current,total);
			}

			Engine & perform() {
				Udjat::HTTP::Engine::perform(true);
				return *this;
			}

		};

		return Engine{*this,progress}.perform().str();

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

			return Engine{*this,progress}.perform(false);

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("http");

		} catch(...) {

			Logger::String{"Unexpected error"}.error("http");
		}

		return -1;

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

			void content_length(unsigned long long length) override {
				total = (double) length;
				debug(__FUNCTION__,"(",length,")");
				progress(current,total);
			}

			void write(const void *contents, size_t length) override {
				file.write(contents,length);
				current += length;
				debug("Saving ",current,"/",total);
				progress(current,total);
			}

		};

		debug("Saving file");
		int rc = Engine{*this, file, progress}.perform(true);
		debug("rc=",rc);

		if(rc >= 200 && rc <= 299) {
			return true;
		}

		return false;
	}

	/*
	Protocol::Header & HTTP::Worker::Header::assign(const Udjat::TimeStamp &value) {
		std::string::assign(HTTP::TimeStamp((time_t) value).to_string());
		return *this;
	}

	Protocol::Header & HTTP::Worker::header(const char *name) {

		for(Header &header : headerlist) {
			if(header == name) {
				return header;
			}
		}

		headerlist.emplace_back(name);
		return headerlist.back();

	}
	*/

 }

