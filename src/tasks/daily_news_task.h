#pragma once

#include "task_base.h"
#include <atomic>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

struct daily_news_task_setting {
	daily_news_task_setting() = default;

	bool enable = true;
	mn::time time = {0, 0, 9};
	std::string url = u8"https://api.vvhan.com/api/60s";
	std::vector<uint64_t> subscribed_groups;
	std::vector<uint64_t> subscribed_users;
	std::vector<std::string> call_cmds = {u8"日报", u8"news", u8"News"};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(daily_news_task_setting, enable, time, url, subscribed_groups,
                                                subscribed_users, call_cmds)

class daily_news_task : public task_base {
private:
	daily_news_task_setting setting;
	mutable std::mutex mtx;
	std::string url;

public:
	daily_news_task() { task_name = "daily_news_task"; }

	virtual bool shoule_call(mn::meesage whose) override {
		for (const std::string &cmd : setting.call_cmds) {
			if (handle_message(whose.message, cmd)) {
				return true;
			}
		}
		return false;
	}

	virtual void begin() override {
		load_config<daily_news_task_setting>(setting);
		enable = setting.enable;
		std::string temp_url = setting.url;
		temp_url.erase(std::remove_if(temp_url.begin(), temp_url.end(), ::isspace), temp_url.end());
		url = temp_url;
	}

	virtual void run(std::string &response) override {
		if (request_body.empty()) {
			response = u8"[CQ:image,file=" + url + ",cache=0]";
		} else if (request_body == u8"reset") {
			std::unique_lock<std::mutex> locker(mtx);
			std::string temp_url = setting.url;
			temp_url.erase(std::remove_if(temp_url.begin(), temp_url.end(), ::isspace), temp_url.end());
			url = temp_url;
			response = u8"API获取接口已恢复默认配置";
		} else {
			std::unique_lock<std::mutex> locker(mtx);
			std::string temp_url = request_body;
			temp_url.erase(std::remove_if(temp_url.begin(), temp_url.end(), ::isspace), temp_url.end());
			url = temp_url;
			response = u8"API获取接口已切换";
		}
	}

	virtual void tick(mn::time time) override {
		if (setting.time == time) {
			if (send_message_deg) {
				mn::meesage meesage;
				std::string response;
				run(response);
				// 订阅的消息
				meesage.message = "[CQ:at,qq=all] " + response;

				for (uint64_t id : setting.subscribed_groups) {
					meesage.is_group_msg = true;
					meesage.to = id;
					send_message_deg(meesage);
				}

				for (uint64_t id : setting.subscribed_users) {
					meesage.is_group_msg = false;
					meesage.to = id;
					send_message_deg(meesage);
				}
			}
		}
	}
};