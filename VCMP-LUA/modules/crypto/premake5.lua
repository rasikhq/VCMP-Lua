project "module-crypto"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin/interm/" .. outputdir .. "/%{prj.name}")

	files
	{
		"include/**.hpp",
		"src/**.cpp",
	}

	includedirs
	{
		"include",
	}

	filter "system:linux"
		buildoptions { "-g2", "-O2" }