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

 #include <config.h>
 #include <private/worker.h>
 #include <udjat/tools/http/timestamp.h>
 #include <iostream>

 using namespace std;

 namespace Udjat {

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

