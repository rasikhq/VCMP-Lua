project "LuaPlugin"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
    
	files
	{
		"**.h",
        "**.cpp",

        "include/**.h",
        "include/**.c",
        
		"sol/**.hpp",
        
        "vcmpWrap/**.h",
        "vcmpWrap/Classes/**.h",
        "vcmpWrap/Classes/**.cpp",
        "vcmpWrap/EventManager/**.cpp",
        "vcmpWrap/EventManager/**.h",
        "vcmpWrap/Log/**.h",
        "vcmpWrap/Timer/**.h",
        "vcmpWrap/Timer/**.cpp",
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

	defines { "SPDLOG_COMPILED_LIB" }

	links { "spdlog", "Lua" }

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines {"_DEBUG"}
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines {"_RELEASE"}
		runtime "Release"
		optimize "on"