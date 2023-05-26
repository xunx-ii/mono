#pragma once

#include "../mono.h"
#include "../type.h"
#include <filesystem>
#include <string>

class task_base {
public:
	task_base() {}

	// 返回true时 将发送run的response给消息发送者。
	virtual bool shoule_call(mn::meesage whose) { return false; }

	virtual void run(std::string &response) {}

	// 初始化函数 函数启动时调用一次
	virtual void begin() {}

	virtual void set_send_message_deg(std::function<void(mn::meesage message)> deg) { send_message_deg = deg; }

	// 每秒跑一次 send_message_deg是一个发送信息的回调
	virtual void tick(mn::time time) {}

	virtual std::string get_config_path() {
		std::filesystem::path base_task_config_path = std::filesystem::current_path() / "config";
		if (!std::filesystem::exists(base_task_config_path)) {
			std::filesystem::create_directory(base_task_config_path);
		}
		return (base_task_config_path / (task_name + ".json")).string();
	}

	virtual bool handle_message(const std::string &cmd, const std::string &call_cmd) {
		if (cmd == call_cmd) {
			return true;
		}

		int pos = cmd.find_first_of(" ");

		if (cmd.substr(0, pos) == call_cmd) {
			request_body = cmd.substr(pos + 1, cmd.size());
			return true;
		}

		return false;
	}

	template <class T> bool load_config(T &setting) {
		nlohmann::json root = setting;
		std::string config_path = get_config_path();
		if (!std::filesystem::exists(config_path)) {
			std::ofstream out(config_path.c_str());
			if (out.is_open()) {
				out << root.dump(4);
			} else {
				spdlog::error("Failed to generate the configuration file. Procedure {}",
				              config_path.c_str());
				return false;
			}
			out.close();
		}
		std::ifstream ifs(config_path);
		if (!ifs.is_open()) {
			spdlog::error("Failed to open config file: {}", config_path);
			return false;
		}

		try {
			root = nlohmann::json::parse(ifs);
			setting = root.get<T>();
		} catch (nlohmann::json::parse_error &e) {
			spdlog::error("Failed to parse config file: {} {}", config_path, e.what());
			ifs.close();
			return false;
		}

		ifs.close();
		return true;
	}

public:
	bool enable;
	bool has_config;
	std::string task_name;
	std::string request_body;
	std::function<void(mn::meesage message)> send_message_deg;
};
