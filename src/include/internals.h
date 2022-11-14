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

#pragma once

#include <config.h>
#include <udjat/defs.h>
#include <udjat/tools/configuration.h>
#include <udjat/tools/logger.h>
#include <udjat/tools/http.h>
#include <udjat/tools/string.h>
#include <udjat/tools/http/timestamp.h>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <sstream>

#ifdef _WIN32
	#include <udjat/win32/exception.h>
#endif // _WIN32

#ifdef HAVE_WINHTTP
	#include <winhttp.h>
#endif // HAVE_WINHTTP

#ifdef HAVE_CURL
	#include <curl/curl.h>
#endif // HAVE_CURL

using namespace std;

namespace Udjat {

#ifdef _WIN32

	#define INTERNET_TEXT wchar_t * __attribute__((cleanup(wchar_t_cleanup)))
	#define INTERNET_HANDLE HINTERNET __attribute__((cleanup(hinternet_t_cleanup)))

	UDJAT_PRIVATE void wchar_t_cleanup(wchar_t **buf);
	UDJAT_PRIVATE void hinternet_t_cleanup(HINTERNET *handle);

#endif // _WIN32

}
