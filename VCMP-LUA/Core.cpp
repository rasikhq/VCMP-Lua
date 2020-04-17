#include "pch.h"
#include "include/utility.h"
#include "vcmpWrap/globaTables.h"
#include "vcmpWrap/vcmpCallbacks.h"

PluginFuncs* g_Funcs;
PluginCallbacks* g_Calls;
PluginInfo* g_Info;

lua_State* Lua;

extern "C" EXPORT unsigned int VcmpPluginInit(PluginFuncs * pluginFuncs, PluginCallbacks * pluginCalls, PluginInfo * pluginInfo) {
	g_Funcs = pluginFuncs;
	g_Calls = pluginCalls;
	g_Info = pluginInfo;

	pluginInfo->pluginVersion = 0x1001;
	pluginInfo->apiMajorVersion = PLUGIN_API_MAJOR;
	pluginInfo->apiMinorVersion = PLUGIN_API_MINOR;


	CSimpleIni conf(false, true, false);
	SI_Error ini_ret = conf.LoadFile("luaconfig.ini");

	if (ini_ret < 0) {
		switch (ini_ret) {
			case SI_FAIL:
				OutputError("Failed to load LUA config file");
				break;

			case SI_NOMEM:
				OutputError("Failed to load LUA config file (No memory)");
				break;

			case SI_FILE:
				OutputError("Failed to load LUA config file: %s", std::strerror(errno));
				break;

			default: OutputError("Failed to load LUA config file (unknown)");
		}
		return false;
	}

	Lua = luaL_newstate();
	luaL_openlibs(Lua);

	luabridge::enableExceptions(Lua);

	RegisterClasses(Lua);
	RegisterVCMPCallbacks();

	std::list<CSimpleIniA::Entry> scripts;
	if (conf.GetAllValues("scripts", "script", scripts) && scripts.size() > 0) {
		for (auto it = scripts.begin(); it != scripts.end(); it++) {
			luaL_dofile(Lua, it->pItem);
		}
	}
	else {
		OutputError("No lua scripts speciefied to load");
	}

	return 1;
}