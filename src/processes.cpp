#include "processes.h"
#include "application.h"
#include "tasks/daily_news_task.h"
#include <cstdint>
#include <spdlog/spdlog.h>

processes::processes() {
	tasker_mgr.start();
	tasks.push_back(new daily_news_task());
}

processes::~processes() {}

void processes::send(mn::time time) {
	// 每秒一次发送一次
	mn::meesage meesage;
	if (processed_messages.pop(meesage)) {
		app->send_message(meesage.is_group_msg ? build_group_msg(meesage.message, meesage.to)
		                                       : build_private_msg(meesage.message, meesage.to));
	}
}

void processes::tick(mn::time time, float delta_time) {
	static uint8_t sec = 0;

	// 间隔是每秒一次
	if (time.sec != sec) {
		for (task_base *task : tasks) {
			if (task->enable) {
				task->tick(time);
			}
		}
		send(time);
		sec = time.sec;
	}
}

void processes::begin() {
	for (task_base *task : tasks) {
		task->begin();
		task->set_send_message_deg([this](mn::meesage message) { processed_messages.push(message); });
	}
}

void processes::processing(recv_message_type type, std::string message) {
	if (type == recv_message_type::text) {
		nlohmann::json root;
		try {
			root = nlohmann::json::parse(message);
			if (root["post_type"] == "message") {
				if (root["message_type"] == "group" || root["message_type"] == "private") {
					bool is_group_msg = root["message_type"] == "group";
					uint64_t to = is_group_msg ? root["group_id"] : root["user_id"];
					std::string cmd = root["message"];
					for (task_base *tasker : tasks) {
						if (!tasker->enable)
							continue;
						tasker_mgr.async([this, tasker, to, cmd, is_group_msg]() {
							if (tasker->shoule_call({is_group_msg, to, cmd})) {
								std::string response;
								tasker->run(response);

								mn::meesage info;
								info.to = to;
								info.is_group_msg = is_group_msg;
								info.message = response;

								processed_messages.push(info);
							}
						});
					}
				}
			}

		} catch (nlohmann::json::parse_error &e) {
			spdlog::error("failed to parse message: {}", e.what());
		}
	}
}
