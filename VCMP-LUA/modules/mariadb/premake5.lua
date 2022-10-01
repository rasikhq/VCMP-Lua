files
{
	"include/**.hpp",
	"include/**.h",
	"src/**.hpp",
	"**.cpp"
}

includedirs
{
	"include",
	"include/mysql",
}

filter {"system:windows"}
	libdirs { "lib" }

-- filter {"system:windows", "Release32"}
-- 	links { "MariaDBClient32.lib" }

filter {"system:windows", "Release"}
	-- MySQL cancer crap keeps depending on dynamic libssl and libcrypto so even though I link with the static versions, it first looks for the DLLs causing an Error 126 /facepalm
	links { "mysqlclient.lib" }

filter {"system:not windows", "Release32"}
	linkoptions { "-L modules/mariadb/lib/mariadbclient32.a" }

filter {"system:not windows", "Release"}
	linkoptions { "-L modules/mariadb/lib/mariadbclient.a" }
	
filter {}