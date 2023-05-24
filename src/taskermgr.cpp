#include "taskermgr.h"

taskermgr::taskermgr() { thread_num = std::thread::hardware_concurrency(); }

taskermgr::~taskermgr() {
	tasks.push_exit(thread_num);
	threads.join_all();
}

void taskermgr::start() {
	if (thread_num <= 0) {
		return;
	}
	size_t total = thread_num - threads.size();
	for (size_t i = 0; i < total; ++i) {
		threads.create_thread([this]() { run(); });
	}
}

task::ptr taskermgr::async(in_task _task) {
	auto ret = std::make_shared<task>(std::move(_task));
	tasks.push_task_first(ret);
	return ret;
}

size_t taskermgr::size() { return tasks.size(); }

void taskermgr::run() {
	task::ptr task;
	while (true) {
		if (!tasks.get_task(task)) {
			break;
		}

		try {
			(*task)();
			task = nullptr;
		} catch (std::exception &ex) {
			spdlog::error("ThreadPool catch a exception: {}", ex.what());
		}
	}
}
