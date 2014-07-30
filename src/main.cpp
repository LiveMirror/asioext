#include <iostream>

#include "asio_ext\asio_ext_service.h"
#include "asio_ext\asio_ext_task_handler.h"

using namespace AsioExt;

///////////////////////////////////////////////////////////////////////////////
// simple example

void simpleGrandChildTask(TaskHandlerP handler)
{
	boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 10));
}

void simpleChildTask(TaskHandlerP handler)
{
	for (uint i = 0; i < 3; ++i)
	{
		handler->startChild(boost::bind(simpleGrandChildTask, _1));
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 5));
}

void simpleTask(TaskHandlerP handler)
{
	for (uint i = 0; i < 3; ++i)
	{
		handler->startChild(boost::bind(simpleChildTask, _1));
	}
}

void simpleExample()
{
	Service service;

	{
		TaskHandler::start(*service, boost::bind(simpleTask, _1),
			[] () { 
				// handle task tree finished here
		});
	}
}

///////////////////////////////////////////////////////////////////////////////
// sample with logging

boost::mutex printMutex;

void sampleGrandChildTask(TaskHandlerP handler, std::string caption)
{
	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " started" << std::endl;
	}

	for (uint i = 0; i < 10; ++i)
	{
		if (handler->aborted())
			return;

		boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 4));
	}

	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " finished" << std::endl;
	}
}

void sampleChildTask(TaskHandlerP handler, std::string caption)
{
	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " started" << std::endl;
	}

	for (uint i = 0; i < 3; ++i)
	{
		std::stringstream s;
		s << "\t" << caption << " - " << i;
		std::string childCaption = s.str();

		handler->startChild(
			boost::bind(sampleGrandChildTask, _1, s.str()),
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree OK" << std::endl;
			},
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree finished" << std::endl;
			});
	}

	for (uint i = 0; i < 10; ++i)
	{
		if (handler->aborted())
			return;

		boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 2));
	}

	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " finished" << std::endl;
	}
}

void sampleTask(TaskHandlerP handler, std::string caption)
{
	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " started" << std::endl;
	}

	for (uint i = 0; i < 3; ++i)
	{
		std::stringstream s;
		s << "\t" << caption << " - " << i;
		std::string childCaption = s.str();

		handler->startChild(
			boost::bind(sampleChildTask, _1, s.str()),
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree OK" << std::endl;
			},
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree finished" << std::endl;
			});
	}

	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << caption << " finished" << std::endl;
	}
}

void exampleWithLogging()
{
	for (uint i = 0; i < 2; ++i)
	{
		Service service;

		std::string caption = "task0";

		TaskHandlerP taskHandler = TaskHandler::start(*service,
			boost::bind(sampleTask, _1, caption),
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree OK" << std::endl;
			},
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree finished" << std::endl;
			});

		if (i > 0)
		{
			boost::shared_ptr<basio::deadline_timer> timer(new basio::deadline_timer(*service, boost::posix_time::milliseconds(10)));

			timer->async_wait([taskHandler, timer] (const boost::system::error_code&)
			{
				taskHandler->abort();
			});
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

int main(void)
{
	simpleExample();
	exampleWithLogging();	

	return 0;
}

///////////////////////////////////////////////////////////////////////////////