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
  * @brief Implements HTTP Action.
  */

 #pragma once

 #include <udjat/defs.h>
 #include <udjat/tools/action.h>
 
 namespace Udjat {

	namespace HTTP {

		class UDJAT_API Action : public Udjat::Action {
		protected:
			const char *url;
			const HTTP::Method method;
			const char *text = "";
			const MimeType mimetype;
		
		public:

			class Factory : public Udjat::Action::Factory {
			public:
				Factory(const char *name = "url") : Udjat::Action::Factory{name} {
				}

				std::shared_ptr<Udjat::Action> ActionFactory(const XML::Node &node) const override;

			};

			Action(const XML::Node &node);

			int call(const Udjat::Value &request, Udjat::Value &response, bool except) override;

		};

	}

 }
