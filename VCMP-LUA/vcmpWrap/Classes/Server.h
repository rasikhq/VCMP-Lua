#pragma once
#include "pch.h"

class Server {
public:
	Server() = default;
	~Server() = default;

	static void Init(sol::state*);

	/*** @Lua static ***/
	static constexpr const char* s_TYPE = "Server";
	static inline const char* getStaticType() { return s_TYPE; }

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/

	/*** METHODS ***/

	static int GetSkinID(const char*);

	static bool getOption(vcmpServerOption option);
	static bool setOption(vcmpServerOption option, bool toggle);
	static sol::object getSettings();

	static std::string getName();
	static void setName(const std::string&);

	static std::string getGamemode();
	static void setGamemode(const std::string&);

	static std::string getPassword();
	static void setPassword(const std::string&);

	static void addClass(int32_t teamId, uint32_t colour, int32_t modelIndex, float x, float y, float z, float angle, int32_t weaponOne, int32_t weaponOneAmmo, int32_t weaponTwo, int32_t weaponTwoAmmo, int32_t weaponThree, int32_t weaponThreeAmmo);
	static void addClassEx(int32_t teamId, uint32_t colour, int32_t modelIndex, sol::table spawnPosition, float angle, sol::variadic_args vargs);
	static void addClassEx2(int32_t teamId, uint32_t colour, int32_t modelIndex, sol::table spawnPositionAngle, sol::variadic_args vargs);

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
	static sol::table getWastedSettings();
	static int32_t getKillDelay();

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
	static void setWastedSettings(sol::table);
	static void setKillDelay(int32_t);

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