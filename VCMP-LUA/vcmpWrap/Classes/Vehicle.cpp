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
}
