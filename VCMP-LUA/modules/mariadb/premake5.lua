files
{
	"include/**.hpp",
}

includedirs
{
	"include",
	"include/mysql",
}

filter {"system:windows"}
	libdirs { "lib" }

filter {"system:windows", "Release32"}
	links { "MariaDBClient32.lib" }

filter {"system:windows", "Release"}
	links { "MariaDBClient.lib" }

filter {"system:not windows", "Release32"}
	linkoptions { "-L modules/mariadb/lib/mariadbclient32.a" }

filter {"system:not windows", "Release"}
	linkoptions { "-L modules/mariadb/lib/mariadbclient.a" }