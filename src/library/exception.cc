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
 namespace Udjat {

	static const struct {
		unsigned int http;
		int syscode;
	} syscodes[] = {
		{ 401, EPERM 		},
		{ 403, EPERM 		},
		{ 404, ENOENT		},
		{ 405, EINVAL		},
		{ 407, EPERM	 	},
//		{ 407, ETIMEOUT 	},
		{ 501, ENOTSUP		},
//		{ 504, ETIMEOUT 	},
	};

	static int toSysError(unsigned int http) {

		for(size_t ix = 0; ix < (sizeof(syscodes)/sizeof(syscodes[0])); ix++) {
			if(syscodes[ix].http == http) {
				return syscodes[ix].syscode;
			}
		}

		return -1;
	}

	static string toSysMessage(unsigned int http) {

		for(size_t ix = 0; ix < (sizeof(syscodes)/sizeof(syscodes[0])); ix++) {
			if(syscodes[ix].http == http) {
				return string(strerror(syscodes[ix].syscode));
			}
		}

		return string{"HTTP error " + to_string(http)};
	}

	HTTP::Exception::Exception(const char *u, const char *message) : runtime_error(message), url(u) {
#ifdef DEBUG
		cerr << "Exception(" << url << "): " << message << endl;
#endif // DEBUG
	}

	HTTP::Exception::Exception(unsigned int code, const char *url, const char *message) : Exception(url,message) {
		codes.http = code;
		codes.system = error_code(toSysError(code),system_category());
	}

	HTTP::Exception::Exception(unsigned int code, const char *url) : Exception(code,url,toSysMessage(code).c_str()) {
	}

 }

 */
