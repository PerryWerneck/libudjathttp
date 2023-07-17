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

 #include <udjat/tools/systemservice.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/http/client.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/agent.h>
 #include <udjat/factory.h>
 #include <udjat/module.h>
 #include <udjat/tools/http/worker.h>
 #include <iostream>
 #include <memory>

 using namespace std;
 using namespace Udjat;

//---[ Implement ]------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	Udjat::Quark::init();
	Udjat::Logger::redirect();
	Udjat::Logger::enable(Udjat::Logger::Trace);
	Udjat::Logger::enable(Udjat::Logger::Debug);
	Udjat::Logger::console(true);

	cout << HTTP::Worker{"http://localhost"}.get([](double, double){ return true; });

	Udjat::Module::unload();

	return 0;

}
