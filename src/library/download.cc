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
 #include <udjat/tools/http/timestamp.h>
 #include <sys/types.h>
 #include <sys/stat.h>

 #ifndef _WIN32
	#include <unistd.h>
 #endif // _WIN32


 namespace Udjat {

	bool HTTP::Client::get(const char *filename, const std::function<bool(double current, double total)> &progress, const char *config) {

		struct stat st;
		bool updated = false;

		if(stat(filename,&st) < 0) {

			if(errno != ENOENT) {
				throw system_error(errno,system_category(),Logger::Message("Can't stat '{}'",filename));
			}

			memset(&st,0,sizeof(st));

			cout << "http\tDownloading '" << filename << "'" << endl;
		}

#ifdef DEBUG
		cout << "Last modification time: " << HTTP::TimeStamp(st.st_mtime) << endl;
#endif // DEBUG

		Worker * worker = Worker::getInstance(this);

		try {

			updated = worker->get(filename,st.st_mtime,config,progress);

		} catch(const std::exception &e) {

			cerr << "http\t" << url << ": " << e.what() << endl;
			delete worker;
			throw;

		} catch(...) {

			cerr << "http\t" << url << ": Unexpected error" << endl;
			delete worker;
			throw;

		}

		delete worker;

		return updated;

	}

 }
