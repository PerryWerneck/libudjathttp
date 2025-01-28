/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2024 Perry Werneck <perry.werneck@gmail.com>
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
 #include <udjat/defs.h>
 #include <udjat/module.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/tools/http/worker.h>
 #include <udjat/module/http.h>
 #include <udjat/tools/logger.h>

 #ifdef HAVE_CURL
	#include <curl/curl.h>
 #endif // HAVE_CURL

 using namespace std;

 namespace Udjat {

#ifdef HAVE_CURL
	#define ALLOW_DEFAULT true
#else
	#define ALLOW_DEFAULT false
#endif

	Udjat::Module * HTTP::Module::Factory(const char *name, const ModuleInfo &info) {

		class Module : public HTTP::Module {
		private:
			std::shared_ptr<Protocol> http, https;

		public:
			Module(const char *name, const ModuleInfo &info) 
				: HTTP::Module{name,info},http{make_shared<Protocol>("http",info,ALLOW_DEFAULT)},https{make_shared<Protocol>("https",info)} {
			}

			virtual ~Module() {
			}

		};

		return new Module(name,info);
	}

	HTTP::Module::Module(const char *name, const ModuleInfo &modinfo) : Udjat::Module(name,modinfo) {
		debug("Loading ",modinfo.description,"...");
	}

	HTTP::Module::~Module() {
	}

 }


