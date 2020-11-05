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

	char PasswordBuffer[128];
	g_Funcs->GetServerPassword(PasswordBuffer, sizeof(PasswordBuffer));

	settings["maxPlayers"] = s_Settings.maxPlayers;
	settings["port"] = s_Settings.port;
	settings["serverName"] = std::string(s_Settings.serverName);
	settings["serverPassword"] = std::string(PasswordBuffer);
	settings["flags"] = s_Settings.flags;

	return settings;
}

void Server::Init(sol::state* L) {
	sol::state& state = *L;
	sol::table ServerTable = L->create_table_with("getSettings", &Server::getSettings);
	ServerTable["getOption"] = &Server::getOption;
	ServerTable["setOption"] = &Server::setOption;

	ServerTable.set_function("setName", [](const std::string& newName) {
		g_Funcs->SetServerName(newName.c_str());
	});

	ServerTable.set_function("setPassword", [](const std::string& newName) {
		g_Funcs->SetServerPassword(newName.c_str());
	});

	state["Server"] = ServerTable;
}