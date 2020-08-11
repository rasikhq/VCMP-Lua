#include "Vehicle.h"

extern PluginFuncs* g_Funcs;

std::vector<Vehicle> Vehicle::s_Vehicles;

Vehicle* Vehicle::Register(int32_t model, int32_t world, float x, float y, float z, float angle, int32_t primaryColor, int32_t secondaryColor) {
	s_Vehicles.emplace_back(model, world, x, y, z, angle, primaryColor, secondaryColor);
	return &s_Vehicles.back();
}

Vehicle* Vehicle::Get(int32_t id) {
	for (auto& Vehicle : s_Vehicles) {
		if (Vehicle.m_ID == id)
			return &Vehicle;
	}
	return nullptr;
}

void Vehicle::Unregister(Vehicle* Vehicle) {
	for (auto it = s_Vehicles.begin(); it != s_Vehicles.end(); it++) {
		if (it->m_ID == Vehicle->getID()) {
			s_Vehicles.erase(it);
			break;
		}
	}
}

Vehicle::Vehicle(int32_t model, int32_t world, float x, float y, float z, float angle, int32_t primaryColor, int32_t secondaryColor) 
{
	m_ID = g_Funcs->CreateVehicle(model, world, x, y, z, angle, primaryColor, secondaryColor);
}

Vehicle::Vehicle(int32_t model, int32_t world, sol::table transform, int32_t primaryColor, int32_t secondaryColor)
{
	if (transform[4] == sol::lua_nil)
		transform[4] = 0.0f;
	m_ID = g_Funcs->CreateVehicle(model, world, transform[1], transform[2], transform[3], transform[4], primaryColor, secondaryColor);
}

/*** METHODS ***/
bool Vehicle::destroy() {
	return (g_Funcs->DeleteVehicle(m_ID) == vcmpError::vcmpErrorNone);
}

bool Vehicle::getOption(vcmpVehicleOption option) const {
	return g_Funcs->GetVehicleOption(m_ID, option);
}

void Vehicle::setOption(vcmpVehicleOption option, bool status) {
	g_Funcs->SetVehicleOption(m_ID, option, static_cast<uint8_t>(status));
}

/*** PROPERTIES ***/
int32_t Vehicle::getID() const {
	return m_ID;
}

int32_t Vehicle::getModel() const {
	return g_Funcs->GetVehicleModel(m_ID);
}

int32_t Vehicle::getWorld() const {
	return g_Funcs->GetVehicleWorld(m_ID);
}

void Vehicle::setWorld(int32_t world) {
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
	g_Funcs->SetVehicleSpawnRotationEuler(m_ID, rotation[1], rotation[2], rotation[3]);
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

	if (colors[1].get_type() == sol::type::number)
		primary = colors[1];

	if (colors[2].get_type() == sol::type::number)
		secondary = colors[2];

	g_Funcs->SetVehicleColour(m_ID, primary, secondary);
}

/*** COMMON PROPERTIES ***/
sol::as_table_t<std::vector<float>> Vehicle::getPosition() const
{
	float x, y, z;
	g_Funcs->GetVehiclePosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}

void Vehicle::setPosition(sol::table position)
{
	g_Funcs->SetPlayerPosition(m_ID, position[1], position[2], position[3]);
}

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

	/*** METHODS ***/
	userdata["destroy"] = &Vehicle::destroy;
	userdata["getOption"] = &Vehicle::getOption;
	userdata["setOption"] = &Vehicle::setOption;

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

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["position"] = sol::property(&Vehicle::getPosition, &Vehicle::setPosition);
	userdata["angle"] = sol::property(&Vehicle::getRotation, &Vehicle::setRotation);
}
