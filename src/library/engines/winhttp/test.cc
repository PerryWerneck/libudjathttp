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
 #include <internals.h>
 #include <private/worker.h>
 #include <udjat/tools/logger.h>

 using namespace std;

 namespace Udjat {

	int HTTP::Worker::test(const std::function<bool(double current, double total)> UDJAT_UNUSED(&progress)) noexcept {

		progress(0,0);

		//
		// Open connection
		//
		INTERNET_HANDLE	connection;
		INTERNET_HANDLE	request;

		try {

			connection = connect();
			request = open(connection,"HEAD");

		} catch(const std::exception &e) {

			cerr << "winhttp\t" << url() << ": " << e.what() << endl;
			return -1;

		} catch(...) {

			cerr << "winhttp\t" << url() << ": Unexpected error" << endl;
			return -1;

		}

		//
		// Do request
		//
		auto rc = WinHttpSendRequest(
					request,
					WINHTTP_NO_ADDITIONAL_HEADERS,
					0,
					WINHTTP_NO_REQUEST_DATA,
					0,
					0,
					0
				);

		if(!rc) {
			return -1;
		}

		if(!WinHttpReceiveResponse(request, NULL)) {
			return -1;
		}

		//
		// Get status code
		//
		DWORD dwStatusCode = 0;
		DWORD dwSize = sizeof(DWORD);

		if(!WinHttpQueryHeaders(
				request,
				WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
				WINHTTP_HEADER_NAME_BY_INDEX,
				&dwStatusCode,
				&dwSize,
				WINHTTP_NO_HEADER_INDEX
			)) {
				return -1;
			}

		return dwStatusCode;

	}

 }
