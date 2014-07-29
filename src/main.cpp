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
		handler->post(boost::bind(simpleGrandChildTask, handler->childHandler()));
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 5));
}

void simpleTask(TaskHandlerP handler)
{
	for (uint i = 0; i < 3; ++i)
	{
		handler->post(boost::bind(simpleChildTask, handler->childHandler()));
	}
}

void simpleExample()
{
	Service service;

	{
		service->post(boost::bind(simpleTask, TaskHandler::create(*service,
			[] () { 
				// handle task tree finished here
		})));
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

		handler->post(boost::bind(sampleGrandChildTask, handler->childHandler(
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree OK" << std::endl;
			},
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree finished" << std::endl;
			}
		), s.str()));
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

		handler->post(boost::bind(sampleChildTask, handler->childHandler(
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree OK" << std::endl;
			},
			[childCaption] () {
					boost::mutex::scoped_lock lock(printMutex);
					std::cout << childCaption << " tree finished" << std::endl;
			}
		), s.str()));
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

		TaskHandlerP taskHandler = TaskHandler::create(*service,
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree OK" << std::endl;
			},
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree finished" << std::endl;
			});

		service->post(boost::bind(sampleTask, taskHandler, caption));

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