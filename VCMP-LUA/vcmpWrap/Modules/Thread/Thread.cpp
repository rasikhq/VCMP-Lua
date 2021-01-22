#include "Thread.h"

extern sol::state Lua;

void Thread::Init(sol::state* Lua)
{
	if (!::Lua.get_or("__experimental__", false)) // Thread class is experimental only for now
		return;

	sol::usertype<Thread> userdata = Lua->new_usertype<Thread>("Thread", sol::constructors<Thread()>());

	userdata["run"] = &Thread::run;
	userdata["wait"] = &Thread::wait;
	userdata["get"] = &Thread::get;
	userdata["ready"] = &Thread::isReady;
}

Thread::Thread()
{
}

void Thread::run(sol::function f)
{
	task = async::spawn([f]() -> sol::object {
		if (f.valid())
		{
			auto value = f.call();
			if (value.valid())
				return value.get<sol::object>();
			else
			{
				spdlog::error("Thread::run >> Failed to run worker function: {}", value.status());
				return sol::nil;
			}
		}
		else
			return sol::nil;
	});
}

bool Thread::wait()
{
	if (!task.valid())
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
	if (!task.valid())
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
	if (task.valid() && task.ready())
		return true;
	return false;
}
