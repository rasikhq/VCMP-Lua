project "module-sqlite3"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	pic "On"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"sqlite3/**.h",
		"sqlite3/**.c",
	}

project "module-sqliteCpp"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"sqliteCpp/**.h",
		"sqliteCpp/**.cpp",
	}

	includedirs
	{
		"sqlite3",
		"sqliteCpp/include",
	}

	links {"module-sqlite3"}