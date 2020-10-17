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
	links { "mariadbclient" }
