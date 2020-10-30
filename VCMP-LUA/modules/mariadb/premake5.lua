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
	links { "MariaDBClient.lib" }

configuration "linux"
	filter "configurations:Release32"
		linkoptions { "-L modules/mariadb/lib/mariadbclient32.a" }
	filter "configurations:Release"
		linkoptions { "-L xmodules/mariadb/lib/mariadbclient.a" }
