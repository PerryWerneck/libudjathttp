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
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/actions/abstract.h>
 #include <udjat/tools/actions/http.h>
 #include <memory>

 using namespace std;
 
 namespace Udjat {

	std::shared_ptr<Udjat::Action> HTTP::Action::Factory::ActionFactory(const XML::Node &node) const {
		debug("---> Building action");
		return make_shared<HTTP::Action>(node);
	}

	HTTP::Action::Action(const XML::Node &node) 
		: 	Udjat::Action{node}, 
			url{String{node,"url"}.as_quark()},
			method{HTTP::MethodFactory(node,"get")},
			text{payload(node)}, 
			mimetype{MimeTypeFactory(String{node,"payload-format","json"}.c_str())} {

		if(!(url && *url)) {
			throw runtime_error("Required attribute 'url' is missing or empty");
		}
	}

	int HTTP::Action::call(Udjat::Request &request, Udjat::Response &response, bool except) {
		return Udjat::Action::exec(response,except,[&]() {

			// Get payload
			String payload{text};
			if(payload.empty()) {
				payload = request.to_string(mimetype);
			} else {
				payload.expand(request);
			}

			String response = Protocol::call(
									String{url}.expand(request).c_str(),
									method,
									payload.c_str()
								);

			if(!response.empty()) {
				Logger::String{response.c_str()}.write(Logger::Trace,name());
			}

			return 0;

		});
	}

 }