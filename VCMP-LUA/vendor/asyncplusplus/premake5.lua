project "asyncplusplus"
	kind "StaticLib"
	language "C++"
	cppdialect "C++11"
	pic "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files 
	{ 
		"include/**.h",
		"src/**.h", "src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/VCMP-LUA/vendor/asyncplusplus/include"
	}
	
	defines { "LIBASYNC_STATIC" }
