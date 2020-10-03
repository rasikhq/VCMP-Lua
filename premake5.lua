workspace "VCMPLua"
	architecture "x86_64"
	startproject "LuaPlugin"

	configurations
	{
		"Debug32",
		"Debug",
		"Release32",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	defines { "SPDLOG_COMPILED_LIB" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "VCMP-LUA/vendor/Lua"
include "VCMP-LUA/vendor/spdlog"
include "VCMP-LUA"