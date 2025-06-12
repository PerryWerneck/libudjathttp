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

 #include <config.h>
 #include <udjat/defs.h>
 #include <private/context.h>
 #include <udjat/tools/string.h>
 #include <udjat/net/ip/address.h>

 using namespace std;

 namespace Udjat {

	void HTTP::Context::set_local(const sockaddr_storage &addr) noexcept {

		payload.text.expand([addr](const char *key, std::string &value){

			if(strcasecmp(key,"ipaddr") == 0) {
				value = to_string(addr);
				return true;
			}

			if(strcasecmp(key,"network-interface") == 0 || strcasecmp(key,"nic") == 0) {
				value = IP::Address{addr}.nic();
				return true;
			}

			if(strcasecmp(key,"macaddress") == 0) {
				value = IP::Address{addr}.macaddress();
				return true;
			}

			return false;

        },false,false);

	}

	void HTTP::Context::set_remote(const sockaddr_storage &addr) noexcept {

		payload.text.expand([addr](const char *key, std::string &value){

			if(strcasecmp(key,"hostip") == 0) {
				value = to_string(addr);
				return true;
			}

			return false;

		},false,false);

	}

 }

