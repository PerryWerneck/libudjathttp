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
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/http/exception.h>
 #include <udjat/tools/file/handler.h>
 #include <list>

 namespace Udjat {

	namespace HTTP {

		class UDJAT_API Worker : public Protocol::Worker {
		private:

			struct {
				std::list<Protocol::Header> request;
				std::list<Protocol::Header> response;
			} headers;

			struct {
				std::string user;
				std::string passwd;
			} auth;

			void response(const char *name, const char *value);

		public:

			Worker(const char *url = "", const HTTP::Method method = HTTP::Get, const char *payload = "") : Protocol::Worker{url,method,payload} {
			}

			const char * user() const noexcept {
				return auth.user.c_str();
			}

			const char * passwd() const noexcept {
				return auth.passwd.c_str();
			}

			inline void socket(int sock) {
				Protocol::Worker::set_socket(sock);
			}

			String get(const std::function<bool(double current, double total)> &progress) override;

			int test(const std::function<bool(double current, double total)> &progress) noexcept override;

			inline const std::list<Protocol::Header> requests() const {
				return headers.request;
			}

			inline const std::list<Protocol::Header> responses() const {
				return headers.response;
			}

			Protocol::Header & request(const char *name) override;
			const Protocol::Header & response(const char *name) override;

			Worker & credentials(const char *user, const char *passwd) override;

			bool save(File::Handler &file, const std::function<bool(double current, double total)> &progress) override;
			bool save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) override;
			void save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer) override;

		};

	}

 }
