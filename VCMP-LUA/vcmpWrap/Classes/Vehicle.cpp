#include "Vehicle.h"
#include "Player.h"

extern PluginFuncs* g_Funcs;

std::vector<Vehicle*> Vehicle::s_Vehicles;

void Vehicle::Register(Vehicle* inst) {
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

Vehicle::Vehicle(int32_t model, int32_t world, float x, float y, float z, float angle, int32_t primaryColor, int32_t secondaryColor) 
{
	m_ID = g_Funcs->CreateVehicle(model, world, x, y, z, angle, primaryColor, secondaryColor);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

Vehicle::Vehicle(int32_t model, int32_t world, sol::table transform, int32_t primaryColor, int32_t secondaryColor)
{
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

bool Vehicle::isStreamedForPlayer(Player* player) { return g_Funcs->IsVehicleStreamedForPlayer(m_ID, player->getID()); }

bool Vehicle::getOption(vcmpVehicleOption option) const { return g_Funcs->GetVehicleOption(m_ID, option); }
void Vehicle::setOption(vcmpVehicleOption option, bool status) { g_Funcs->SetVehicleOption(m_ID, option, static_cast<uint8_t>(status)); }

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

/*** PROPERTIES ***/
int32_t Vehicle::getID() const { return m_ID; }
int32_t Vehicle::getModel() const { return g_Funcs->GetVehicleModel(m_ID); }

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
void Vehicle::setHealth(float health) {
	if (getHealth() == health)
		return;
	g_Funcs->SetVehicleHealth(m_ID, health); 
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

void Vehicle::setColor(sol::table colors) {
	int32_t primary, secondary;
	g_Funcs->GetVehicleColour(m_ID, &primary, &secondary);
	g_Funcs->SetVehicleColour(m_ID, colors.get_or(1, primary), colors.get_or(2, secondary));
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

uint32_t Vehicle::getDamage() const { return g_Funcs->GetVehicleDamageData(m_ID); }
void Vehicle::setDamage(uint32_t data) { 
	if (getDamage() == data)
		return;
	g_Funcs->SetVehicleDamageData(m_ID, data); 
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

sol::as_table_t<std::vector<float>> Vehicle::getRotation() const
{
	float x, y, z;
	g_Funcs->GetVehicleRotationEuler(m_ID, &x, &y, &z);
	std::vector<float> rotation = { x, y, z };
	return sol::as_table(rotation);
}

void Vehicle::setRotation(sol::table rotation)
{
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	g_Funcs->SetVehicleRotationEuler(m_ID, x, y, z);
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
		>()
	);

	userdata["type"] = &Vehicle::getStaticType;
	userdata["findByID"] = &Vehicle::Get;
	userdata["count"] = []() { return s_Vehicles.size(); };

	/*** METHODS ***/
	userdata["destroy"] = &Vehicle::destroy;
	userdata["streamedForPlayer"] = &Vehicle::isStreamedForPlayer;
	userdata["getOption"] = &Vehicle::getOption;
	userdata["setOption"] = &Vehicle::setOption;
	userdata["getSpeed"] = sol::overload(&Vehicle::getSpeed, &Vehicle::getSpeedEx);
	userdata["setSpeed"] = sol::overload(&Vehicle::setSpeed, &Vehicle::setSpeedDefault, &Vehicle::setSpeedEx, &Vehicle::setSpeedExDefault);

	/*** READ-ONLY ***/
	userdata.set("getID", &Vehicle::getID);
	userdata.set("getModel", &Vehicle::getModel);

	/*** PROPERTIES ***/
	userdata["world"] = sol::property(&Vehicle::getWorld, &Vehicle::setWorld);
	userdata["health"] = sol::property(&Vehicle::getHealth, &Vehicle::setHealth);
	userdata["idleRespawnTime"] = sol::property(&Vehicle::getIdleRespawnTime, &Vehicle::setIdleRespawnTime);
	userdata["spawnPosition"] = sol::property(&Vehicle::getSpawnPosition, &Vehicle::setSpawnPosition);
	userdata["spawnRotation"] = sol::property(&Vehicle::getSpawnRotation, &Vehicle::setSpawnRotation);
	userdata["color"] = sol::property(&Vehicle::getColor, &Vehicle::setColor);
	userdata["radio"] = sol::property(&Vehicle::getRadio, &Vehicle::setRadio);
	userdata["damage"] = sol::property(&Vehicle::getDamage, &Vehicle::setDamage);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["position"] = sol::property(&Vehicle::getPosition, &Vehicle::setPosition);
	userdata["angle"] = sol::property(&Vehicle::getRotation, &Vehicle::setRotation);
}
