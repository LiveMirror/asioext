#include "asio_ext_task_handler.h"

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

TaskHandler::TaskHandler(basio::io_service& service, TaskHandlerP parentTaskHandler, const Handler& successHandler, const Handler& exitHandler)
: service_(service)
, parentTaskHandler_(parentTaskHandler)
, successHandler_(successHandler)
, exitHandler_(exitHandler)
{
	if (parentTaskHandler)
		abortedFunc_ = &TaskHandler::_abortedFromParent;
}

///////////////////////////////////////////////////////////////////////////////

TaskHandler::TaskHandler(basio::io_service& service, const Handler& successHandler, const Handler& exitHandler)
: service_(service)
, successHandler_(successHandler)
, exitHandler_(exitHandler)
, abortedFunc_(&TaskHandler::_notAborted)
{
}

///////////////////////////////////////////////////////////////////////////////

TaskHandlerP TaskHandler::create(basio::io_service& service, const Handler& successHandler, 
	const Handler& exitHandler)
{
	return TaskHandlerP(new TaskHandler(service, successHandler, exitHandler));
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

TaskHandlerP TaskHandler::childHandler(const Handler& successHandler, const Handler& exitHandler)
{
	return TaskHandlerP(new TaskHandler(service_, shared_from_this(), successHandler, exitHandler));
}

///////////////////////////////////////////////////////////////////////////////

void TaskHandler::post(Handler handler)
{
	service_.post(handler);
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
