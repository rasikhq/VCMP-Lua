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
