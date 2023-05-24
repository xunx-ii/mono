#pragma once

#include "mono.h"

enum recv_message_type { text, binary };

namespace mn {
struct meesage {
	bool is_group_msg;
	uint64_t to;
	std::string message;
};
struct time {
	uint8_t sec;  /* Seconds.	[0-60] (1 leap second) */
	uint8_t min;  /* Minutes.	[0-59] */
	uint8_t hour; /* Hours.	[0-23] */

	bool operator==(const time &anthoer) {
		return sec == anthoer.sec && min == anthoer.min && hour == anthoer.hour;
	}
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(time, sec, min, hour)
struct setting {
	setting() = default;
	setting(const std::string &in_server_uri, const std::string &in_access_token)
	    : server_uri(std::move(in_server_uri)), access_token(std::move(in_access_token)) {}

	std::string server_uri = "ws://127.0.0.1:8080";
	std::string access_token = "";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(setting, server_uri, access_token)
} // namespace mn
