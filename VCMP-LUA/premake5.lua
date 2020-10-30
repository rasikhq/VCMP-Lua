project "LuaPlugin"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	pic "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")
    
	files
	{
		"pch.h",
        	"pch.cpp",
        	"Core.cpp",

        	"include/**.h",
        	"include/**.c",
        
        	"vcmpWrap/**.h",
		"vcmpWrap/**.cpp",
    	}
    
	includedirs
	{
		"%{wks.location}/VCMP-LUA/",
		"%{wks.location}/VCMP-LUA/include",
		"%{wks.location}/VCMP-LUA/vcmpWrap",
		"%{wks.location}/VCMP-LUA/vendor",
		"%{wks.location}/VCMP-LUA/vendor/Lua",
		"%{wks.location}/VCMP-LUA/vendor/sol",
		"%{wks.location}/VCMP-LUA/vendor/spdlog/include",
	}

	links { "spdlog", "Lua" }

	-- Specify modules
	include "modules/mariadb"

	filter "system:windows"
		systemversion "latest"
		defines { "WIN32" }
	  
	filter "configurations:Debug"
		defines {"_DEBUG"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines {"_RELEASE"}
		runtime "Release"
		optimize "on"