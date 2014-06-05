#include "asio_ext_task_shared_mutex.h"
#include "asio_ext_task_handler.h"

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

TaskSharedMutex::TaskSharedMutex()
: numReaders_(0)
, hasWriter_(false)
{
}

///////////////////////////////////////////////////////////////////////////////

TaskSharedMutex::~TaskSharedMutex()
{
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::start(TaskHandlerP parentTaskHandler, const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	startInternal(false, parentTaskHandler, NULL, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::start(basio::io_service& service, const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	startInternal(false, TaskHandlerP(), &service, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startShared(TaskHandlerP parentTaskHandler, const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	startInternal(true, parentTaskHandler, NULL, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startShared(basio::io_service& service, const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	startInternal(true, TaskHandlerP(), &service, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startInternal(bool shared, TaskHandlerP parentTaskHandler, basio::io_service* service, const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	auto wrappedExitHandler = [this, shared, exitHandler]()
	{
		taskFinished(shared);

		if (exitHandler)
			exitHandler();
	};

	boost::mutex::scoped_lock lock(accessMutex_);

	const bool deferred = (!waitingTasks_.empty() || hasWriter_ || (!shared && numReaders_ > 0));

	if (!deferred)
	{
		taskStartedInternal(shared);

		if (service)
			TaskHandler::start(*service, task, successHandler, wrappedExitHandler);
		else
			parentTaskHandler->startChild(task, successHandler, wrappedExitHandler);
	}
	else
	{
		auto wrappedTask = [this, shared, task](TaskHandlerP taskHandler)
		{
			taskStarted(shared);
			task(taskHandler);
		};
		
		if (service)
		{
			auto startTask = [service, wrappedTask, successHandler, wrappedExitHandler]()
			{
				TaskHandler::start(*service, wrappedTask, successHandler, wrappedExitHandler);
			};

			waitingTasks_.push_back(WaitingTask(shared, startTask));
		}
		else
		{
			auto startTask = [parentTaskHandler, wrappedTask, successHandler, wrappedExitHandler]()
			{
				parentTaskHandler->startChild(wrappedTask, successHandler, wrappedExitHandler);
			};

			waitingTasks_.push_back(WaitingTask(shared, startTask));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::taskStartedInternal(bool shared)
{
	if (shared)
		++numReaders_;
	else
		hasWriter_ = true;
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::taskStarted(bool shared)
{
	boost::mutex::scoped_lock lock(accessMutex_);
	taskStartedInternal(shared);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::taskFinished(bool shared)
{
	// todo: make lock_guard with more precise range
	boost::mutex::scoped_lock lock(accessMutex_);

	if (shared)
		--numReaders_;
	else
		hasWriter_ = false;

	while (!waitingTasks_.empty())
	{
		WaitingTask& waitingTask = waitingTasks_.front();

		const bool canBeStarted = !(hasWriter_ || (!waitingTask.shared_ && numReaders_ > 0));

		if (canBeStarted)
		{
			taskStartedInternal(waitingTask.shared_);
			waitingTask.startTask_();
			waitingTasks_.pop_front();
		}
		else
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////

}
