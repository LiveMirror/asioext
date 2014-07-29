#pragma once

#include "asio_ext_config.h"

#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

class Service : boost::noncopyable
{
	basio::io_service service_;
	boost::thread_group threads_;
	boost::shared_ptr<basio::io_service::work> dummyWork_;

public:
	Service(const uint numThreads = boost::thread::hardware_concurrency());
	~Service();

	basio::io_service* operator -> () { return &service_; }
	const basio::io_service* operator -> () const { return &service_; }
	basio::io_service& operator* () { return service_; }
	const basio::io_service& operator* () const { return service_; }

private:

	void threadFunc();
};

///////////////////////////////////////////////////////////////////////////////
}
