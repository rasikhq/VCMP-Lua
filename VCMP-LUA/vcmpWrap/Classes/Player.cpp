#include "Player.h"

extern PluginFuncs* g_Funcs;
extern lua_State* Lua;

std::vector<Player> Player::s_Players;

Player* Player::Register(int32_t id) {
	s_Players.emplace_back(id);
	return &s_Players.back();
}

Player* Player::Get(int32_t id) {
	for (auto& player : s_Players) {
		if (player.m_ID == id)
			return &player;
	}
	return nullptr;
}

void Player::Unregister(Player* player) {
	for (auto it = s_Players.begin(); it != s_Players.end(); it++) {
		if (it->m_ID == player->getID()) {
			s_Players.erase(it);
			break;
		}
	}
}

const char* Player::getStaticType() { return "Player"; }

Player Player::findByID(int32_t id) {
	luabridge::LuaRef player(Lua);
	Player* playerPtr = Get(id);
	if (playerPtr != nullptr) {
		player = *playerPtr;
		return player;
	}
	return player;
}

Player::Player(int32_t id)
	: m_ID(id) {

	g_Funcs->GetPlayerName(id, m_Name, sizeof(m_Name));
}

const char* Player::getType() { return "Player"; }

int32_t Player::getID() {
	return m_ID;
}

bool Player::isOnline() {
	return static_cast<bool>(g_Funcs->IsPlayerConnected(m_ID));
}

bool Player::getAdmin() const {
	return static_cast<bool>(g_Funcs->IsPlayerAdmin(m_ID));
}

void Player::setAdmin(bool toggle) {
	g_Funcs->SetPlayerAdmin(m_ID, static_cast<bool>(toggle));
}

std::string Player::getIP() {
	char buffer[16];
	g_Funcs->GetPlayerIP(m_ID, buffer, sizeof(buffer));
	return std::string(buffer);
}

std::string Player::getUID() {
	char buffer[128];
	g_Funcs->GetPlayerUID(m_ID, buffer, sizeof(buffer));
	return std::string(buffer);
}

std::string Player::getUID2() {
	char buffer[128];
	g_Funcs->GetPlayerUID2(m_ID, buffer, sizeof(buffer));
	return std::string(buffer);
}

int32_t Player::getKey() {
	return g_Funcs->GetPlayerKey(m_ID);
}

int32_t Player::getState() {
	return g_Funcs->GetPlayerState(m_ID);
}

int32_t Player::getUniqueWorld() {
	return g_Funcs->GetPlayerUniqueWorld(m_ID);
}

int32_t Player::getClass() {
	return g_Funcs->GetPlayerClass(m_ID);
}

bool Player::isSpawned() {
	return static_cast<bool>(g_Funcs->IsPlayerSpawned(m_ID));
}

bool Player::isTyping() {
	return static_cast<bool>(g_Funcs->IsPlayerTyping(m_ID));
}

int32_t Player::getPing() {
	return g_Funcs->GetPlayerPing(m_ID);
}

float Player::getFPS() {
	return (float) g_Funcs->GetPlayerFPS(m_ID);
}

int32_t Player::getWorld() const {
	return g_Funcs->GetPlayerWorld(m_ID);
}

void Player::setWorld(int32_t value) {
	g_Funcs->SetPlayerWorld(m_ID, value);
}

int32_t Player::getSecWorld()const {
	return g_Funcs->GetPlayerSecondaryWorld(m_ID);
}

void Player::setSecWorld(int32_t value) {
	g_Funcs->SetPlayerSecondaryWorld(m_ID, value);
}

int32_t Player::getTeam() const {
	return g_Funcs->GetPlayerTeam(m_ID);
}

void Player::setTeam(int32_t value) {
	g_Funcs->SetPlayerTeam(m_ID, value);
}

int32_t Player::getSkin() const {
	return g_Funcs->GetPlayerSkin(m_ID);
}

void Player::setSkin(int32_t value) {
	g_Funcs->SetPlayerSkin(m_ID, value);
}

int32_t Player::getColor() const {
	return g_Funcs->GetPlayerColour(m_ID);
}

void Player::setColor(int32_t value) {
	g_Funcs->SetPlayerColour(m_ID, value);
}

int32_t Player::getCash() const {
	return g_Funcs->GetPlayerMoney(m_ID);
}

void Player::setCash(int32_t value) {
	g_Funcs->SetPlayerMoney(m_ID, value);
}

int32_t Player::getScore() const {
	return g_Funcs->GetPlayerScore(m_ID);
}

void Player::setScore(int32_t value) {
	g_Funcs->SetPlayerScore(m_ID, value);
}

int32_t Player::getWantedLevel() const {
	return g_Funcs->GetPlayerWantedLevel(m_ID);
}

void Player::setWantedLevel(int32_t value) {
	g_Funcs->SetPlayerWantedLevel(m_ID, value);
}

float Player::getHP() const {
	return g_Funcs->GetPlayerHealth(m_ID);
}

void Player::setHP(float value) {
	g_Funcs->SetPlayerHealth(m_ID, value);
}

float Player::getArmour() const {
	return g_Funcs->GetPlayerArmour(m_ID);
}

void Player::setArmour(float value) {
	g_Funcs->SetPlayerArmour(m_ID, value);
}

std::string Player::getName() const {
	return m_Name;
}

void Player::setName(const std::string& name) {
	strcpy(m_Name, name.c_str());
}

void Player::msg(const std::string& msg) {
	g_Funcs->SendClientMessage(m_ID, 0xFFFFFFFF, msg.c_str());
}

void Player::init(lua_State* L) {
	luabridge::getGlobalNamespace(L)
		.beginClass<Player>("Player")
		
		/*** Class properties (including common)***/
		.addStaticProperty("type", Player::getStaticType)
		.addFunction("type", &Player::getType)
		.addStaticFunction("findByID", Player::findByID)

		/*** Class object methods ***/
		.addFunction("getID", &Player::getID)
		.addFunction("getIP", &Player::getIP)
		.addFunction("getUID", &Player::getUID)
		.addFunction("getUID2", &Player::getUID2)
		.addFunction("getKey", &Player::getKey)
		.addFunction("getState", &Player::getState)
		.addFunction("getUniqueWorld", &Player::getUniqueWorld)
		.addFunction("getClass", &Player::getClass)
		.addFunction("isOnline", &Player::isOnline)
		.addFunction("isSpawned", &Player::isSpawned)
		.addFunction("isTyping", &Player::isTyping)
		.addFunction("getPing", &Player::getPing)
		.addFunction("getFPS", &Player::getFPS)

		.addFunction("msg", &Player::msg)

		/*** Class object properties ***/
		.addProperty("admin", &Player::getAdmin, &Player::setAdmin)
		.addProperty("world", &Player::getWorld, &Player::setWorld)
		.addProperty("secondaryWorld", &Player::getSecWorld, &Player::setSecWorld)
		.addProperty("team", &Player::getTeam, &Player::setTeam)
		.addProperty("skin", &Player::getSkin, &Player::setSkin)
		.addProperty("color", &Player::getColor, &Player::setColor)
		.addProperty("cash", &Player::getCash, &Player::setCash)
		.addProperty("score", &Player::getScore, &Player::setScore)
		.addProperty("wantedLevel", &Player::getWantedLevel, &Player::setWantedLevel)
		.addProperty("health", &Player::getHP, &Player::setHP)
		.addProperty("armour", &Player::getArmour, &Player::setArmour)
		.addProperty("name", &Player::getName, &Player::setName)

		.endClass();
}