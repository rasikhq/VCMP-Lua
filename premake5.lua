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

	filter "configurations:*32"
		architecture "x86"

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	flags { "MultiProcessorCompile"	}
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "VCMP-LUA/vendor/Lua"
include "VCMP-LUA/vendor/spdlog"
include "VCMP-LUA"
