#pragma once
#include "pch.h"

class Server {
public:
	static void Init(sol::state*);
	static int GetSkinID(const char*);

	static bool getOption(vcmpServerOption option);
	static bool setOption(vcmpServerOption option, bool toggle);
	static sol::object getSettings();

	static std::string getName();
	static void setName(const std::string&);

	static std::string getPassword();
	static void setPassword(const std::string&);

	static uint16_t getFallTimer();
	static int32_t getTimeRate();
	static int32_t getHour();
	static int32_t getMinute();
	static int32_t getWeather();
	static float getGravity();
	static float getWaterLevel();
	static float getGameSpeed();
	static float getFlightAltitude();
	static float getVehicleRespawnHeight();

	static void setFallTimer(uint16_t);
	static void setTimeRate(int32_t);
	static void setHour(int32_t);
	static void setMinute(int32_t);
	static void setWeather(int32_t);
	static void setGravity(float);
	static void setWaterLevel(float);
	static void setGameSpeed(float);
	static void setFlightAltitude(float);
	static void setVehicleRespawnHeight(float);

	/*** PLAYER ***/
	static bool banIP(char*);
	static bool unbanIP(char*);
	static bool isIPBanned(char*);
};

/*****************************************************************************************************/

class Map {
public:
	static void Init(sol::state*);

	static std::string GetDistrictName(float, float);
};

/*****************************************************************************************************/

class Radio {
public:
	static void Init(sol::state*);
};

/*****************************************************************************************************/

class Weapon {
public:
	static void Init(sol::state*);

	static int GetWeaponID(const char*);

private:
	static void setData(int32_t, int32_t, double);
	static double getData(int32_t, int32_t);
};

/*****************************************************************************************************/

class Blip {
public:
	static void Init(sol::state*);
};

/*****************************************************************************************************/

class Sound {
public:
	static void Init(sol::state*);
};