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
 #include <udjat/tools/file.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <libgen.h>
 #include <utime.h>

 namespace Udjat {

	bool HTTP::Client::Worker::get(const char *filename, time_t timestamp, const char *config) {

		INTERNET_HANDLE	connection = connect();
		INTERNET_HANDLE	request = open(connection,"GET");

		send(request);

		if(!WinHttpReceiveResponse(request, NULL)) {
			throw Win32::Exception(this->client->url + ": Error receiving response");
		}

		//
		// Get status code
		//
		DWORD dwStatusCode = 0;
		{
			DWORD dwSize = sizeof(DWORD);

			if(!WinHttpQueryHeaders(
					request,
					WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&dwStatusCode,
					&dwSize,
					WINHTTP_NO_HEADER_INDEX
				)) {
					throw Udjat::Win32::Exception("Cant get HTTP status code");
				}
		}

#ifdef DEBUG
			cout << "The status code was " << dwStatusCode << endl;
#endif // DEBUG

		if(dwStatusCode != 304) {
			//
			// Not modified
			//
			cout << "http\tUsing local '" << filename << "' (not modified)" << endl;
			return false;
		}

		if(dwStatusCode != 200) {
			//
			// Failed.
			//
			wchar_t buffer[1024];
			DWORD dwSize = 1023;

			ZeroMemory(buffer, sizeof(buffer));

			WinHttpQueryHeaders(
				request,
				WINHTTP_QUERY_STATUS_TEXT,
				WINHTTP_HEADER_NAME_BY_INDEX,
				buffer,
				&dwSize,
				WINHTTP_NO_HEADER_INDEX
			);

			char text[1024];
			ZeroMemory(text, sizeof(text));
			wcstombs(text, buffer, 1023);

			throw Udjat::HTTP::Exception((unsigned int) dwStatusCode, this->client->url.c_str(), text);
		}

		//
		// Download file.
		//
		File::Temporary tempfile(filename);

		char buffer[4096] = {0};
		DWORD length = 0;

		while(WinHttpReadData(request, buffer, sizeof(buffer), &length) && length > 0) {
			tempfile.write((void *) buffer,(size_t) length);
			length = 0;
		}

		tempfile.save();
		return true;
	}


 }

