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

bool Vehicle::destroy() {
	return (g_Funcs->DeleteVehicle(m_ID) == vcmpError::vcmpErrorNone);
}

int32_t Vehicle::getID() {
	return m_ID;
}

int32_t Vehicle::getModel() {
	return g_Funcs->GetVehicleModel(m_ID);
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

	/*** READ-ONLY ***/
	userdata.set("getID", &Vehicle::getID);
	userdata.set("getModel", &Vehicle::getModel);

	/*** METHODS ***/
	userdata["destroy"] = &Vehicle::destroy;

	/*** PROPERTIES ***/

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["position"] = sol::property(&Vehicle::getPosition, &Vehicle::setPosition);
	userdata["angle"] = sol::property(&Vehicle::getRotation, &Vehicle::setRotation);
}
