#pragma once
#include "pch.h"

class Vehicle {
public:
	static std::vector<Vehicle> s_Vehicles;

	static void Init(sol::state*);
	static Vehicle* Register(int32_t, int32_t, float, float, float, float, int32_t = -1, int32_t = -1);
	static Vehicle* Get(int32_t id);
	static void Unregister(Vehicle* player);

	Vehicle(int32_t, int32_t, float, float, float, float, int32_t = -1, int32_t = -1);
	Vehicle(int32_t, int32_t, sol::table, int32_t = -1, int32_t = -1);

	bool destroy();

	int32_t getID();
	int32_t getModel();

private:
	int32_t m_ID;
};