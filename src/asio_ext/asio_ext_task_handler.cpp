#include "asio_ext_task_handler.h"
#include "asio_ext_service.h"

#include <boost/bind.hpp>

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

TaskHandler::TaskHandler(Service& service, TaskHandlerP parentTaskHandler, 
	const TaskFunc& task, const SuccessFunc& successHandler, const ExitFunc& exitHandler)
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

TaskHandler::TaskHandler(Service& service, const TaskFunc& task, 
	const SuccessFunc& successHandler, const ExitFunc& exitHandler)
: service_(service)
, task_(task)
, successHandler_(successHandler)
, exitHandler_(exitHandler)
, abortedFunc_(&TaskHandler::_notAborted)
, taskPosted_(false)
{
}

///////////////////////////////////////////////////////////////////////////////

TaskHandlerP TaskHandler::start(Service& service, const TaskFunc& task, 
	const SuccessFunc& successHandler, const ExitFunc& exitHandler)
{
	assert(task);

	TaskFunc wrappedTask = [&service, task] (TaskHandlerP taskHandler)
	{
		service.setCurrentTask(*taskHandler);
		task(taskHandler);
	};

	TaskHandlerP taskHandler = std::make_shared<TaskHandler>(service, task, successHandler, exitHandler);
	service->post(boost::bind(wrappedTask, taskHandler));
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
		exitHandler_(this);
}

///////////////////////////////////////////////////////////////////////////////

TaskHandlerP TaskHandler::startChild(const TaskFunc& task, const SuccessFunc& successHandler, 
	const ExitFunc& exitHandler)
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
			service_->post(boost::bind(taskHandler->task_, taskHandler));
		}
		else
		{
			taskPosted_ = false;
		}

		service_.setCurrentTask(*taskHandler);
		task(taskHandler);
	};

	TaskHandlerP taskHandler = std::make_shared<TaskHandler>(service_, shared_from_this(), wrappedTask, successHandler, exitHandler);

	{
		boost::mutex::scoped_lock lock(childrenActivityMutex_);

		if (taskPosted_)
		{
			taskHandler->selfShared_ = taskHandler;
			waitingTasks_.push_back(*taskHandler);
		}
		else
		{
			service_->post(boost::bind(wrappedTask, taskHandler));
			taskPosted_ = true;
		}
	}

	return taskHandler;
}

///////////////////////////////////////////////////////////////////////////////

Service& TaskHandler::getService()
{
	return service_;
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

bool TaskHandler::isChildOf(TaskHandler& handler) const
{
	if (parentTaskHandler_.get() == &handler)
		return true;
	else
	{
		if (parentTaskHandler_)
			return parentTaskHandler_->isChildOf(handler);
		else
			return false;
	}
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
