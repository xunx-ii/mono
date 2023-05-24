#pragma once

#include "task_base.h"
#include <spdlog/spdlog.h>
#include <string>

struct daily_news_task_setting {
	daily_news_task_setting() = default;

	bool enable = true;
	mn::time time = {0, 0, 9};
	std::vector<uint64_t> subscribed_groups;
	std::vector<uint64_t> subscribed_users;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(daily_news_task_setting, enable, time, subscribed_groups,
                                                subscribed_users)

class daily_news_task : public task_base {
private:
	daily_news_task_setting setting;

public:
	daily_news_task() {
		task_name = "daily_news_task";
		request.cmd = u8"日报";
	}

	virtual void begin() override {
		task_base::begin();
		load_config<daily_news_task_setting>(setting);
		enable = setting.enable;
	}

	virtual void run(std::string &response) override {
		response = "[CQ:image,file=https://api.03c3.cn/zb/,cache=0]";
	}

	virtual void tick(mn::time time) override {
		if (setting.time == time) {
			if (send_message_deg) {
				mn::meesage meesage;
				std::string response;
				run(response);
				meesage.message = response;

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