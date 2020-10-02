workspace "VCMPLua"
	architecture "x86_64"
	startproject "LuaPlugin"

	configurations
	{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["Lua"] = "%{wks.location}/VCMP-Lua/vendor/Lua/include"

include "VCMP-LUA/vendor/Lua"
include "VCMP-LUA"