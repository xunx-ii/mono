#pragma once

#include <mutex>
#include <queue>

// 这个队列不需要唤醒机制
template <typename T> class queue {
public:
	void push(const T &msg) {
		std::unique_lock<std::mutex> locker(mtx);
		q.push(std::move(msg));
	}

	bool pop(T &msg) {
		std::lock_guard<std::mutex> locker(mtx);
		if (q.empty()) {
			return false;
		}
		msg = std::move(q.front());
		q.pop();
		return true;
	}

	bool empty() const {
		std::lock_guard<std::mutex> locker(mtx);
		return q.empty();
	}

private:
	std::queue<T> q;
	mutable std::mutex mtx;
};