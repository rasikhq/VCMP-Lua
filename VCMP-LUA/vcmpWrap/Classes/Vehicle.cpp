#include "Vehicle.h"
#include "Player.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

std::vector<Vehicle*> Vehicle::s_Vehicles;

void Vehicle::Register(Vehicle* inst) {
	inst->m_LuaData = Lua.create_table();
	s_Vehicles.push_back(inst);
}

Vehicle* Vehicle::Get(int32_t id) {
	for (auto& Vehicle : s_Vehicles) {
		if (Vehicle->m_ID == id)
			return Vehicle;
	}
	return nullptr;
}

void Vehicle::Unregister(Vehicle* Vehicle) {
	for (auto it = s_Vehicles.begin(); it != s_Vehicles.end(); it++) {
		if ((*it)->m_ID == Vehicle->getID()) {
			s_Vehicles.erase(it);
			break;
		}
	}
}

sol::table Vehicle::getActive()
{
	sol::table entities = Lua.create_table_with();
	for (auto entity : s_Vehicles)
		entities[entity->getID()] = entity;
	return entities;
}

void Vehicle::resetAllHandlings() 
{
	g_Funcs->ResetAllVehicleHandlings();
}

uint8_t Vehicle::modelHandlingRuleExists(int32_t modelIndex, int32_t ruleIndex) 
{
	return g_Funcs->ExistsHandlingRule(modelIndex, ruleIndex);
}

void Vehicle::resetModelHandlingRule(int32_t modelIndex, int32_t ruleIndex) 
{
	g_Funcs->ResetHandlingRule(modelIndex, ruleIndex);
}

void Vehicle::resetModelHandlingRules(int32_t modelIndex) 
{
	g_Funcs->ResetHandling(modelIndex);
}

double Vehicle::getModelHandlingRule(int32_t modelIndex, int32_t ruleIndex) 
{
	return g_Funcs->GetHandlingRule(modelIndex, ruleIndex);
}

void Vehicle::setModelHandlingRule(int32_t modelIndex, int32_t ruleIndex, double value) 
{
	g_Funcs->SetHandlingRule(modelIndex, ruleIndex, value);
}

