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
	boost::unique_lock<boost::mutex> lock(accessMutex_);

	while (!waitingTasks_.empty() || numReaders_ > 0 || hasWriter_)
	{
		taskFinishedCond_.wait(lock);
	}
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

		{
			boost::mutex::scoped_lock lock(accessMutex_);
			taskFinishedCond_.notify_one();
		}
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
		if (service)
		{
			auto startTask = [service, task, successHandler, wrappedExitHandler]()
			{
				TaskHandler::start(*service, task, successHandler, wrappedExitHandler);
			};

			waitingTasks_.push_back(WaitingTask(shared, startTask));
		}
		else
		{
			auto startTask = [parentTaskHandler, task, successHandler, wrappedExitHandler]()
			{
				parentTaskHandler->startChild(task, successHandler, wrappedExitHandler);
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
