#include "Server.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

static ServerSettings s_Settings;

bool Server::getOption(vcmpServerOption option) {
	return g_Funcs->GetServerOption(option);
}

bool Server::setOption(vcmpServerOption option, bool toggle) {
	g_Funcs->SetServerOption(option, toggle);
	return true;
}

sol::object Server::getSettings() {
	if (g_Funcs->GetServerSettings(&s_Settings) != vcmpErrorNone) {
		spdlog::error("[ERROR] Failed to retrieve server settings from plugin!");
		return sol::nil;
	}
	sol::table settings = Lua.create_table();

	settings["maxPlayers"] = s_Settings.maxPlayers;
	settings["port"] = s_Settings.port;
	settings["serverName"] = std::string(s_Settings.serverName);
	settings["flags"] = s_Settings.flags;

	return settings;
}

void Server::Init(sol::state* L) {
	sol::state& state = *L;
	L->set("Server", L->create_table_with("getSettings", &Server::getSettings));
	state["Server"]["getOption"] = &Server::getOption;
	state["Server"]["setOption"] = &Server::setOption;
}