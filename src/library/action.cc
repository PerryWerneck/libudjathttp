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

 /**
  * @brief Implements HTTP based action.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <curl/curl.h>
 #include <udjat/tools/actions/abstract.h>
 #include <udjat/tools/actions/http.h>
 #include <udjat/tools/url.h>
 #include <memory>

 using namespace std;
 
 namespace Udjat {

	std::shared_ptr<Udjat::Action> HTTP::Action::Factory::ActionFactory(const XML::Node &node) const {
		return make_shared<HTTP::Action>(node);
	}

	HTTP::Action::Action(const XML::Node &node) 
		: 	Udjat::Action{node}, 
			url{node,"url",true},
			method{HTTP::MethodFactory(node,"get")},
			payload{super::payload(node)}, 
			mimetype{MimeTypeFactory(String{node,"payload-format","json"}.c_str())} {
	}

	int HTTP::Action::call(Udjat::Request &request, Udjat::Response &response, bool except) {
		return Udjat::Action::exec(response,except,[&]() {

			String payload{this->payload};
			payload.expand(request);

			if(!url.get(response,method,payload.c_str())) {
				return -1;
			}

			return 0;

		});
	}

 }