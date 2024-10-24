/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2023 Perry Werneck <perry.werneck@gmail.com>
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

 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/version.h>
 #include <udjat/module/abstract.h>
 #include <udjat/module/info.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/http/worker.h>
 
 #include <memory>

 namespace Udjat {

	namespace HTTP {

		/// @brief Generic http module.
		class UDJAT_API Module : public Udjat::Module {
		protected:

			class Protocol : public Udjat::Protocol {
			public:
				Protocol(const char *name, const ModuleInfo &info, bool def = false) : Udjat::Protocol(name,info) {
					if(def) {
						setDefault();
					}
				}

				std::shared_ptr<Worker> WorkerFactory() const override {
					return std::make_shared<Udjat::HTTP::Worker>();
				}
			};

		public:

			static Udjat::Module * Factory(const char *name, const ModuleInfo &info);

			Module(const char *name, const ModuleInfo &info);
			virtual ~Module();

		};

	}

 }
