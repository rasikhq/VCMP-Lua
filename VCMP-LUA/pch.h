#ifndef PCH_H
#define PCH_H

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <vector>
#include <unordered_map>
#include <sstream>

#include <vcmp.h>
#include <SimpleIni.h>

#if defined(X86)
#include "lua/x32/include/lua.hpp"
#elif defined(X64)
#include "lua/x64/include/lua.hpp"
#endif

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#endif //PCH_H
