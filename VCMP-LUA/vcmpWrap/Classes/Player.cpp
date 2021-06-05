#include "Player.h"
#include "Vehicle.h"
#include "Object.h"

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

sol::table Player::getActive(bool spawned)
{
	sol::table entities = Lua.create_table_with();
	for (const auto& entity : s_Players)
		if(spawned && g_Funcs->IsPlayerSpawned(entity.getID()))
			entities[entity.getID()] = &entity;
		else entities[entity.getID()] = &entity;
	return entities;
}

void Player::msgAll(const std::string& msg, sol::variadic_args args)
{
	for (auto& player : s_Players)
		player.msg(msg, args);
}

Player::Player(int32_t id)
	: m_ID(id), m_LuaData(Lua.create_table()) {

	g_Funcs->GetPlayerName(id, m_Name, sizeof(m_Name));
}

/*** METHODS ***/
void Player::msg(const std::string& msg, sol::variadic_args args) {
	int32_t color = args.size() > 0 ? args[0] : 0xFFFFFFFF;
	g_Funcs->SendClientMessage(m_ID, color, msg.c_str());
}

bool Player::getOption(vcmpPlayerOption option) const {
	return g_Funcs->GetPlayerOption(m_ID, option);
}

void Player::setOption(vcmpPlayerOption option, bool status) {
	g_Funcs->SetPlayerOption(m_ID, option, static_cast<uint8_t>(status));
}

bool Player::isPlayerStreamed(Player* player) const {
	if (player->isOnline()) {
		return g_Funcs->IsPlayerStreamedForPlayer(m_ID, player->getID());
	}
	return false;
}

void Player::forceSpawn() const
{
	g_Funcs->ForcePlayerSpawn(m_ID);
}

void Player::playSound(int32_t sound) const
{
	g_Funcs->PlaySound(getWorld(), sound, NAN, NAN, NAN);
}

void Player::playSound3D(int32_t sound) const
{
	float x, y, z;
	g_Funcs->GetPlayerPosition(m_ID, &x, &y, &z);
	g_Funcs->PlaySound(getWorld(), sound, x, y, z);
}

void Player::playSound3DEx(int32_t sound, sol::table position) const
{
	g_Funcs->PlaySound(getWorld(), sound, position[1], position[2], position[3]);
}

void Player::playSound3DEx2(int32_t sound, float x, float y, float z) const
{
	g_Funcs->PlaySound(getWorld(), sound, x, y, z);
}

int32_t Player::getID() const {
	return m_ID;
}

