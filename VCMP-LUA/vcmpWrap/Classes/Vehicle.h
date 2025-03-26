#pragma once
#include "pch.h"
#include "../Constants.h"

#define MAX_VEHICLES 1000

class Player;

class Vehicle {
public:
	static std::vector<Vehicle*> s_Vehicles;

	static void Init(sol::state*);
	static void Register(Vehicle*);
	static Vehicle* Get(int32_t);
	static void Unregister(Vehicle*);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Vehicle"; }
	static sol::table getActive();

	static void resetAllHandlings();
	static uint8_t modelHandlingRuleExists(int32_t, int32_t);
	static void resetModelHandlingRule(int32_t, int32_t);
	static void resetModelHandlingRules(int32_t);
	static double getModelHandlingRule(int32_t, int32_t);
	static void setModelHandlingRule(int32_t, int32_t, double);

	/*** @Lua meta-functions & extra ***/
	bool operator==(const Vehicle& other) { return this->m_ID == other.m_ID; }
	int32_t operator()() { return this->m_ID; }

	/*** CONSTRUCTORS ***/
	Vehicle(int32_t, int32_t, float, float, float, float, int32_t = -1, int32_t = -1);
	Vehicle(int32_t, int32_t, sol::table, int32_t = -1, int32_t = -1);

	/*** DESTRUCTOR(S) ***/
	~Vehicle() { destroy(); };

	/*** METHODS ***/
	bool destroy();
	void respawn();
	void repair();
	bool isStreamedForPlayer(Player*);
	bool getOption(vcmpVehicleOption) const;
	void setOption(vcmpVehicleOption, bool);
	int32_t getPartStatus(int32_t) const;
	void setPartStatus(int32_t, int32_t);
	int32_t getTyreStatus(int32_t) const;
	void setTyreStatus(int32_t, int32_t);
	sol::as_table_t<std::vector<float>> getSpeed(vcmpVehicleSpeed type = vcmpVehicleSpeed::Normal) const;
	sol::as_table_t<std::vector<float>> getSpeedEx() const;
	void setSpeed(vcmpVehicleSpeed, float, float, float, bool);
	void setSpeedDefault(float, float, float, bool);
	void setSpeedEx(vcmpVehicleSpeed, sol::table, bool);
	void setSpeedExDefault(sol::table, bool);
	void resetHandling();
	void resetHandlingRule(int32_t);
	void resetHandlingRuleEx(sol::table);
	uint8_t hasHandlingRule(int32_t);
	void setHandlingRule(int32_t, double);
	double getHandlingRule(int32_t);
	void set3DArrowToPlayer(Player* player, bool) const;
	int32_t get3DArrowToPlayer(Player* player) const;

	/*** READ-ONLY ***/
	int32_t getID() const;
	int32_t getModel() const;
	Player* getOccupant(int32_t) const;
	std::tuple<float, float> getTurretRotation() const;
	
	/*** PROPERTIES ***/
	int32_t getWorld() const;
	int32_t getRadio() const;
	uint32_t getIdleRespawnTime() const;
	uint32_t getLightsData() const;
	uint32_t getDamage() const;
	uint32_t getImmunity() const;
	float getHealth() const;
	bool getTaxiLight() const;
	sol::as_table_t<std::vector<float>> getSpawnPosition() const;
	sol::as_table_t<std::vector<float>> getSpawnRotation() const;
	sol::as_table_t<std::vector<int32_t>> getColor() const;
	
	void setWorld(int32_t);
	void setRadio(int32_t);
	void setIdleRespawnTime(uint32_t);
	void setLightsData(uint32_t);
	void setDamage(uint32_t);
	void setImmunity(uint32_t);
	void setHealth(float);
	void setTaxiLight(bool);
	void setSpawnPosition(sol::table);
	void setSpawnRotation(sol::table);
	void setColor(sol::table);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Vehicle::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);

	sol::table getRotation() const;
	void setRotation(sol::table);
	void setRotationEuler(float, float, float);
	void setRotationQuaternion(float, float, float, float);
	/******/
private:
	int32_t m_ID;
	sol::table m_LuaData;
};