Vehicle::Vehicle(int32_t model, int32_t world, float x, float y, float z, float angle, int32_t primaryColor, int32_t secondaryColor) 
{
	if (s_Vehicles.size() >= s_Vehicles.capacity()) {
		spdlog::error("Max Vehicle entity limit of {} reached!", MAX_VEHICLES);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreateVehicle(model, world, x, y, z, angle, primaryColor, secondaryColor);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

Vehicle::Vehicle(int32_t model, int32_t world, sol::table transform, int32_t primaryColor, int32_t secondaryColor)
{
	if (s_Vehicles.size() >= s_Vehicles.capacity()) {
		spdlog::error("Max Vehicle entity limit of {} reached!", MAX_VEHICLES);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreateVehicle(model, world, transform[1], transform[2], transform[3], transform.get_or(4, 0.0f), primaryColor, secondaryColor);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

/*** METHODS ***/
bool Vehicle::destroy() {
	bool bDestroyed = g_Funcs->DeleteVehicle(m_ID) == vcmpError::vcmpErrorNone;
	if (bDestroyed)
		Unregister(this);
	return bDestroyed;
}

void Vehicle::respawn() {
	g_Funcs->RespawnVehicle(m_ID);
}

void Vehicle::repair() {
	g_Funcs->SetVehicleHealth(m_ID, 1000);
	g_Funcs->SetVehicleDamageData(m_ID, 0);

	uint32_t dwLightsData = g_Funcs->GetVehicleLightsData(m_ID);
	dwLightsData &= 0xFFFFFF00;
	g_Funcs->SetVehicleLightsData(m_ID, dwLightsData);
}

bool Vehicle::isStreamedForPlayer(Player* player) { return g_Funcs->IsVehicleStreamedForPlayer(m_ID, player->getID()); }

bool Vehicle::getOption(vcmpVehicleOption option) const { return g_Funcs->GetVehicleOption(m_ID, option); }
void Vehicle::setOption(vcmpVehicleOption option, bool status) { g_Funcs->SetVehicleOption(m_ID, option, static_cast<uint8_t>(status)); }

int32_t Vehicle::getPartStatus(int32_t part) const {
	return g_Funcs->GetVehiclePartStatus(m_ID, part);
}

void Vehicle::setPartStatus(int32_t part, int32_t status) {
	g_Funcs->SetVehiclePartStatus(m_ID, part, status);
}

int32_t Vehicle::getTyreStatus(int32_t tyre) const {
	return g_Funcs->GetVehicleTyreStatus(m_ID, tyre);
}

void Vehicle::setTyreStatus(int32_t tyre, int32_t status) {
	g_Funcs->SetVehicleTyreStatus(m_ID, tyre, status);
}

sol::as_table_t<std::vector<float>> Vehicle::getSpeed(vcmpVehicleSpeed type) const {
	float vx, vy, vz;
	switch (type) {
	case vcmpVehicleSpeed::Normal:
		g_Funcs->GetVehicleSpeed(m_ID, &vx, &vy, &vz, false);
		break;

	case vcmpVehicleSpeed::NormalRelative:
		g_Funcs->GetVehicleSpeed(m_ID, &vx, &vy, &vz, true);
		break;

	case vcmpVehicleSpeed::Turn:
		g_Funcs->GetVehicleTurnSpeed(m_ID, &vx, &vy, &vz, false);
		break;

	case vcmpVehicleSpeed::TurnRelative:
		g_Funcs->GetVehicleTurnSpeed(m_ID, &vx, &vy, &vz, true);
		break;
	}
	std::vector<float> speed = { vx, vy, vz };
	return sol::as_table(speed);
}

sol::as_table_t<std::vector<float>> Vehicle::getSpeedEx() const {
	return getSpeed(vcmpVehicleSpeed::Normal);
}

void Vehicle::setSpeed(vcmpVehicleSpeed type, float vx, float vy, float vz, bool bAddSpeed) {
	switch (type) {
	case vcmpVehicleSpeed::Normal:
		g_Funcs->SetVehicleSpeed(m_ID, vx, vy, vz, bAddSpeed, false);
		break;

	case vcmpVehicleSpeed::NormalRelative:
		g_Funcs->SetVehicleSpeed(m_ID, vx, vy, vz, bAddSpeed, true);
		break;

	case vcmpVehicleSpeed::Turn:
		g_Funcs->SetVehicleTurnSpeed(m_ID, vx, vy, vz, bAddSpeed, false);
		break;

	case vcmpVehicleSpeed::TurnRelative:
		g_Funcs->SetVehicleTurnSpeed(m_ID, vx, vy, vz, bAddSpeed, true);
		break;
	}
}

void Vehicle::setSpeedDefault(float vx, float vy, float vz, bool bAddSpeed) {
	setSpeed(vcmpVehicleSpeed::Normal, vx, vy, vz, bAddSpeed);
}

void Vehicle::setSpeedEx(vcmpVehicleSpeed type, sol::table values, bool bAddSpeed) {
	if (values.size() < 3) {
		spdlog::error("Table passed to Vehicle::addSpeed does not have enough values");
		return;
	}
	float vx = values[1];
	float vy = values[2];
	float vz = values[3];
	setSpeed(type, vx, vy, vz, bAddSpeed);
}

void Vehicle::setSpeedExDefault(sol::table values, bool bAddSpeed) {
	setSpeedEx(vcmpVehicleSpeed::Normal, values, bAddSpeed);
}

void Vehicle::resetHandling() 
{
	g_Funcs->ResetInstHandling(m_ID);
}

void Vehicle::resetHandlingRule(int32_t ruleIndex) 
{
	g_Funcs->ResetInstHandlingRule(m_ID, ruleIndex);
}

void Vehicle::resetHandlingRuleEx(sol::table ruleIndexes) 
{
	ruleIndexes.for_each([this] (const sol::object& key, const sol::object& value) {
		if(value.is<int32_t>()) {
			int32_t ruleIndex = value.as<int32_t>();
			this->resetHandlingRule(ruleIndex);
		}
	});
}

uint8_t Vehicle::hasHandlingRule(int32_t ruleIndex) 
{
	return g_Funcs->ExistsInstHandlingRule(m_ID, ruleIndex);
}

void Vehicle::setHandlingRule(int32_t ruleIndex, double value) 
{
	g_Funcs->SetInstHandlingRule(m_ID, ruleIndex, value);
}

double Vehicle::getHandlingRule(int32_t ruleIndex) 
{
	return g_Funcs->GetInstHandlingRule(m_ID, ruleIndex);
}

/*** PROPERTIES ***/
int32_t Vehicle::getID() const { return m_ID; }
int32_t Vehicle::getModel() const { return g_Funcs->GetVehicleModel(m_ID); }

Player* Vehicle::getOccupant(int32_t slot) const {
	int32_t id = g_Funcs->GetVehicleOccupant(m_ID, slot);
	if(g_Funcs->GetLastError() != vcmpError::vcmpErrorNone)
		return nullptr;

	return Player::Get(id);
}

int32_t Vehicle::getWorld() const {
	return g_Funcs->GetVehicleWorld(m_ID); 
}

void Vehicle::setWorld(int32_t world) {
	if (getWorld() == world)
		return;
	g_Funcs->SetVehicleWorld(m_ID, world); 
}

uint32_t Vehicle::getIdleRespawnTime() const {
	return g_Funcs->GetVehicleIdleRespawnTimer(m_ID); 
}
void Vehicle::setIdleRespawnTime(uint32_t time) {
	g_Funcs->SetVehicleIdleRespawnTimer(m_ID, time); 
}

float Vehicle::getHealth() const {
	return g_Funcs->GetVehicleHealth(m_ID); 
}

bool Vehicle::getTaxiLight() const {
	return (g_Funcs->GetVehicleLightsData(m_ID) & (1 << 8)) != 0;
}

void Vehicle::setHealth(float health) {
	if (getHealth() == health)
		return;
	g_Funcs->SetVehicleHealth(m_ID, health); 
}

void Vehicle::setTaxiLight(bool state) {
	if((getTaxiLight() != 0) == state)
		return;

	uint32_t dwLightsData = g_Funcs->GetVehicleLightsData(m_ID);

	if(state)
		dwLightsData |= (1 << 8);
	else
		dwLightsData &= ~(1 << 8);

	g_Funcs->SetVehicleLightsData(m_ID, dwLightsData);
}

sol::as_table_t<std::vector<float>> Vehicle::getSpawnPosition() const {
	float x, y, z;
	g_Funcs->GetVehicleSpawnPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}

void Vehicle::setSpawnPosition(sol::table position) {
	if (position.size() < 3) return;
	g_Funcs->SetVehicleSpawnPosition(m_ID, position[1], position[2], position[3]);
}

sol::as_table_t<std::vector<float>> Vehicle::getSpawnRotation() const {
	float x, y, z;
	g_Funcs->GetVehicleSpawnRotationEuler(m_ID, &x, &y, &z);
	std::vector<float> rotation = { x, y, z };
	return sol::as_table(rotation);
}

void Vehicle::setSpawnRotation(sol::table rotation) {
	if (rotation.size() < 3) return;
	if (rotation.size() == 3)
		g_Funcs->SetVehicleSpawnRotationEuler(m_ID, rotation.get_or(1, 0.0f), rotation.get_or(2, 0.0f), rotation.get_or(3, 0.0f));
	else if (rotation.size() == 4)
		g_Funcs->SetVehicleSpawnRotation(m_ID, rotation.get_or(1, 0.0f), rotation.get_or(2, 0.0f), rotation.get_or(3, 0.0f), rotation.get_or(4, 0.0f));
}

sol::as_table_t<std::vector<int32_t>> Vehicle::getColor() const {
	int32_t primary, secondary;
	g_Funcs->GetVehicleColour(m_ID, &primary, &secondary);
	std::vector<int32_t> color = { primary, secondary };
	return sol::as_table(color);
}

std::tuple<float, float> Vehicle::getTurretRotation() const {
	float horizontal, vertical;
	g_Funcs->GetVehicleTurretRotation(m_ID, &horizontal, &vertical);

	return std::make_tuple(horizontal, vertical);
}

void Vehicle::setColor(sol::table colors) {
	int32_t primary, secondary;
	g_Funcs->GetVehicleColour(m_ID, &primary, &secondary);
	g_Funcs->SetVehicleColour(m_ID, colors.get_or(1, primary), colors.get_or(2, secondary));
}

void Vehicle::set3DArrowToPlayer(Player* player, bool state) const
{
	g_Funcs->SetVehicle3DArrowForPlayer(m_ID, player->getID(), state);
}

int32_t Vehicle::get3DArrowToPlayer(Player* player) const
{
	return g_Funcs->GetVehicle3DArrowForPlayer(m_ID, player->getID());
}

uint32_t Vehicle::getLightsData() const { return g_Funcs->GetVehicleLightsData(m_ID); }
void Vehicle::setLightsData(uint32_t data) {
	if (getLightsData() == data)
		return;
	g_Funcs->SetVehicleLightsData(m_ID, data);
}

int32_t Vehicle::getRadio() const { return g_Funcs->GetVehicleRadio(m_ID); }
void Vehicle::setRadio(int32_t radioID) {
	if (getRadio() == radioID)
		return;
	g_Funcs->SetVehicleRadio(m_ID, radioID);
}

uint32_t Vehicle::getDamage() const { 
	return g_Funcs->GetVehicleDamageData(m_ID);
}

uint32_t Vehicle::getImmunity() const {
	return g_Funcs->GetVehicleImmunityFlags(m_ID);
}

void Vehicle::setDamage(uint32_t data) { 
	if (getDamage() == data)
		return;
	g_Funcs->SetVehicleDamageData(m_ID, data); 
}

void Vehicle::setImmunity(uint32_t flags) {
	g_Funcs->SetVehicleImmunityFlags(m_ID, flags);
}

/*** COMMON PROPERTIES ***/
sol::as_table_t<std::vector<float>> Vehicle::getPosition() const
{
	float x, y, z;
	g_Funcs->GetVehiclePosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}
void Vehicle::setPosition(sol::table position) { g_Funcs->SetVehiclePosition(m_ID, position[1], position[2], position[3], 0); }

sol::table Vehicle::getRotation() const
{
	sol::table rotationTable = Lua.create_table();
	sol::table rotationTableEuler = Lua.create_table();
	sol::table rotationTableQuaternion = Lua.create_table();

	float x, y, z, w;
	g_Funcs->GetVehicleRotationEuler(m_ID, &x, &y, &z);
	rotationTableEuler[1] = x;
	rotationTableEuler[2] = y;
	rotationTableEuler[3] = z;

	g_Funcs->GetVehicleRotation(m_ID, &x, &y, &z, &w);
	rotationTableQuaternion[1] = x;
	rotationTableQuaternion[2] = y;
	rotationTableQuaternion[3] = z;
	rotationTableQuaternion[4] = w;

	rotationTable["euler"] = rotationTableEuler;
	rotationTable["quaternion"] = rotationTableQuaternion;

	return rotationTable;
}

void Vehicle::setRotation(sol::table rotation)
{
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	
	if (rotation.size() > 3)
	{
		float w = rotation.get_or(4, 0.0f);
		g_Funcs->SetVehicleRotation(m_ID, x, y, z, w);
	}
	else
		g_Funcs->SetVehicleRotationEuler(m_ID, x, y, z);
}

void Vehicle::setRotationEuler(float x, float y, float z)
{
	g_Funcs->SetVehicleRotationEuler(m_ID, x, y, z);
}

void Vehicle::setRotationQuaternion(float x, float y, float z, float w)
{
	g_Funcs->SetVehicleRotation(m_ID, x, y, z, w);
}

/******/

void Vehicle::Init(sol::state* L) {
	s_Vehicles.reserve(MAX_VEHICLES);

	sol::usertype<Vehicle> userdata = L->new_usertype<Vehicle>("Vehicle",
		sol::constructors<
			Vehicle(int32_t, int32_t, float, float, float, float),
			Vehicle(int32_t, int32_t, float, float, float, float, int32_t),
			Vehicle(int32_t, int32_t, float, float, float, float, int32_t, int32_t),

			Vehicle(int32_t, int32_t, sol::table),
			Vehicle(int32_t, int32_t, sol::table, int32_t),
			Vehicle(int32_t, int32_t, sol::table, int32_t, int32_t)
		>(),
		sol::meta_function::type, &Vehicle::getStaticType,
		sol::meta_function::type_info, &Vehicle::getStaticType
	);

	userdata["type"] = &Vehicle::getStaticType;
	userdata["findByID"] = &Vehicle::Get;
	userdata["count"] = []() { return s_Vehicles.size(); };
	userdata["getActive"] = &Vehicle::getActive;
	userdata["resetAllHandlings"] = &Vehicle::resetAllHandlings;
	userdata["modelHandlingRuleExists"] = &Vehicle::modelHandlingRuleExists;
	userdata["resetModelHandlingRule"] = &Vehicle::resetModelHandlingRule;
	userdata["resetModelHandlingRules"] = &Vehicle::resetModelHandlingRules;
	userdata["getModelHandlingRule"] = &Vehicle::getModelHandlingRule;
	userdata["setModelHandlingRule"] = &Vehicle::setModelHandlingRule;

	/*** METHODS ***/
	userdata["destroy"] = &Vehicle::destroy;
	userdata["respawn"] = &Vehicle::respawn;
	userdata["repair"] = &Vehicle::repair;
	userdata["fix"] = &Vehicle::repair; // Alias for repair
	userdata["streamedForPlayer"] = &Vehicle::isStreamedForPlayer;
	userdata["getOption"] = &Vehicle::getOption;
	userdata["setOption"] = &Vehicle::setOption;
	userdata["getPartStatus"] = &Vehicle::getPartStatus;
	userdata["setPartStatus"] = &Vehicle::setPartStatus;
	userdata["getTyreStatus"] = &Vehicle::getTyreStatus;
	userdata["setTyreStatus"] = &Vehicle::setTyreStatus;
	userdata["getSpeed"] = sol::overload(&Vehicle::getSpeed, &Vehicle::getSpeedEx);
	userdata["setSpeed"] = sol::overload(&Vehicle::setSpeed, &Vehicle::setSpeedDefault, &Vehicle::setSpeedEx, &Vehicle::setSpeedExDefault);
	userdata["getRotation"] = &Vehicle::getRotation;
	userdata["setRotation"] = sol::overload(&Vehicle::setRotation, &Vehicle::setRotationEuler, &Vehicle::setRotationQuaternion);
	userdata["resetHandling"] = &Vehicle::resetHandling;
	userdata["resetHandlingRule"] = sol::overload(&Vehicle::resetHandlingRule, &Vehicle::resetHandlingRuleEx);
	userdata["hasHandlingRule"] = &Vehicle::hasHandlingRule;
	userdata["getHandlingRule"] = &Vehicle::getHandlingRule;
	userdata["setHandlingRule"] = &Vehicle::setHandlingRule;
	userdata["get3DArrowToPlayer"] = &Vehicle::get3DArrowToPlayer;
	userdata["set3DArrowToPlayer"] = &Vehicle::set3DArrowToPlayer;

	/*** READ-ONLY ***/
	userdata.set("getType", &Vehicle::getType);
	userdata.set("getID", &Vehicle::getID);
	userdata.set("getModel", &Vehicle::getModel);
	userdata.set("getOccupant", &Vehicle::getOccupant);
	userdata.set("getTurretRotation", &Vehicle::getTurretRotation);

	/*** PROPERTIES ***/
	userdata["world"] = sol::property(&Vehicle::getWorld, &Vehicle::setWorld);
	userdata["health"] = sol::property(&Vehicle::getHealth, &Vehicle::setHealth);
	userdata["taxiLight"] = sol::property(&Vehicle::getTaxiLight, &Vehicle::setTaxiLight);
	userdata["idleRespawnTime"] = sol::property(&Vehicle::getIdleRespawnTime, &Vehicle::setIdleRespawnTime);
	userdata["spawnPosition"] = sol::property(&Vehicle::getSpawnPosition, &Vehicle::setSpawnPosition);
	userdata["spawnRotation"] = sol::property(&Vehicle::getSpawnRotation, &Vehicle::setSpawnRotation);
	userdata["color"] = sol::property(&Vehicle::getColor, &Vehicle::setColor);
	userdata["radio"] = sol::property(&Vehicle::getRadio, &Vehicle::setRadio);
	userdata["damage"] = sol::property(&Vehicle::getDamage, &Vehicle::setDamage);
	userdata["immunity"] = sol::property(&Vehicle::getImmunity, &Vehicle::setImmunity);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["data"] = &Vehicle::m_LuaData;
	userdata["position"] = sol::property(&Vehicle::getPosition, &Vehicle::setPosition);
	userdata["angle"] = sol::property(&Vehicle::getRotation, &Vehicle::setRotation);
	userdata["rotation"] = sol::property(&Vehicle::getRotation, &Vehicle::setRotation);
}

std::ostream& operator<<(std::ostream& os, const Vehicle& e) { os << e.getStaticType(); return os; }