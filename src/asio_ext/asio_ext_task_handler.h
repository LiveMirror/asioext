#pragma once

#include "asio_ext_config.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

namespace AsioExt
{

class TaskHandler;
typedef boost::shared_ptr<TaskHandler> TaskHandlerP;

///////////////////////////////////////////////////////////////////////////////

class TaskHandler 
	: boost::noncopyable
	, public boost::enable_shared_from_this<TaskHandler>
{
	typedef boost::function<void ()> VoidFunc;
	typedef boost::function<void (TaskHandlerP)> TaskFunc;

	basio::io_service& service_;
	TaskHandlerP parentTaskHandler_;
	TaskFunc task_;
	VoidFunc successHandler_;
	VoidFunc exitHandler_;
	bool (TaskHandler::*abortedFunc_)() const;
	
private:
	TaskHandler(basio::io_service& service, TaskHandlerP parentTaskHandler, 
		const TaskFunc& task, const VoidFunc& successHandler, const VoidFunc& exitHandler);

	TaskHandler(basio::io_service& service, const TaskFunc& task, const VoidFunc& successHandler, 
		const VoidFunc& exitHandler);

public:
	static TaskHandlerP start(basio::io_service& service, const TaskFunc& task,
		const VoidFunc& successHandler = VoidFunc(), const VoidFunc& exitHandler = VoidFunc());

	~TaskHandler();

	TaskHandlerP startChild(const TaskFunc& task, const VoidFunc& successHandler = VoidFunc(), 
		const VoidFunc& exitHandler = VoidFunc());

	bool aborted() const;
	void abort();

private:
	bool _notAborted() const;
	bool _aborted() const;
	bool _abortedFromParent() const;
};

///////////////////////////////////////////////////////////////////////////////
}
