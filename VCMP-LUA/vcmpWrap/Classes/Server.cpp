#include "Server.h"

extern PluginFuncs* g_Funcs;
extern lua_State* Lua;

optionUMType Server::s_OptionCode = {
	{"DeathMessages",		vcmpServerOption::vcmpServerOptionDeathMessages},
	{"DriveBy",				vcmpServerOption::vcmpServerOptionDisableDriveBy},
	{"DriveOnWater",		vcmpServerOption::vcmpServerOptionDriveOnWater},
	{"FastSwitch",			vcmpServerOption::vcmpServerOptionFastSwitch},
	{"FlyingCars",			vcmpServerOption::vcmpServerOptionFlyingCars},
	{"FrameLimit",			vcmpServerOption::vcmpServerOptionFrameLimiter},
	{"FriendlyFire",		vcmpServerOption::vcmpServerOptionFriendlyFire},
	{"JoinMessages",		vcmpServerOption::vcmpServerOptionJoinMessages},
	{"JumpSwitch",			vcmpServerOption::vcmpServerOptionJumpSwitch},
	{"ShootInAir",			vcmpServerOption::vcmpServerOptionShootInAir},
	{"Nametags",			vcmpServerOption::vcmpServerOptionShowNameTags},
	{"TeamMarkersOnly",		vcmpServerOption::vcmpServerOptionOnlyShowTeamMarkers},
	{"StuntBike",			vcmpServerOption::vcmpServerOptionStuntBike},
	{"SyncFrameLimiter",	vcmpServerOption::vcmpServerOptionSyncFrameLimiter},
	{"TaxiBoostJump",		vcmpServerOption::vcmpServerOptionTaxiBoostJump},
	{"WallGlitch",			vcmpServerOption::vcmpServerOptionWallGlitch},
	{"DeathMessages",		vcmpServerOption::vcmpServerOptionDeathMessages},
	{"Classes",				vcmpServerOption::vcmpServerOptionUseClasses},
	{"ChatTags",			vcmpServerOption::vcmpServerOptionChatTagsEnabled},
	{"BackfaceCulling",		vcmpServerOption::vcmpServerOptionDisableBackfaceCulling},
	{"HeliBladeDamage",		vcmpServerOption::vcmpServerOptionDisableHeliBladeDamage},
	{"PerfectHandling",		vcmpServerOption::vcmpServerOptionPerfectHandling},
	{"Markers",				vcmpServerOption::vcmpServerOptionShowMarkers}
};

static ServerSettings s_Settings;

bool Server::getOption(const std::string& option) {
	if (s_OptionCode.find(option) != s_OptionCode.end()) {
		return g_Funcs->GetServerOption(s_OptionCode[option]);
	}
	return false;
}

bool Server::setOption(const std::string& option, bool toggle) {
	if (s_OptionCode.find(option) != s_OptionCode.end()) {
		g_Funcs->SetServerOption(s_OptionCode[option], toggle);
		return true;
	}
	return false;
}

luabridge::LuaRef Server::getSettings() {
	if (g_Funcs->GetServerSettings(&s_Settings) != vcmpErrorNone) {
		std::cout << "[ERROR] Failed to retrieve server settings from plugin!" << std::endl;
		return luabridge::LuaRef(Lua);
	}
	luabridge::LuaRef settings(Lua, luabridge::newTable(Lua));

	settings["maxPlayers"] = s_Settings.maxPlayers;;
	settings["port"] = s_Settings.port;
	settings["serverName"] = std::string(s_Settings.serverName);

	return settings;
}

void Server::Init(lua_State* L) {
	luabridge::getGlobalNamespace(L)
		.beginNamespace("Server")

		.addFunction("getOption", Server::getOption)
		.addFunction("setOption", Server::setOption)

		.addFunction("getSettings", Server::getSettings)

		.endNamespace();
}