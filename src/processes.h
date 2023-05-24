#pragma once

#include "mono.h"
#include "taskermgr.h"
#include "tasks/task_base.h"
#include "type.h"

class application;

class processes {
public:
	processes();
	~processes();

	void tick(mn::time time, float delta_time);
	void processing(recv_message_type type, std::string message);
	void begin();

public:
	application *app;

private:
	void send(mn::time time);

private:
	std::vector<task_base *> tasks;
	queue<mn::meesage> processed_messages;
	taskermgr tasker_mgr;
};
