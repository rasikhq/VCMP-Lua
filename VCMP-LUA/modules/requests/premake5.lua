project "module-cpr"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	pic "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"cpr/include/**.h",
		"cpr/src/**.cpp",
	}

	includedirs
	{
		"cpr/include"
	}
	
	libdirs
	{
		"cpr/lib"
	}

	defines "CURL_STATICLIB"

	links {"cpr.lib", "mongoose.lib"}