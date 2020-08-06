#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>

#include <vcmp.h>
#include <SimpleIni.h>

#include "lua/lua.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#endif //PCH_H
