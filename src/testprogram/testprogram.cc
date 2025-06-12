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
 #include <udjat/tests.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/module.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/actions/http.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/url/handler/http.h>
 
 using namespace std;
 using namespace Udjat;

 int main(int argc, char **argv) {


	static const ModuleInfo info{"url-tester"};
	
	return Testing::run(argc,argv,info,[](Application &){

	 	udjat_module_init();

		cout << "---[ Client tests begin ]------------------------------" << endl;
		
		// Udjat::URL url{"http://127.0.0.1/udjat/css/style.css"};
		const char *env = getenv("UDJAT_URL");
		if(!env) {
			env = "http://127.0.0.1/udjat/css/style.css";
		}

		{
			Udjat::URL url{env};
			auto handler = HTTP::Handler::Factory{"http"}.HandlerFactory(url);
	
			debug("URL name = ",url.name().c_str());
			auto response = handler->get(String{"/tmp/",url.name()}.c_str());
	
			cout << "-----" << endl << response << endl << "-----" << endl;
		
		}

		{
			cout << endl;

			auto filename = Udjat::URL{env}.tempfile([](double current, double total){
				return false;
			});
	
			cout << "Filename = " << filename << endl;
		}


		cout << "---[ Client tests complete ]----------------------------" << endl;

	});

 }

