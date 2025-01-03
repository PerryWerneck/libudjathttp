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
  * @brief Implements HTTP agent.
  */

 #pragma once

 #include <udjat/defs.h>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/http/method.h>
 #include <udjat/tools/actions/http.h>
 #include <udjat/agent.h>
 
 namespace Udjat {

	namespace HTTP {

		class UDJAT_API Agent : public Udjat::Agent<unsigned int>, private Udjat::HTTP::Action {		
		public:

			class Factory : public Udjat::Abstract::Agent::Factory {
			public:
				Factory(const char *name = "url") : Udjat::Abstract::Agent::Factory{name} {
				}

				std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object &parent, const XML::Node &node) const;

			};

			Agent(const XML::Node &node);

			std::shared_ptr<Abstract::State> computeState() override;
			bool refresh(bool) override;

		};

	}

 }
