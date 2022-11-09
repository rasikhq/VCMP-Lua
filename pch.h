#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS
#undef _UNICODE

// Fixes some issues with pre-C++17
#define NOMINMAX

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <future>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>

#include <vcmp.h>
#include <SimpleIni.h>

#include <lua.h>

#define SOL_ALL_SAFETIES_ON 1

#include <sol/sol.hpp>

extern "C" 
{
	#include <lanes.h>
}

//#include <mariadb++/connection.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#endif //PCH_H
