#pragma once

#include "nlohmann/json.hpp"

static std::string build_group_msg(const std::string &message, uint64_t group_id) {
	nlohmann::json root;
	root["action"] = "send_group_msg";
	root["params"]["group_id"] = group_id;
	root["params"]["message"] = message;
	return root.dump(4);
}

static std::string build_private_msg(const std::string &message, uint64_t id) {
	nlohmann::json root;
	root["action"] = "send_private_msg";
	root["params"]["user_id"] = id;
	root["params"]["message"] = message;
	return root.dump(4);
}