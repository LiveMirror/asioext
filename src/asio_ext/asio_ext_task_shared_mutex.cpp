#include "asio_ext_task_shared_mutex.h"
#include "asio_ext_task_handler.h"
#include "asio_ext_service.h"

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

	assert(tasks_.empty());
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::start(TaskHandlerP parentTaskHandler, const TaskFunc& task, const SuccessFunc& successHandler, 
	const ExitFunc& exitHandler)
{
	startInternal(false, parentTaskHandler, NULL, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::start(Service& service, const TaskFunc& task, const SuccessFunc& successHandler, 
	const ExitFunc& exitHandler)
{
	startInternal(false, TaskHandlerP(), &service, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startShared(TaskHandlerP parentTaskHandler, const TaskFunc& task, const SuccessFunc& successHandler, 
	const ExitFunc& exitHandler)
{
	startInternal(true, parentTaskHandler, NULL, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startShared(Service& service, const TaskFunc& task, const SuccessFunc& successHandler, 
	const ExitFunc& exitHandler)
{
	startInternal(true, TaskHandlerP(), &service, task, successHandler, exitHandler);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::assertLocked()
{
	boost::mutex::scoped_lock lock(tasksMutex_);

	TaskHandler* currentTaskHandler = NULL;

	for (auto taskHandler = tasks_.begin(); taskHandler != tasks_.end(); ++taskHandler)
	{
		if (currentTaskHandler = (*taskHandler)->getService().getCurrentTask())
			break;
	}

	assert(currentTaskHandler != NULL);

	bool foundParent = false;

	for (auto taskHandler = tasks_.begin(); taskHandler != tasks_.end(); ++taskHandler)
	{
		if (currentTaskHandler == *taskHandler || currentTaskHandler->isChildOf(**taskHandler))
		{
			foundParent = true;
			break;
		}
	}

	assert(foundParent);
}

///////////////////////////////////////////////////////////////////////////////

void TaskSharedMutex::startInternal(bool shared, TaskHandlerP parentTaskHandler, Service* serviceP, const TaskFunc& task, 
	const SuccessFunc& successHandler, const ExitFunc& exitHandler)
{
	Service& service = serviceP ? *serviceP : parentTaskHandler->getService();

	auto wrappedTask = [this, task](TaskHandlerP taskHandler)
	{
		{
			boost::mutex::scoped_lock lock(tasksMutex_);
			tasks_.push_back(taskHandler.get());
		}

		task(taskHandler);
	};

	auto wrappedExitHandler = [this, &service, shared, exitHandler](TaskHandler* taskHandler)
	{
		if (exitHandler)
			exitHandler(taskHandler);

		{
			{
				boost::mutex::scoped_lock lock(tasksMutex_);

				auto it = std::find(tasks_.begin(), tasks_.end(), taskHandler);
				assert(it != tasks_.end());
				tasks_.erase(it);
			}

			{
				boost::mutex::scoped_lock lock(accessMutex_);
				taskFinishedInternal(shared);
			}
		}

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

		if (serviceP)
			TaskHandler::start(*serviceP, wrappedTask, successHandler, wrappedExitHandler);
		else
			parentTaskHandler->startChild(task, successHandler, wrappedExitHandler);
	}
	else
	{
		if (serviceP)
		{
			auto startTask = [serviceP, wrappedTask, successHandler, wrappedExitHandler]()
			{
				TaskHandler::start(*serviceP, wrappedTask, successHandler, wrappedExitHandler);
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

void TaskSharedMutex::taskFinishedInternal(bool shared)
{
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