bool Player::isOnline() const {
	return static_cast<bool>(g_Funcs->IsPlayerConnected(m_ID));
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

int32_t Player::getKey() const {
	return g_Funcs->GetPlayerKey(m_ID);
}

int32_t Player::getState() const {
	return g_Funcs->GetPlayerState(m_ID);
}

int32_t Player::getUniqueWorld() const {
	return g_Funcs->GetPlayerUniqueWorld(m_ID);
}

int32_t Player::getClass() const {
	return g_Funcs->GetPlayerClass(m_ID);
}

bool Player::isSpawned() const {
	return static_cast<bool>(g_Funcs->IsPlayerSpawned(m_ID));
}

bool Player::isTyping() const {
	return static_cast<bool>(g_Funcs->IsPlayerTyping(m_ID));
}

bool Player::isCrouching() const {
	return static_cast<bool>(g_Funcs->IsPlayerCrouching(m_ID));
}

bool Player::isCameraLocked() const
{
	return g_Funcs->IsCameraLocked(m_ID);
}

int32_t Player::getPing() const {
	return g_Funcs->GetPlayerPing(m_ID);
}

int32_t Player::getAction() const
{
	return g_Funcs->GetPlayerAction(m_ID);
}

int32_t Player::getAmmo(int32_t slot) const
{
	return g_Funcs->GetPlayerAmmoAtSlot(m_ID, slot);
}

int32_t Player::getVehicleSlot() const
{
	return g_Funcs->GetPlayerInVehicleSlot(m_ID);
}

int32_t Player::getVehicleStatus() const
{
	return g_Funcs->GetPlayerInVehicleStatus(m_ID);
}

int32_t Player::getWeapon() const
{
	return g_Funcs->GetPlayerWeapon(m_ID);
}

sol::as_table_t<std::vector<float>> Player::getAimPos() const
{
	float x, y, z;
	g_Funcs->GetPlayerAimPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}

sol::as_table_t<std::vector<float>> Player::getAimDir() const
{
	float x, y, z;
	g_Funcs->GetPlayerAimDirection(m_ID, &x, &y, &z);
	std::vector<float> direction = { x, y, z };
	return sol::as_table(direction);
}

Object* Player::isStandingOnObject() const
{
	return Object::Get(g_Funcs->GetPlayerStandingOnObject(m_ID));
}

Vehicle* Player::isStandingOnVehicle() const
{
	return Vehicle::Get(g_Funcs->GetPlayerStandingOnVehicle(m_ID));;
}

float Player::getFPS() const {
	return (float)g_Funcs->GetPlayerFPS(m_ID);
}

void Player::getModules() const {
	g_Funcs->GetPlayerModuleList(m_ID);
}

void Player::setWeapon(int32_t weapon, int32_t ammo) const {
	g_Funcs->SetPlayerWeapon(m_ID, weapon, ammo);
}

void Player::giveWeapon(int32_t weapon, int32_t ammo) const {
	g_Funcs->GivePlayerWeapon(m_ID, weapon, ammo);
}

void Player::setAnimationCompact(int32_t animation) const
{
	g_Funcs->SetPlayerAnimation(m_ID, 0, animation);
}

void Player::setAnimation(int32_t group, int32_t animation) const
{
	g_Funcs->SetPlayerAnimation(m_ID, group, animation);
}

void Player::redirect(const std::string& ip, uint32_t port, const std::string& serverPassword) const
{
	g_Funcs->RedirectPlayerToServer(m_ID, ip.c_str(), port, getName().c_str(), serverPassword.c_str(), "");
}

void Player::restoreCamera() const
{
	g_Funcs->RestoreCamera(m_ID);
}

void Player::selectClass() const
{
	g_Funcs->ForcePlayerSelect(m_ID);
}

void Player::eject() const
{
	g_Funcs->RemovePlayerFromVehicle(m_ID);
}

void Player::setCam(sol::table position, sol::table lookAt) const
{
	if (position.size() < 3) {
		spdlog::error("Player::setCam: Invalid position!");
		return;
	}
	g_Funcs->SetCameraPosition(m_ID, position[1], position[2], position[3], lookAt.get_or<float>(1, position[1]), lookAt.get_or<float>(2, position[2]), lookAt.get_or<float>(3, position[3]));
}

void Player::setCamEx(float camX, float camY, float camZ, float lookX, float lookY, float lookZ) const
{
	g_Funcs->SetCameraPosition(m_ID, camX, camY, camZ, lookX, lookY, lookZ);
}

void Player::kick() const 
{
	g_Funcs->KickPlayer(m_ID);
}

void Player::ban() const
{
	g_Funcs->BanPlayer(m_ID);
}

/*** PROPERTIES ***/
bool Player::getAdmin() const {
	return static_cast<bool>(g_Funcs->IsPlayerAdmin(m_ID));
}

void Player::setAdmin(bool toggle) {
	if (getAdmin() == toggle)
		return;
	g_Funcs->SetPlayerAdmin(m_ID, static_cast<uint8_t>(toggle));
}

int32_t Player::getWorld() const {
	return g_Funcs->GetPlayerWorld(m_ID);
}

void Player::setWorld(int32_t value) {
	if (getWorld() == value)
		return;
	g_Funcs->SetPlayerWorld(m_ID, value);
}

int32_t Player::getSecWorld() const {
	return g_Funcs->GetPlayerSecondaryWorld(m_ID);
}

void Player::setSecWorld(int32_t value) {
	if (getSecWorld() == value)
		return;
	g_Funcs->SetPlayerSecondaryWorld(m_ID, value);
}

int32_t Player::getTeam() const {
	return g_Funcs->GetPlayerTeam(m_ID);
}

void Player::setTeam(int32_t value) {
	if (getTeam() == value)
		return;
	g_Funcs->SetPlayerTeam(m_ID, value);
}

int32_t Player::getSkin() const {
	return g_Funcs->GetPlayerSkin(m_ID);
}

void Player::setSkin(int32_t value) {
	if (getSkin() == value)
		return;
	g_Funcs->SetPlayerSkin(m_ID, value);
}

int32_t Player::getColor() const {
	return g_Funcs->GetPlayerColour(m_ID);
}

void Player::setColor(int32_t value) {
	if (getColor() == value)
		return;
	g_Funcs->SetPlayerColour(m_ID, value);
}

int32_t Player::getCash() const {
	return g_Funcs->GetPlayerMoney(m_ID);
}

void Player::setCash(int32_t value) {
	if (getCash() == value)
		return;
	g_Funcs->SetPlayerMoney(m_ID, value);
}

int32_t Player::getScore() const {
	return g_Funcs->GetPlayerScore(m_ID);
}

void Player::setScore(int32_t value) {
	if (getScore() == value)
		return;
	g_Funcs->SetPlayerScore(m_ID, value);
}

int32_t Player::getWantedLevel() const {
	return g_Funcs->GetPlayerWantedLevel(m_ID);
}

void Player::setWantedLevel(int32_t value) {
	if (getWantedLevel() == value)
		return;
	g_Funcs->SetPlayerWantedLevel(m_ID, value);
}

uint32_t Player::getImmunity() const {
	return g_Funcs->GetPlayerImmunityFlags(m_ID);
}

void Player::setImmunity(uint32_t value) {
	if (getImmunity() == value)
		return;
	g_Funcs->SetPlayerImmunityFlags(m_ID, value);
}

float Player::getHP() const {
	return g_Funcs->GetPlayerHealth(m_ID);
}

void Player::setHP(float value) {
	if (getHP() == value)
		return;
	g_Funcs->SetPlayerHealth(m_ID, value);
}

float Player::getArmour() const {
	return g_Funcs->GetPlayerArmour(m_ID);
}

void Player::setArmour(float value) {
	if (getArmour() == value)
		return;
	g_Funcs->SetPlayerArmour(m_ID, value);
}

std::string Player::getName() const {
	return m_Name;
}

void Player::setName(const std::string& name) {
	if (getName() == name)
		return;
	strcpy(m_Name, name.c_str());
}

Vehicle* Player::getVehicle() const {
	auto id = g_Funcs->GetPlayerVehicleId(m_ID);
	spdlog::debug("Player::getVehicle() :: id: {} :: vcmpError: {}", id, int(g_Funcs->GetLastError()));
	if (!id || g_Funcs->GetLastError() != vcmpError::vcmpErrorNone)
		return nullptr;
	spdlog::debug("Player::getVehicle() :: Returning Vehicle pointer");
	auto vehicle = Vehicle::Get(id);
	return vehicle;
}

void Player::setVehicle(Vehicle* vehicle) {
	g_Funcs->PutPlayerInVehicle(m_ID, vehicle->getID(), 0, 1, 1);
}

void Player::setVehicleWithSlot(Vehicle* vehicle, int32_t slot) {
	g_Funcs->PutPlayerInVehicle(m_ID, vehicle->getID(), slot, 1, 1);
}

int32_t Player::getWeaponSlot() const {
	return g_Funcs->GetPlayerWeaponSlot(m_ID);
}

void Player::setWeaponSlot(int32_t slot) {
	g_Funcs->SetPlayerWeaponSlot(m_ID, slot);
}

Player* Player::getSpectateTarget() const {
	auto id = g_Funcs->GetPlayerSpectateTarget(m_ID);
	return Player::Get(id);
}

void Player::setSpectateTarget(Player* target) {
	g_Funcs->SetPlayerSpectateTarget(m_ID, target->getID());
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
	if (position.size() < 3) return;
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
	s_Players.reserve(MAX_PLAYERS);

	sol::usertype<Player> userdata = L->new_usertype<Player>("Player",
		sol::meta_function::type, &Player::getStaticType,
		sol::meta_function::type_info, &Player::getStaticType
	);

	userdata["type"] = &Player::getStaticType;
	userdata["findByID"] = &Player::Get;
	userdata["count"] = []() { return s_Players.size(); };
	userdata["getActive"] = sol::overload([]() -> sol::table { return Player::getActive(false); }, &Player::getActive);
	userdata["msgAll"] = &Player::msgAll;

	/*** METHODS ***/
	userdata["msg"] = &Player::msg;
	userdata["getOption"] = &Player::getOption;
	userdata["setOption"] = &Player::setOption;
	userdata["isPlayerStreamed"] = &Player::isPlayerStreamed;
	userdata["forceSpawn"] = &Player::forceSpawn;
	userdata["playSound"] = &Player::playSound;
	userdata["playSound3D"] = sol::overload(&Player::playSound3D, &Player::playSound3DEx, &Player::playSound3DEx2);
	userdata["setWeapom"] = &Player::setWeapon;
	userdata["giveWeapon"] = &Player::giveWeapon;
	userdata["setVehicle"] = sol::overload(&Player::setVehicle, &Player::setVehicleWithSlot);
	userdata["redirect"] = &Player::redirect;
	userdata["setCamera"] = sol::overload(&Player::setCam, &Player::setCamEx);
	userdata["restoreCamera"] = &Player::restoreCamera;
	userdata["selectClass"] = &Player::selectClass;
	userdata["setAnimation"] = sol::overload(&Player::setAnimationCompact, &Player::setAnimation);
	userdata["eject"] = &Player::eject;

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
	userdata.set("isCrouching", &Player::isCrouching);
	userdata.set("getPing", &Player::getPing);
	userdata.set("getFPS", &Player::getFPS);
	userdata.set("getModules", &Player::getModules);

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
	userdata["immunity"] = sol::property(&Player::getImmunity, &Player::setImmunity);
	userdata["health"] = sol::property(&Player::getHP, &Player::setHP);
	userdata["armour"] = sol::property(&Player::getArmour, &Player::setArmour);
	userdata["name"] = sol::property(&Player::getName, &Player::setName);
	userdata["vehicle"] = sol::property(&Player::getVehicle, &Player::setVehicle);
	userdata["weaponSlot"] = sol::property(&Player::getWeaponSlot, &Player::setWeaponSlot);
	userdata["spectateTarget"] = sol::property(&Player::getSpectateTarget, &Player::setSpectateTarget);

	userdata["id"] = sol::property(&Player::getID);
	userdata["online"] = sol::property(&Player::isOnline);
	userdata["spawned"] = sol::property(&Player::isSpawned);
	userdata["typing"] = sol::property(&Player::isTyping);
	userdata["crouching"] = sol::property(&Player::isCrouching);
	userdata["cameraLocked"] = sol::property(&Player::isCameraLocked);
	userdata["ip"] = sol::property(&Player::getIP);
	userdata["uid"] = sol::property(&Player::getUID);
	userdata["uid2"] = sol::property(&Player::getUID2);
	userdata["key"] = sol::property(&Player::getKey);
	userdata["state"] = sol::property(&Player::getState);
	userdata["uniqueWorld"] = sol::property(&Player::getUniqueWorld);
	userdata["class"] = sol::property(&Player::getClass);
	userdata["ping"] = sol::property(&Player::getPing);
	userdata["fps"] = sol::property(&Player::getFPS);
	userdata["action"] = sol::property(&Player::getAction);
	userdata["ammo"] = sol::property(&Player::getAmmo);
	userdata["vehicleSlot"] = sol::property(&Player::getVehicleSlot);
	userdata["vehicleStatus"] = sol::property(&Player::getVehicleStatus);
	userdata["weapon"] = sol::property(&Player::getWeapon);
	userdata["aimDirection"] = sol::property(&Player::getAimDir);
	userdata["aimPosition"] = sol::property(&Player::getAimPos);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["data"] = &Player::m_LuaData;
	userdata["position"] = sol::property(&Player::getPosition, &Player::setPosition);
	userdata["angle"] = sol::property(&Player::getRotation, &Player::setRotation);
}

std::ostream& operator<<(std::ostream& os, const Player& e) { os << e.getStaticType(); return os; }