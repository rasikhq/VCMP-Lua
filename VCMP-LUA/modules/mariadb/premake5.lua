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
	libdirs { "//usr/lib/x86_64-linux-gnu" }
	links { "mariadbclient" }