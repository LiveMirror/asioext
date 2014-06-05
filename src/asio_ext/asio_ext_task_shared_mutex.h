#pragma once

#include "asio_ext_config.h"

#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

class TaskSharedMutex : boost::noncopyable
{
	struct WaitingTask
	{
		bool shared_;

		VoidFunc startTask_;

		WaitingTask(bool shared, const VoidFunc& startTask)
			: shared_(shared), startTask_(startTask)
		{}
	};

	boost::mutex accessMutex_;

	uint numReaders_;
	bool hasWriter_;

	std::list<WaitingTask> waitingTasks_;

public:
	TaskSharedMutex();
	~TaskSharedMutex();

	void start(TaskHandlerP parentTaskHandler, const TaskFunc& task, const VoidFunc& successHandler = VoidFunc(), 
		const VoidFunc& exitHandler = VoidFunc());
	void start(basio::io_service& service, const TaskFunc& task, const VoidFunc& successHandler = VoidFunc(), 
		const VoidFunc& exitHandler = VoidFunc());
	void startShared(TaskHandlerP parentTaskHandler, const TaskFunc& task, const VoidFunc& successHandler = VoidFunc(), 
		const VoidFunc& exitHandler = VoidFunc());
	void startShared(basio::io_service& service, const TaskFunc& task, const VoidFunc& successHandler = VoidFunc(), 
		const VoidFunc& exitHandler = VoidFunc());

private:
	void startInternal(bool shared, TaskHandlerP parentTaskHandler, basio::io_service* service, const TaskFunc& task, const VoidFunc& successHandler, 
		const VoidFunc& exitHandler);
	void taskStartedInternal(bool shared);

	void taskStarted(bool shared);
	void taskFinished(bool shared);
};

///////////////////////////////////////////////////////////////////////////////
}
