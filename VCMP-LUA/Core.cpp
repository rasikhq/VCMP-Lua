#include "pch.h"
#include "include/utility.h"
#include "vcmpWrap/globalTables.h"
#include "vcmpWrap/vcmpCallbacks.h"

PluginFuncs* g_Funcs;
PluginCallbacks* g_Calls;
PluginInfo* g_Info;

sol::state Lua;

static std::vector<std::string> s_ScriptFiles;

void reload_scripts();
void LoadLuaModule(std::string);

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

	pluginInfo->pluginVersion = 0x2400;
	pluginInfo->apiMajorVersion = PLUGIN_API_MAJOR;
	pluginInfo->apiMinorVersion = PLUGIN_API_MINOR;

	CSimpleIni conf(false, true, false);
	SI_Error ini_ret = conf.LoadFile("luaconfig.ini");

	if (ini_ret < 0) {
		switch (ini_ret) {
			case SI_FAIL:
				spdlog::error("Failed to load LUA config file");
				break;

			case SI_NOMEM:
				spdlog::error("Failed to load LUA config file (No memory)");
				break;

			case SI_FILE:
				spdlog::error("Failed to load LUA config file: {}", std::strerror(errno));
				break;

			default: spdlog::error("Failed to load LUA config file (unknown)");
		}
		return false;
	}

	Lua.open_libraries();

	Logger::Init(&Lua, spdlog::level::info); // Set level to info to avoid debug messages or trace to get internal debug messages

	//luaopen_lanes_embedded(Lua.lua_state(), NULL);

	//Lua.set_exception_handler(&my_exception_handler);

	bool experimental_mode = false;

	// Load Configuration
	{
		std::list<CSimpleIniA::Entry> configOptions;
		if (conf.GetAllKeys("config", configOptions) && configOptions.size() > 0) {
			for (const auto& setting : configOptions) {
				if (strcmp(setting.pItem, "loglevel") == 0) {
					const auto value = conf.GetValue("config", setting.pItem);
					try {
						unsigned short loglevel = std::stoi(value);
						spdlog::info("Reading logging level as: {}", loglevel);
						Logger::setLevel(loglevel);
					}
					catch (const std::exception e) { spdlog::error("Invalid logging level!"); }
				}
				else if (strcmp(setting.pItem, "logfile") == 0) {
					Logger::setFile(conf.GetValue("config", setting.pItem), 0, 0);
				}
				else if (strcmp(setting.pItem, "experimental") == 0) {
					experimental_mode = static_cast<bool>(conf.GetValue("config", setting.pItem));
				}
			}
		}
		else spdlog::warn("No configuration settings supplied, using defaults");
	}

	if (experimental_mode) {
		Lua["__experimental__"] = true;
		spdlog::warn("Experimental features may be really unstable, be very careful when using them.");
		Lua["__reload_scripts"] = &reload_scripts;
	}

	RegisterClasses(&Lua);
	RegisterVCMPCallbacks();

	// Load Modules
	{
		std::list<CSimpleIniA::Entry> modules;
		if(conf.GetAllKeys("modules", modules) && modules.size() > 0) {
			for(const auto& module : modules)
				if(std::string(conf.GetValue("modules", module.pItem)) == "true")
					LoadLuaModule(module.pItem);
		}
		else spdlog::info("No lua modules defined");
	}

	// Load Scripts
	{
		std::list<CSimpleIniA::Entry> scripts;
		if (conf.GetAllValues("scripts", "script", scripts) && scripts.size() > 0) {
			for (auto it = scripts.begin(); it != scripts.end(); it++) {
				auto result = Lua.safe_script_file(it->pItem, sol::script_pass_on_error);
				s_ScriptFiles.emplace_back(it->pItem);
				if (!result.valid()) {
					sol::error e = result;
					spdlog::error("Failed to load script: {}", e.what());
				}
			}
		}
		else spdlog::error("No Lua scripts specified to load");
	}

	return 1;
}

void reload_scripts()
{
	spdlog::warn("Reloading scripts...");

	// Re-trigger some crucial events which simulate a 'restart'
	EventManager::Trigger("onServerShutdown");
	// Reset the event handlers to avoid duplication
	EventManager::Reset();

	for (const auto& scriptFile : s_ScriptFiles)
	{
		spdlog::info("Parsing script: {}", scriptFile);
		auto result = Lua.safe_script_file(scriptFile, sol::script_pass_on_error);
		if (!result.valid()) {
			sol::error e = result;
			spdlog::error("Failed to load script: {}", e.what());
		}
	}

	// Re-trigger some crucial events which simulate a 'restart'
	EventManager::Trigger("onServerInit");
}

enum LuaModules {
	Invalid = 0,
	Lanes
};

static std::unordered_map<std::string, LuaModules> s_LuaModules = {
	{"lanes", LuaModules::Lanes}
};

void LoadLuaModule(std::string name) {
	LuaModules module = s_LuaModules.find(name) != s_LuaModules.end() ? s_LuaModules[name] : LuaModules::Invalid;
	
	spdlog::info("Loading Lua module: {}", name);

	switch(module) {
		case Lanes:
		luaopen_lanes_embedded(Lua.lua_state(), NULL);
		break;
	}
}