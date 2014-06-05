#include <iostream>

#include "asio_ext\asio_ext_service.h"
#include "asio_ext\asio_ext_task_handler.h"
#include "asio_ext\asio_ext_task_shared_mutex.h"

using namespace AsioExt;

///////////////////////////////////////////////////////////////////////////////
// simple example

void simpleGrandChildTask(TaskHandlerP handler, int a, int b)
{
	//boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 10));
}

void simpleChildTask(TaskHandlerP handler, int a, int b)
{
	for (uint i = 0; i < 100; ++i)
	{
		handler->startChild(boost::bind(simpleGrandChildTask, _1, a + 1, b + 1));
	}

	//boost::this_thread::sleep(boost::posix_time::milliseconds(rand() % 5));
}

void simpleTask(TaskHandlerP handler, int a, int b)
{
	for (uint i = 0; i < 100; ++i)
	{
		handler->startChild(boost::bind(simpleChildTask, _1, a + 1, b + 1));
	}
}

void simpleExample()
{
	Service service;

	LARGE_INTEGER startTime;
	QueryPerformanceCounter(&startTime);

	TaskHandler::start(*service,
		[](AsioExt::TaskHandlerP task)
		{
			int a = 2;
			int b = 3;

			for (uint i = 0; i < 100; ++i)
			{
				task->startChild(
					boost::bind(simpleTask, _1, a++, b++), // task
					[] () { 
						// successfully finished
					},
					[] () {
						// all task tree finished
					});
			}
		},
		[startTime]()
		{
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			long elapsedTime = (long)(endTime.QuadPart - startTime.QuadPart);

			std::cout << "Elapsed time: " << elapsedTime << std::endl;
		});
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
					std::cout << childCaption << " tree SUCCESS <---" << std::endl;
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
					std::cout << childCaption << " tree SUCCESS <---" << std::endl;
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
	for (uint i = 1; i < 2; ++i)
	{
		Service service(8);

		std::string caption = "task0";

		TaskHandlerP taskHandler = TaskHandler::start(*service,
			boost::bind(sampleTask, _1, caption),
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree SUCCESS <---" << std::endl;
			},
			[caption] () {
				boost::mutex::scoped_lock lock(printMutex);
				std::cout << caption << " tree finished" << std::endl;
			});

		if (i > 0)
		{
			boost::shared_ptr<basio::deadline_timer> timer(new basio::deadline_timer(*service, boost::posix_time::milliseconds(100)));

			timer->async_wait([taskHandler, timer] (const boost::system::error_code&)
			{
				taskHandler->abort();
			});
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// shared mutex example

void readTask(TaskHandlerP taskHandler, int i)
{
	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << "Read " << i << std::endl;
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds((rand() % 100) + 500));
}

void writeTask(TaskHandlerP taskHandler, int i)
{
	{
		boost::mutex::scoped_lock lock(printMutex);
		std::cout << "Write " << i << std::endl;
	}

	boost::this_thread::sleep(boost::posix_time::milliseconds((rand() % 100) + 500));
}

void exampleWithSharedMutex()
{
	Service service;
	TaskSharedMutex mutex;

	for (int i = 0; i < 10; ++i)
		mutex.startShared(*service, boost::bind(readTask, _1, i));
	
	for (int i = 0; i < 10; ++i)
		mutex.start(*service, boost::bind(writeTask, _1, i));

	for (int i = 0; i < 10; ++i)
		mutex.startShared(*service, boost::bind(readTask, _1, i));

	for (int i = 0; i < 2; ++i)
		mutex.start(*service, boost::bind(writeTask, _1, i));
}

///////////////////////////////////////////////////////////////////////////////

int main(void)
{
	//simpleExample();
	//exampleWithLogging();	
	exampleWithSharedMutex();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////