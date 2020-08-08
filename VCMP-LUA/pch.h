#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <vcmp.h>
#include <SimpleIni.h>

#include "lua/lua.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#endif //PCH_H
