#include "pch.h"
#include "include/utility.h"
#include "vcmpWrap/globalTables.h"
#include "vcmpWrap/vcmpCallbacks.h"

PluginFuncs* g_Funcs;
PluginCallbacks* g_Calls;
PluginInfo* g_Info;

sol::state Lua;

int my_exception_handler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) {
	std::cout << "An exception occurred in a function ";
	if (maybe_exception) {
		std::cout << "(straight from the exception): ";
		const std::exception& ex = *maybe_exception;
		std::cout << ex.what() << std::endl;
	}
	else {
		std::cout << "(from the description parameter): ";
		std::cout.write(description.data(), description.size());
		std::cout << std::endl;
	}
	return sol::stack::push(L, description);
}

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

	Lua.open_libraries();
	Lua.set_exception_handler(&my_exception_handler);

	RegisterClasses(&Lua);
	RegisterVCMPCallbacks();

	std::list<CSimpleIniA::Entry> scripts;
	if (conf.GetAllValues("scripts", "script", scripts) && scripts.size() > 0) {
		for (auto it = scripts.begin(); it != scripts.end(); it++) {
			auto result = Lua.safe_script_file(it->pItem, sol::script_pass_on_error);
			if (!result.valid()) {
				sol::error e = result;
				OutputError("Failed to load script: %s", e.what());
			}
		}
	}
	else {
		OutputError("No lua scripts specified to load");
	}

	return 1;
}