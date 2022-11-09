#include "Thread.h"

extern sol::state Lua;

Thread::Thread()
{
}

Thread::~Thread()
{
	destroy();
}

void Thread::run(sol::function f)
{
	auto& interruptToken = c;
	task = async::spawn([&interruptToken, f]() -> sol::object {
		if (f.valid())
		{
			async::interruption_point(interruptToken);

			auto value = f.call();

			if (value.valid())
				return value.get<sol::object>();
			else
			{
				spdlog::error("Thread::run >> Failed to run worker function");
				return sol::nil;
			}
		}
		else
			return sol::nil;
	});
}

bool Thread::wait()
{
	if (task.canceled() || c.is_canceled())
	{
		spdlog::error("Thread::get >> Task is destroyed/cancelled");
		return false;
	}
	else if (!task.valid())
	{
		spdlog::error("Thread::wait >> Task is invalid");
		return false;
	}
	else if (task.ready())
		return true;

	task.wait();
	return true;
}

sol::object Thread::get()
{
	if (task.canceled() || c.is_canceled())
	{
		spdlog::error("Thread::get >> Task is destroyed/cancelled");
		return sol::nil;
	}
	else if (!task.valid())
	{
		spdlog::error("Thread::get >> Task is invalid");
		return sol::nil;
	}
	else if (!task.ready())
	{
		spdlog::error("Thread::get >> Task is not ready");
		return sol::nil;
	}

	auto value = task.get();
	return value;
}

bool Thread::isReady()
{
	if (!task.canceled() && task.valid() && task.ready())
		return true;
	return false;
}

bool Thread::isCancelled()
{
	return c.is_canceled();
}

void Thread::destroy()
{
	c.cancel();
}

void Thread::Init(sol::state* Lua)
{
	if (!::Lua.get_or("__experimental__", false)) // Thread class is experimental only for now
		return;

	sol::usertype<Thread> userdata = Lua->new_usertype<Thread>("Thread", sol::constructors<Thread()>());

	userdata["run"] = &Thread::run;
	userdata["wait"] = &Thread::wait;
	userdata["get"] = &Thread::get;
	userdata["ready"] = &Thread::isReady;
	userdata["cancelled"] = &Thread::isCancelled;
	userdata["destroy"] = &Thread::destroy;
}