/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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
  * @brief Declare curl based url handler.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/url/handler.h>
 #include <vector>
 #include <string>
 
 namespace Udjat {

 	namespace HTTP {

		class Context;

		/// @brief The HTTP client engine.
		class UDJAT_API Handler : public Udjat::URL::Handler {
		private:
			friend class Context;

			struct Header {
				const std::string name;
				const std::string value;

				Header(const char *n, const char *v) : name{n}, value{v} {
				}
			};

			struct {
				std::vector<Header> request;
				std::vector<Header> response;
			} headers;

		protected:
			const URL url;

		public:

			class Factory : public Udjat::URL::Handler::Factory {
			public:
				Factory(const char *name);
				virtual ~Factory();
				std::shared_ptr<Udjat::URL::Handler> HandlerFactory(const URL &url) const override;
			};

			Handler(const URL &url);

			virtual ~Handler();

			const char * c_str() const noexcept override;

			bool get(Udjat::Value &value, const HTTP::Method method = HTTP::Get, const char *payload = "") override;

			URL::Handler & header(const char *name, const char *value) override;

			const char * header(const char *name) const override;

			int test(const HTTP::Method method = HTTP::Get, const char *payload = "") override;

			int perform(const HTTP::Method method, const char *payload, const std::function<bool(uint64_t current, uint64_t total, const char *data, size_t len)> &progress) override;

		};

 	}

 }

