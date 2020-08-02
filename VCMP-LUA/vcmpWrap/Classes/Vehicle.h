#pragma once
#include "pch.h"

class Vehicle {
public:
	static std::vector<Vehicle> s_Vehicles;

	static void Init(sol::state*);
	static Vehicle* Register(int32_t, int32_t, float, float, float, float, int32_t = -1, int32_t = -1);
	static Vehicle* Get(int32_t id);
	static void Unregister(Vehicle* player);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Vehicle"; }

	/*** CONSTRUCTORS ***/
	Vehicle(int32_t, int32_t, float, float, float, float, int32_t = -1, int32_t = -1);
	Vehicle(int32_t, int32_t, sol::table, int32_t = -1, int32_t = -1);

	/*** READ-ONLY ***/
	int32_t getID();
	int32_t getModel();

	/*** METHODS ***/
	bool destroy();

	/*** PROPERTIES ***/

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Vehicle::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);

	sol::as_table_t<std::vector<float>> getRotation() const;
	void setRotation(sol::table);
	/******/
private:
	int32_t m_ID;
};