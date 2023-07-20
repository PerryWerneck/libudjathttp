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
  * @brief Common methods for http engine.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <private/engine.h>
 #include <udjat/tools/logger.h>

 using namespace std;

 namespace Udjat {

	void HTTP::Engine::content_length(unsigned long long) {
	}

	int HTTP::Engine::check_result(int status_code, bool except) {

		if(status_code >= 200 && status_code <= 299) {
			return status_code;
		}

		switch(status_code) {
		case 304:	// Not modified.
			Logger::String{worker.url().c_str()," was not modified"}.trace("curl");
			return 304;

		default:

			if(except) {
				if(message.empty()) {
					throw HTTP::Exception((unsigned int) status_code, worker.url().c_str());
				} else {
					throw HTTP::Exception((unsigned int) status_code, worker.url().c_str(), message.c_str());
				}
			}

		}

		return status_code;

	}

 }



