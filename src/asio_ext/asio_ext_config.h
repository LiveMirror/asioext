#pragma once

#include <boost/asio.hpp>
#include <boost/function.hpp>

namespace AsioExt
{
///////////////////////////////////////////////////////////////////////////////

namespace basio = boost::asio;
typedef unsigned int uint;

class TaskHandler;
typedef boost::shared_ptr<TaskHandler> TaskHandlerP;

typedef boost::function<void ()> VoidFunc;
typedef boost::function<void (TaskHandlerP)> TaskFunc;
typedef boost::function<void ()> SuccessFunc;
typedef boost::function<void (TaskHandler*)> ExitFunc;

///////////////////////////////////////////////////////////////////////////////
}
