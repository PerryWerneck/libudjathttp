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

#ifdef _WIN32
#include "os/windows/base64.h"
#endif // _WIN32

namespace Udjat {

	HTTP::Client::Client(const char *u) : url(u) {
	}

	HTTP::Client::~Client() {
	}

	Udjat::String HTTP::Client::get() {

		Udjat::String response;
		Worker * worker = Worker::getInstance(this);

		try {

			response = worker->call("GET");

		} catch(const std::exception &e) {

			cerr << "http\t" << url << " - " << e.what() << endl;
			delete worker;
			throw;

		} catch(...) {

			cerr << "http\t" << url << " - Unexpected error" << endl;
			delete worker;
			throw;

		}

		delete worker;
		return response;

	}

	Udjat::String HTTP::Client::post(const char *payload) {

		Udjat::String response;
		Worker * worker = Worker::getInstance(this);

		try {

			response = worker->call("POST",payload);

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
		return response;

	}

	void HTTP::Client::set(const HTTP::Client::Header &header) {
		headers.push_back(header);
		throw system_error(ENOTSUP,system_category(),"Not implemented");
	}

	HTTP::Client & HTTP::Client::setCredentials(const char *username, const char *password) {
#ifdef _WIN32
		// Encode authentication
		// When the user agent wants to send authentication credentials to the server, it may use the Authorization field.
		//
		// The Authorization field is constructed as follows:
		//
		// The username and password are combined with a single colon (:). This means that the username itself cannot contain a colon.
		// The resulting string is encoded into an octet sequence. The character set to use for this encoding is by default unspecified, as long as it is compatible with US-ASCII, but the server may suggest use of UTF-8 by sending the charset parameter.[8]
		// The resulting string is encoded using a variant of Base64.
		// The authorization method and a space (e.g. "Basic ") is then prepended to the encoded string.
		// For example, if the browser uses Aladdin as the username and OpenSesame as the password, then the field's value is the Base64 encoding of Aladdin:OpenSesame, or QWxhZGRpbjpPcGVuU2VzYW1l. Then the Authorization header will appear as:
		//
		// Authorization: Basic QWxhZGRpbjpPcGVuU2VzYW1l
		//
		char text[4096];
		memset(text,0,4096);

		strncpy(text,username,4095);
		strncat(text,":",4095);
		strncat(text,password,4095);

		string auth{"Basic "};
		auth += base64::encode((const unsigned char *) text, strlen(text));

		headers.push_back(Header{"Authorization",auth.c_str()});

#else
		credentials.username = username;
		credentials.password = password;
#endif // _WIN32
		return *this;
	}

}
