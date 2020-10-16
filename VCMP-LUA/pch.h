#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS
#undef _UNICODE

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <future>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <vcmp.h>
#include <SimpleIni.h>

#include <Lua/lua.hpp>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <mariadb++/connection.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#endif //PCH_H
