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
 #include <udjat/tools/http.h>
 #include <udjat/moduleinfo.h>

 using namespace std;

 /// @brief Register udjat module.
 Udjat::Module * udjat_module_init() {

	static const Udjat::ModuleInfo moduleinfo{
#if defined(_WIN32)
		"WinHTTP module for " STRINGIZE_VALUE_OF(PRODUCT_NAME), 	// The module description.
#elif defined(HAVE_CURL)
		"CURL module for " STRINGIZE_VALUE_OF(PRODUCT_NAME), 		// The module description.
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

			Udjat::String call(const Udjat::URL &url, const Udjat::HTTP::Method method, const char *payload) const override {

				switch(method) {
				case Udjat::HTTP::Get:
					return Udjat::HTTP::Client(url).get();

				case Udjat::HTTP::Post:
					return Udjat::HTTP::Client(url).post(payload);

				default:
					throw system_error(ENOTSUP,system_category(),"Unsupported HTTP method");
				}
			}

			bool get(const Udjat::URL &url, const char *filename, const std::function<bool(double current, double total)> &progress) const override {
				return Udjat::HTTP::Client(url).get(filename,progress);
			}

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

