#include "Player.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

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

Player::Player(int32_t id)
	: m_ID(id) {

	g_Funcs->GetPlayerName(id, m_Name, sizeof(m_Name));
}

int32_t Player::getID() const {
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

/*** COMMON PROPERTIES ***/
sol::as_table_t<std::vector<float>> Player::getPosition() const
{
	float x, y, z;
	g_Funcs->GetPlayerPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}

void Player::setPosition(sol::table position)
{
	g_Funcs->SetPlayerPosition(m_ID, position[1], position[2], position[3]);
}

float Player::getRotation() const
{
	return g_Funcs->GetPlayerHeading(m_ID);
}

void Player::setRotation(float angle)
{
	g_Funcs->SetPlayerHeading(m_ID, angle);
}

/******/

void Player::Init(sol::state* L) {
	sol::usertype<Player> userdata = L->new_usertype<Player>("Player");

	userdata["type"] = &Player::getStaticType;
	userdata["findByID"] = &Player::Get;

	/*** READ-ONLY ***/
	userdata.set("getType", &Player::getType);
	userdata.set("getID", &Player::getID);
	userdata.set("getIP", &Player::getIP);
	userdata.set("getUID", &Player::getUID);
	userdata.set("getUID2", &Player::getUID2);
	userdata.set("getKey", &Player::getKey);
	userdata.set("getState", &Player::getState);
	userdata.set("getUniqueWorld", &Player::getUniqueWorld);
	userdata.set("getClass", &Player::getClass);
	userdata.set("isOnline", &Player::isOnline);
	userdata.set("isSpawned", &Player::isSpawned);
	userdata.set("isTyping", &Player::isTyping);
	userdata.set("getPing", &Player::getPing);
	userdata.set("getFPS", &Player::getFPS);

	/*** METHODS ***/
	userdata["msg"] = &Player::msg;

	/*** PROPERTIES ***/
	userdata["admin"] = sol::property(&Player::getAdmin, &Player::setAdmin);
	userdata["world"] = sol::property(&Player::getWorld, &Player::setWorld);
	userdata["secondaryWorld"] = sol::property(&Player::getSecWorld, &Player::setSecWorld);
	userdata["team"] = sol::property(&Player::getTeam, &Player::setTeam);
	userdata["skin"] = sol::property(&Player::getSkin, &Player::setSkin);
	userdata["color"] = sol::property(&Player::getColor, &Player::setColor);
	userdata["cash"] = sol::property(&Player::getCash, &Player::setCash);
	userdata["score"] = sol::property(&Player::getScore, &Player::setScore);
	userdata["wantedLevel"] = sol::property(&Player::getWantedLevel, &Player::setWantedLevel);
	userdata["health"] = sol::property(&Player::getHP, &Player::setHP);
	userdata["armour"] = sol::property(&Player::getArmour, &Player::setArmour);
	userdata["name"] = sol::property(&Player::getName, &Player::setName);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["position"] = sol::property(&Player::getPosition, &Player::setPosition);
	userdata["angle"] = sol::property(&Player::getRotation, &Player::setRotation);
}