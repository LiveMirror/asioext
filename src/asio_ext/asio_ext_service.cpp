#include "asio_ext_service.h"

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

Service::Service(const uint numThreads)
: dummyWork_(new basio::io_service::work(service_))
{
	assert(numThreads > 0);

	for (uint threadIndex = numThreads; threadIndex--; )
	{
		boost::thread* thread = new boost::thread(boost::bind(&Service::threadFunc, this));

		{
			boost::mutex::scoped_lock lock(currentTasksMutex_);
			currentTasks_.insert(std::make_pair(thread->get_id(), (TaskHandler*)NULL));
		}

		threads_.add_thread(thread);
	}
}

///////////////////////////////////////////////////////////////////////////////

Service::~Service()
{
	dummyWork_.reset();
	threads_.join_all();
}

///////////////////////////////////////////////////////////////////////////////

void Service::setCurrentTask(TaskHandler& handler)
{
	boost::mutex::scoped_lock lock(currentTasksMutex_);
	auto it = currentTasks_.find(boost::this_thread::get_id());

	assert(it != currentTasks_.end());
	it->second = &handler;
}

///////////////////////////////////////////////////////////////////////////////

void Service::continueTask(TaskHandler* handler)
{
	boost::mutex::scoped_lock lock(currentTasksMutex_);
	auto it = currentTasks_.find(boost::this_thread::get_id());

	if (it != currentTasks_.end())
	{
		assert(!it->second || !handler);
		it->second = handler;
	}
	else
	{
		currentTasks_.insert(std::make_pair(boost::this_thread::get_id(), handler));
	}
}

///////////////////////////////////////////////////////////////////////////////

TaskHandler* Service::getCurrentTask()
{
	boost::mutex::scoped_lock lock(currentTasksMutex_);
	auto it = currentTasks_.find(boost::this_thread::get_id());

	if (it != currentTasks_.end())
	{
		assert(it->second);
		return it->second;
	}
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////

void Service::threadFunc()
{
	service_.run();
}

///////////////////////////////////////////////////////////////////////////////
}