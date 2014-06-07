#pragma once

#include "asio_ext_config.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

namespace AsioExt
{
class Service;

///////////////////////////////////////////////////////////////////////////////

class TaskHandler 
	: boost::noncopyable
	, public boost::enable_shared_from_this<TaskHandler>
	, public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>
{
	TaskHandlerP selfShared_; // to prevent destruction while in intrusive list

	Service& service_;
	TaskHandlerP parentTaskHandler_;
	TaskFunc task_;
	SuccessFunc successHandler_;
	ExitFunc exitHandler_;
	bool (TaskHandler::*abortedFunc_)() const; // does need to be mutexed?

	boost::mutex childrenActivityMutex_;
	bool taskPosted_;
	boost::intrusive::list<TaskHandler, boost::intrusive::constant_time_size<false>> waitingTasks_;
	
private:
	TaskHandler(Service& service, TaskHandlerP parentTaskHandler, 
		const TaskFunc& task, const SuccessFunc& successHandler, const ExitFunc& exitHandler);

	TaskHandler(Service& service, const TaskFunc& task, const SuccessFunc& successHandler, 
		const ExitFunc& exitHandler);

public:
	static TaskHandlerP start(Service& service, const TaskFunc& task,
		const SuccessFunc& successHandler = SuccessFunc(), const ExitFunc& exitHandler = ExitFunc());

	~TaskHandler();

	TaskHandlerP startChild(const TaskFunc& task, const SuccessFunc& successHandler = SuccessFunc(), 
		const ExitFunc& exitHandler = ExitFunc());

	Service& getService();

	bool aborted() const;
	void abort();

	bool isChildOf(TaskHandler& handler) const;

private:
	bool _notAborted() const;
	bool _aborted() const;
	bool _abortedFromParent() const;
};

///////////////////////////////////////////////////////////////////////////////
}
