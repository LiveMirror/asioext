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
	typedef boost::function<void ()> Handler;

	basio::io_service& service_;
	TaskHandlerP parentTaskHandler_;
	Handler successHandler_;
	Handler exitHandler_;
	bool (TaskHandler::*abortedFunc_)() const;
	
private:
	TaskHandler(basio::io_service& service, TaskHandlerP parentTaskHandler, 
		const Handler& successHandler, const Handler& exitHandler);

	TaskHandler(basio::io_service& service, const Handler& successHandler, const Handler& exitHandler);

public:
	static TaskHandlerP create(basio::io_service& service, const Handler& successHandler = Handler(), 
		const Handler& exitHandler = Handler());

	~TaskHandler();

	TaskHandlerP childHandler(const Handler& successHandler = Handler(), 
		const Handler& exitHandler = Handler());

	void post(Handler handler);

	bool aborted() const;
	void abort();

private:
	bool _notAborted() const;
	bool _aborted() const;
	bool _abortedFromParent() const;
};

///////////////////////////////////////////////////////////////////////////////
}
