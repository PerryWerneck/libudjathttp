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

namespace Udjat {

	HTTP::Client::Client(const char *u) : url(u) {
	}

	HTTP::Client::~Client() {
	}

	std::string HTTP::Client::get() {

		string response;
		Worker * worker = Worker::getInstance(this);

		try {

			response = worker->call("GET");

		} catch(...) {

			delete worker;
			throw;

		}

		delete worker;
		return response;

	}

	std::string HTTP::Client::post(const char *payload) {

		string response;
		Worker * worker = Worker::getInstance(this);

		try {

			response = worker->call("POST",payload);

		} catch(...) {

			delete worker;
			throw;

		}

		delete worker;
		return response;

	}

	void HTTP::Client::set(const HTTP::Client::Header &header) {
		headers.push_back(header);
		throw system_error(ENOTSUP,system_category(),"Not implemented");
	}


}
