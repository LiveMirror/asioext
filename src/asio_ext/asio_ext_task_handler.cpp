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
	TaskHandlerP taskHandler(new TaskHandler(service_, shared_from_this(), task, successHandler, exitHandler));
	service_.post(boost::bind(task, taskHandler));
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
