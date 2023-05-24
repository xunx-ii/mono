#pragma once

#include <functional>
#include <memory>

class noncopyable {
protected:
	noncopyable() {}
	~noncopyable() {}

private:
	noncopyable(const noncopyable &that) = delete;
	noncopyable(noncopyable &&that) = delete;
	noncopyable &operator=(const noncopyable &that) = delete;
	noncopyable &operator=(noncopyable &&that) = delete;
};

class task_cancelable : public noncopyable {
public:
	task_cancelable() = default;
	virtual ~task_cancelable() = default;
	virtual void cancel() = 0;
};

template <class R, class... ArgTypes> class task_cancelable_imp;

template <class R, class... ArgTypes> class task_cancelable_imp<R(ArgTypes...)> : public task_cancelable {
public:
	using ptr = std::shared_ptr<task_cancelable_imp>;
	using func_type = std::function<R(ArgTypes...)>;

	~task_cancelable_imp() = default;

	template <typename FUNC> task_cancelable_imp(FUNC &&task) {
		strong_task = std::make_shared<func_type>(std::forward<FUNC>(task));
		weak_task = strong_task;
	}

	void cancel() override { strong_task = nullptr; }

	operator bool() { return strong_task && *strong_task; }

	void operator=(std::nullptr_t) { strong_task = nullptr; }

	R operator()(ArgTypes... args) const {
		auto task = weak_task.lock();
		if (task && *task) {
			return (*task)(std::forward<ArgTypes>(args)...);
		}
		return default_value<R>();
	}

	template <typename T> static typename std::enable_if<std::is_void<T>::value, void>::type default_value() {}

	template <typename T> static typename std::enable_if<std::is_pointer<T>::value, T>::type default_value() {
		return nullptr;
	}

	template <typename T> static typename std::enable_if<std::is_integral<T>::value, T>::type default_value() {
		return 0;
	}

protected:
	std::weak_ptr<func_type> weak_task;
	std::shared_ptr<func_type> strong_task;
};

using in_task = std::function<void()>;
using task = task_cancelable_imp<void()>;