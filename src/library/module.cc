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
 #include <udjat/moduleinfo.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/url.h>
 #include <udjat/module/http.h>
 #include <private/handler.h>

 #if defined(HAVE_CURL)
	#include <curl/curl.h>
 #endif // HAVE_CURL

 namespace Udjat {

       static const Udjat::ModuleInfo moduleinfo{
#if defined(_WIN32)
			"WinHTTP module for " STRINGIZE_VALUE_OF(PRODUCT_NAME),         // The module description.
#elif defined(HAVE_CURL)
			"CURL " LIBCURL_VERSION " module for " STRINGIZE_VALUE_OF(PRODUCT_NAME),                // The module description.
#else
			"HTTP module for " STRINGIZE_VALUE_OF(PRODUCT_NAME),            // The module description.
#endif //
       };

	Udjat::Module * HTTP::Module::Factory(const char *name) {

		class Module : public HTTP::Module {
		private:

			class Factory : public Udjat::HTTP::Handler::Factory {
			public:
				Factory(const char *name) : Udjat::HTTP::Handler::Factory{name} {
				}

				virtual ~Factory() {
				}

				std::shared_ptr<Udjat::URL::Handler> HandlerFactory(const URL &url) const override {
					return std::make_shared<HTTP::Handler>(url);
				}
			};

			Factory http{"http"};
			Factory https{"https"};

#if defined(HAVE_CURL)
			Factory def{"default"};
#endif // HAVE_CURL

		public:
			Module(const char *name) 
				: HTTP::Module{name} {
			}

			virtual ~Module() {
			}

		};

		return new Module(name);

	}

	HTTP::Module::Module(const char *name) : Udjat::Module(name,moduleinfo) {
	}

	HTTP::Module::~Module() {
	}

 }

