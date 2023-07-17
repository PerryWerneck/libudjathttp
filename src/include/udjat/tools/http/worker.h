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

 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/protocol.h>
 #include <udjat/tools/http/exception.h>
 #include <udjat/tools/file/handler.h>
 #include <list>

 namespace Udjat {

	namespace HTTP {

		class UDJAT_API Worker : public Protocol::Worker {
		public:
			class Header : public Protocol::Header {
			public:
				Header(const char *name) : Protocol::Header(name) {
				}

				Protocol::Header & assign(const Udjat::TimeStamp &value) override;

			};

		private:

			std::list<Header> headerlist;

		public:

			Worker(const char *url, const HTTP::Method method = HTTP::Get, const char *payload = "") : Protocol::Worker{url,method,payload} {
			}

			inline void socket(int sock) {
				Protocol::Worker::set_socket(sock);
			}

			String get(const std::function<bool(double current, double total)> &progress) override;

			int test(const std::function<bool(double current, double total)> &progress) noexcept override;

			/*
			Worker & credentials(const char *user, const char *passwd) override;

			int test(const std::function<bool(double current, double total)> &progress) noexcept override;

			void save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer) override;
			bool save(const char *filename, const std::function<bool(double current, double total)> &progress, bool replace) override;

			Protocol::Header & header(const char *name) override;
			*/


		};

	}

 }
