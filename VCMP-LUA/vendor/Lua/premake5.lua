project "Lua"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	-- staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"**.h",
		"**.c"
	}
    
    filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"