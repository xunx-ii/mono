#pragma once

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include "mono.h"
#include "tasker.h"

class semaphore {
public:
	explicit semaphore(size_t initial = 0) { count = 0; }

	void post(size_t n = 1) {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		count += n;
		if (n == 1) {
			cv.notify_one();
		} else {
			cv.notify_all();
		}
	}

	void wait() {
		std::unique_lock<std::recursive_mutex> lock(mtx);
		while (count == 0) {
			cv.wait(lock);
		}
		--count;
	}

private:
	size_t count;
	std::recursive_mutex mtx;
	std::condition_variable_any cv;
};

class thread_group {
private:
	thread_group(thread_group const &);
	thread_group &operator=(thread_group const &);

public:
	thread_group() {}

	~thread_group() { threads.clear(); }

	bool is_this_thread_in() {
		auto this_id = std::this_thread::get_id();
		if (this_id == thread_id) {
			return true;
		}
		return threads.find(this_id) != threads.end();
	}

	bool is_thread_in(std::thread *thrd) {
		if (!thrd) {
			return false;
		}
		auto it = threads.find(thrd->get_id());
		return it != threads.end();
	}

	template <typename F> std::thread *create_thread(F &&thread_func) {
		auto thread_new = std::make_shared<std::thread>(thread_func);
		thread_id = thread_new->get_id();
		threads[thread_id] = thread_new;
		return thread_new.get();
	}

	void remove_thread(std::thread *thrd) {
		auto it = threads.find(thrd->get_id());
		if (it != threads.end()) {
			threads.erase(it);
		}
	}

	void join_all() {
		if (is_this_thread_in()) {
			throw std::runtime_error("Trying joining itself in thread_group");
		}
		for (auto &it : threads) {
			if (it.second->joinable()) {
				it.second->join();
			}
		}
		threads.clear();
	}

	size_t size() { return threads.size(); }

private:
	std::thread::id thread_id;
	std::unordered_map<std::thread::id, std::shared_ptr<std::thread>> threads;
};

template <typename T> class func_list : public std::list<T> {
public:
	template <typename... ARGS> func_list(ARGS &&...args) : std::list<T>(std::forward<ARGS>(args)...){};

	~func_list() = default;

	void append(func_list<T> &other) {
		if (other.empty()) {
			return;
		}
		this->insert(this->end(), other.begin(), other.end());
		other.clear();
	}

	template <typename FUNC> void for_each(FUNC &&func) {
		for (auto &t : *this) {
			func(t);
		}
	}

	template <typename FUNC> void for_each(FUNC &&func) const {
		for (auto &t : *this) {
			func(t);
		}
	}
};

template <typename T> class task_queue {
public:
	template <typename C> void push_task(C &&task_func) {
		{
			std::lock_guard<decltype(mtx)> lock(mtx);
			queue.emplace_back(std::forward<C>(task_func));
		}
		sem.post();
	}

	template <typename C> void push_task_first(C &&task_func) {
		{
			std::lock_guard<decltype(mtx)> lock(mtx);
			queue.emplace_front(std::forward<C>(task_func));
		}
		sem.post();
	}

	void push_exit(size_t n) { sem.post(n); }

	bool get_task(T &tsk) {
		sem.wait();
		std::lock_guard<decltype(mtx)> lock(mtx);
		if (queue.empty()) {
			return false;
		}
		tsk = std::move(queue.front());
		queue.pop_front();
		return true;
	}

	size_t size() const {
		std::lock_guard<decltype(mtx)> lock(mtx);
		return queue.size();
	}

private:
	func_list<T> queue;
	mutable std::mutex mtx;
	semaphore sem;
};

class taskermgr {
public:
	taskermgr();
	~taskermgr();

	void start();
	task::ptr async(in_task _task);
	size_t size();

private:
	void run();

private:
	size_t thread_num;
	task_queue<task::ptr> tasks;
	thread_group threads;
};
