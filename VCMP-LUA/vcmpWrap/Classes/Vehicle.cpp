#include "Vehicle.h"

extern PluginFuncs* g_Funcs;

std::vector<Vehicle> Vehicle::s_Vehicles;

Vehicle* Vehicle::Register(int32_t id) {
	s_Vehicles.emplace_back(id);
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

Vehicle::Vehicle(int32_t id) 
	: m_ID(id) {}

int32_t Vehicle::getID() {
	return m_ID;
}

void Vehicle::init(lua_State* L) {
	// TO DO: Improved constructor...
	luabridge::getGlobalNamespace(L)
		.beginClass<Vehicle>("Vehicle")

		.addFunction("getID", &Vehicle::getID)

		.endClass();
}
