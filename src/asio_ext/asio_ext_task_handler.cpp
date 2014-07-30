#include "asio_ext_task_handler.h"

#include <boost/bind.hpp>

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

TaskHandler::TaskHandler(basio::io_service& service, TaskHandlerP parentTaskHandler, 
	const TaskFunc& task, const VoidFunc& successHandler, const VoidFunc& exitHandler)
: service_(service)
, parentTaskHandler_(parentTaskHandler)
, task_(task)
, successHandler_(successHandler)
, exitHandler_(exitHandler)
, taskPosted_(false)
{
	if (parentTaskHandler)
		abortedFunc_ = &TaskHandler::_abortedFromParent;
}

///////////////////////////////////////////////////////////////////////////////

TaskHandler::TaskHandler(basio::io_service& service, const TaskFunc& task, 
	const VoidFunc& successHandler, const VoidFunc& exitHandler)
: service_(service)
, task_(task)
, successHandler_(successHandler)
, exitHandler_(exitHandler)
, abortedFunc_(&TaskHandler::_notAborted)
, taskPosted_(false)
{
}

///////////////////////////////////////////////////////////////////////////////

TaskHandlerP TaskHandler::start(basio::io_service& service, const TaskFunc& task, 
	const VoidFunc& successHandler, const VoidFunc& exitHandler)
{
	assert(task);
	TaskHandlerP taskHandler(new TaskHandler(service, task, successHandler, exitHandler));
	service.post(boost::bind(task, taskHandler));
	return taskHandler;
}

///////////////////////////////////////////////////////////////////////////////

TaskHandler::~TaskHandler()
{
	if (parentTaskHandler_)
	{
		boost::mutex::scoped_lock lock(parentTaskHandler_->childrenActivityMutex_);
		unlink();
	}

	if (successHandler_ && !aborted())
		successHandler_();

	if (exitHandler_)
		exitHandler_();
}

///////////////////////////////////////////////////////////////////////////////

TaskHandlerP TaskHandler::startChild(const TaskFunc& task, const VoidFunc& successHandler, 
	const VoidFunc& exitHandler)
{
	assert(task);

	TaskFunc wrappedTask = [this, task] (TaskHandlerP taskHandler)
	{
		boost::mutex::scoped_lock lock(childrenActivityMutex_);

		assert(taskPosted_);

		if (!waitingTasks_.empty())
		{
			TaskHandlerP taskHandler;
			waitingTasks_.front().selfShared_.swap(taskHandler);
			assert(taskHandler);

			waitingTasks_.erase(waitingTasks_.begin());
			service_.post(boost::bind(taskHandler->task_, taskHandler));
		}
		else
		{
			taskPosted_ = false;
		}

		task(taskHandler);
	};

	TaskHandlerP taskHandler(new TaskHandler(service_, shared_from_this(), wrappedTask, successHandler, exitHandler));

	{
		boost::mutex::scoped_lock lock(childrenActivityMutex_);

		if (taskPosted_)
		{
			taskHandler->selfShared_ = taskHandler;
			waitingTasks_.push_back(*taskHandler);
		}
		else
		{
			service_.post(boost::bind(wrappedTask, taskHandler));
			taskPosted_ = true;
		}
	}

	return taskHandler;
}

///////////////////////////////////////////////////////////////////////////////

bool TaskHandler::aborted() const
{
	return (this->*abortedFunc_)();
}

///////////////////////////////////////////////////////////////////////////////

void TaskHandler::abort()
{
	abortedFunc_ = &TaskHandler::_aborted;
}

///////////////////////////////////////////////////////////////////////////////

bool TaskHandler::_notAborted() const 
{
	return false; 
}

///////////////////////////////////////////////////////////////////////////////

bool TaskHandler::_aborted() const 
{ 
	return true; 
}

///////////////////////////////////////////////////////////////////////////////

bool TaskHandler::_abortedFromParent() const 
{ 
	return parentTaskHandler_->aborted(); 
}

///////////////////////////////////////////////////////////////////////////////
}
