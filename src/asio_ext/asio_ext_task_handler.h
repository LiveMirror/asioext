#pragma once

#include "asio_ext_config.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

namespace AsioExt
{

///////////////////////////////////////////////////////////////////////////////

class TaskHandler 
	: boost::noncopyable
	, public boost::enable_shared_from_this<TaskHandler>
	, public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
{
	TaskHandlerP selfShared_; // to prevent destruction while in intrusive list

	basio::io_service& service_;
	TaskHandlerP parentTaskHandler_;
	TaskFunc task_;
	VoidFunc successHandler_;
	VoidFunc exitHandler_;
	bool (TaskHandler::*abortedFunc_)() const; // does need to be mutexed?

	boost::mutex childrenActivityMutex_;
	bool taskPosted_;
	boost::intrusive::list<TaskHandler, boost::intrusive::constant_time_size<false>> waitingTasks_;
	
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
