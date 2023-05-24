#pragma once

#include "curl/curl.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "type.h"

#include <fstream>
#include <list>
#include <memory>
#include <string>

size_t req_reply(void *ptr, size_t size, size_t nmemb, void *stream) {
	if (stream == NULL || ptr == NULL || size == 0)
		return 0;

	size_t realsize = size * nmemb;
	std::string *buffer = (std::string *)stream;
	if (buffer != NULL) {
		buffer->append((const char *)ptr, realsize);
	}
	return realsize;
}

static CURLcode http_get(const std::string &url, bool post, const std::string &req, std::string &response,
                         std::shared_ptr<std::list<std::string>> header = nullptr, int connect_timeout = 10,
                         int timeout = 10) {
	CURL *curl = curl_easy_init();
	CURLcode res;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, post);
		if (!req.empty()) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.c_str());
		}
		struct curl_slist *headers = NULL;
		if (header && header->size() > 0) {
			std::list<std::string>::iterator iter, iterEnd;
			iter = header->begin();
			iterEnd = header->end();
			for (; iter != iterEnd; iter++) {
				headers = curl_slist_append(headers, iter->c_str());
			}

			if (headers != NULL) {
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			}
		}
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, true);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, req_reply);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		res = curl_easy_perform(curl);
		if (headers != NULL) {
			curl_slist_free_all(headers);
		}
	}
	curl_easy_cleanup(curl);
	return res;
}

static void make_app_config(const std::string &path) {
	mn::setting setting;
	nlohmann::json root = setting;
	std::ofstream out(path.c_str());
	if (out.is_open()) {
		out << root.dump(4);
	} else {
		spdlog::error("Failed to generate the configuration file. Procedure {}", path);
	}

	out.close();
}