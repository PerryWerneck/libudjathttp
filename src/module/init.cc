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
 #include <udjat/defs.h>
 #include <udjat/module.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/tools/http/worker.h>

 #ifdef HAVE_CURL
	#include <curl/curl.h>
 #endif // HAVE_CURL

 using namespace std;

 /// @brief Register udjat module.
 UDJAT_API Udjat::Module * udjat_module_init() {

	static const Udjat::ModuleInfo moduleinfo{
#if defined(_WIN32)
		"WinHTTP module for " STRINGIZE_VALUE_OF(PRODUCT_NAME), 	// The module description.
#elif defined(HAVE_CURL)
		"CURL " LIBCURL_VERSION " module for " STRINGIZE_VALUE_OF(PRODUCT_NAME), 		// The module description.
#else
		"HTTP module for " STRINGIZE_VALUE_OF(PRODUCT_NAME), 		// The module description.
#endif //
	};

	class Module : public Udjat::Module {
	private:

		class Protocol : public Udjat::Protocol {
		public:
			Protocol(const char *name) : Udjat::Protocol(name,moduleinfo) {
			}

			std::shared_ptr<Worker> WorkerFactory() const override {
				return make_shared<Udjat::HTTP::Worker>();
			}

			/*
			Udjat::String call(const Udjat::URL &url, const Udjat::HTTP::Method method, const char *payload) const override {
				return Udjat::HTTP::Worker(url,method,payload).Udjat::Protocol::Worker::get();
			}

			bool get(const Udjat::URL &url, const char *filename, const std::function<bool(double current, double total)> &progress) const override {
				return Udjat::HTTP::Worker(url).save(filename,progress);
			}
			*/

		};

		Protocol http{"http"};
		Protocol https{"https"};

	public:

		Module() : Udjat::Module("http",moduleinfo) {
		};

		virtual ~Module() {
#ifdef DEBUG
				cout << __FILE__ << "(" << __LINE__ << ")" << endl;
#endif // DEBUG
		}

	};

	return new Module();
 }

