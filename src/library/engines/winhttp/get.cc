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
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>
 #include <udjat/tools/file.h>
 #include <udjat/tools/http/worker.h>
 #include <udjat/tools/logger.h>
 #include <udjat/win32/charset.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <libgen.h>
 #include <utime.h>

 namespace Udjat {

	bool HTTP::Worker::save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace)  {

		progress(0,0);

		//
		// Open connection
		//
		INTERNET_HANDLE	connection = connect();
		INTERNET_HANDLE	request = open(connection);

		//
		// Do request
		//
		send(request);

		if(!WinHttpReceiveResponse(request, NULL)) {
			throw Win32::Exception("Error receiving response");
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
					throw Win32::Exception("Cant get HTTP status code");
				}
		}

		debug("The status code was ",dwStatusCode);

		Logger::String log{"Server response was ",dwStatusCode};

		if(dwStatusCode == 304) {
			//
			// Not modified
			//
			log.append(", keeping '",filename,"'");
			log.write(Logger::Trace,"winhttp");
			return false;
		}

		if(dwStatusCode < 200 || dwStatusCode > 299 ) {
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

			log.append(" (",text,")");
			log.write(Logger::Trace,"winhttp");

			throw HTTP::Exception((unsigned int) dwStatusCode, url().c_str(), text);
		}

		log.append(" updating '",filename,"'");
		log.write(Logger::Trace,"winhttp");

		//
		// Get file length
		//
		double total = 0;
		{
			DWORD fileLength = 0;
			DWORD size = sizeof(DWORD);
			if( WinHttpQueryHeaders( request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER , NULL, &fileLength, &size, NULL ) == TRUE ) {
				total = (double) fileLength;
			}
			progress(0,total);
		}

		//
		// Download file.
		//
		File::Temporary tempfile(filename);

		char buffer[4096] = {0};
		DWORD length = 0;
		double current = 0;

		while(WinHttpReadData(request, buffer, sizeof(buffer), &length) && length > 0) {
			tempfile.write((void *) buffer,(size_t) length);
			current += length;
			progress(current,total);
			length = 0;
		}

		tempfile.save(replace);
		return true;
	}

 }
 */

