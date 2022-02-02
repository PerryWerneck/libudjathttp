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

 #include <internals.h>
 #include <cstring>
 #include <unistd.h>
 #include <cstdio>
 #include <udjat/tools/http/timestamp.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <libgen.h>
 #include <utime.h>

 namespace Udjat {

	struct TransferData {
		int fd = -1;
		const char *filename;

		TransferData(const char *filename);
		~TransferData();

		void save() const;

	};

	TransferData::TransferData(const char *f) : filename(f) {
		char path[PATH_MAX];
		strncpy(path,filename,PATH_MAX);

		fd = open(dirname(path),O_TMPFILE | O_WRONLY, S_IRUSR | S_IWUSR);
		if(fd < 0) {
			throw system_error(errno,system_category(),Logger::Message("Can't create '{}'",filename));
		}

	}

	TransferData::~TransferData() {

		::close(fd);
	}

	void TransferData::save() const {

		char tempfile[PATH_MAX];
		snprintf(tempfile, PATH_MAX,  "/proc/self/fd/%d", fd);

		struct stat st;
		if(stat(filename, &st) == -1) {
			if(errno != ENOENT) {
				throw system_error(errno,system_category(),"Error getting file permissions");
			}
			memset(&st,0,sizeof(st));
			st.st_mode = 0644;
		}

		if(linkat(AT_FDCWD, tempfile, AT_FDCWD, filename, AT_SYMLINK_FOLLOW) == 0) {
			return;
		}

		if(errno != EEXIST) {
			throw system_error(errno,system_category(),"Error saving file");
		}

		char bakfile[PATH_MAX];
		strncpy(bakfile,filename,PATH_MAX);
		char *ptr = strrchr(bakfile,'.');
		if(ptr) {
			*ptr = 0;
		}
		strncat(bakfile,".bak",PATH_MAX);
		unlink(bakfile);

		if(rename(filename, bakfile) != 0) {
			throw system_error(errno,system_category(),"Cant create backup");
		}

		if(linkat(AT_FDCWD, tempfile, AT_FDCWD, filename, AT_SYMLINK_FOLLOW) != 0) {
			throw system_error(errno,system_category(),"Error saving file");
		}

		chmod(filename,st.st_mode);
		chown(filename,st.st_uid,st.st_gid);

	}

	static size_t write_file(void *contents, size_t size, size_t nmemb, TransferData *data) noexcept {

		size_t length = size * nmemb;
		size_t pending = length;
		const char *ptr = (const char *) contents;

		while(pending) {
			ssize_t bytes = ::write(data->fd, ptr, pending);
			if(bytes < 1) {
				cerr << "http\tError '" << strerror(errno) << "' writing temporary file" << endl;
				return 0;
			}
			pending -= bytes;
			ptr += bytes;
		}

		return length;

	}

	void HTTP::Client::Worker::get(const char *filename, time_t timestamp) {

		TransferData data(filename);

		curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, &data);
		curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, write_file);

		struct curl_slist *chunk = curl_slist_append(NULL, "Cache-Control:public, max-age=31536000");

		if(timestamp) {
			string hdr{"If-Modified-Since: "};
			hdr += HTTP::TimeStamp(timestamp).to_string();
			hdr += ";";
#ifdef DEBUG
			cout << hdr << endl;
#endif // DEBUG
			chunk = curl_slist_append(chunk, hdr.c_str());
		}

		curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, chunk);

		CURLcode res = curl_easy_perform(hCurl);

		curl_slist_free_all(chunk);

		if(res != CURLE_OK) {
#ifdef DEBUG
			cout << "CURL-Error=" << res << endl;
#endif // DEBUG
			throw HTTP::Exception(this->client->url.c_str(),curl_easy_strerror(res));
		}

		long response_code = 0;
		curl_easy_getinfo(hCurl, CURLINFO_RESPONSE_CODE, &response_code);

		if(response_code == 200) {

			data.save();

		} else if(response_code == 304) {

			cout << "http\tUsing local '" << filename << "' (not modified)" << endl;
			return;

		} else if(message.empty()) {

			throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str());

		} else {

			throw HTTP::Exception((unsigned int) response_code, this->client->url.c_str(), message.c_str());

		}

		if(this->timestamp) {

			utimbuf ub;
			ub.actime = time(0);
			ub.modtime = (time_t) this->timestamp;

			if(utime(filename,&ub) == -1) {
				cerr << "http\tError '" << strerror(errno) << "' setting file timestamp" << endl;
			} else {
				cout << "http\t'" << filename << "' time set to " << this->timestamp << endl;
			}

		}

	}


 }

