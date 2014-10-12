#pragma once

template <typename T>
class TaskSharedObject
{
	TaskSharedMutex mutex_;
	std::unique_ptr<T> object_;

public:
	TaskSharedObject(std::unique_ptr<T> object);
	// ждём, пока не закончатся таски, при этом не принимаем новые - возвращаем false
	~TaskSharedObject();

	bool postShared(TaskHandlerP task, boost::function<void (TaskHandlerP, const T&)>) const;
	bool post(TaskHandlerP task, boost::function<void (TaskHandlerP, T&)>);
	bool postDelete(TaskHandlerP task, boost::function<void (TaskHandlerP)>);

	operator bool () const;
};