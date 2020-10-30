files
{
	"include/**.hpp",
}

includedirs
{
	"include",
	"include/mysql",
}

configuration "windows"
	libdirs { "lib" }
	links { "MariaDBClient.lib" }

configuration "linux"
	filter "configurations:*32"
		links { "libmariadbclient32.a" }

	filter "configurations:Release"
		links { "libmariadbclient.a" }
	--links { "mariadbclient" }
