#pragma once

#include "../util.h"
#include "task_base.h"
#include <spdlog/spdlog.h>
#include <string>

class daily_tasks : public task_base {
public:
	daily_tasks() {
		task_name = "daily_tasks";
		request.cmd = u8"日常";
	}

	virtual void begin() override {}

	virtual void run(std::string &response) override {}

	virtual void tick(mn::time time) override {}
};