#pragma once

#include "asio_ext_config.h"

#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/condition_variable.hpp>

namespace AsioExt
{
class Service;
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


	boost::mutex tasksMutex_;
	std::vector<TaskHandler*> tasks_;

	std::list<WaitingTask> waitingTasks_;
	boost::condition_variable taskFinishedCond_;

public:
	TaskSharedMutex();
	~TaskSharedMutex();

	void start(TaskHandlerP parentTaskHandler, const TaskFunc& task, const SuccessFunc& successHandler = SuccessFunc(), 
		const ExitFunc& exitHandler = ExitFunc());
	void start(Service& service, const TaskFunc& task, const SuccessFunc& successHandler = SuccessFunc(), 
		const ExitFunc& exitHandler = ExitFunc());
	void startShared(TaskHandlerP parentTaskHandler, const TaskFunc& task, const SuccessFunc& successHandler = SuccessFunc(), 
		const ExitFunc& exitHandler = ExitFunc());
	void startShared(Service& service, const TaskFunc& task, const SuccessFunc& successHandler = SuccessFunc(), 
		const ExitFunc& exitHandler = ExitFunc());

	void assertLocked();

private:
	void startInternal(bool shared, TaskHandlerP parentTaskHandler, Service* service, const TaskFunc& task, const SuccessFunc& successHandler, 
		const ExitFunc& exitHandler);
	void taskStartedInternal(bool shared);
	void taskFinishedInternal(bool shared);
};

///////////////////////////////////////////////////////////////////////////////
}
