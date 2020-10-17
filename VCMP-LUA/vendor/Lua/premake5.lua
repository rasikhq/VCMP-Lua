project "Lua"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	pic "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"**.h",
		"**.c"
	}
 
