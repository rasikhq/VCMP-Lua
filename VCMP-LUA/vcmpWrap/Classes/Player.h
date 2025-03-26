#pragma once
#include "pch.h"

class Vehicle;
class Object;

class Player {
public:
	static std::vector<Player> s_Players;

	static void Init(sol::state*);
	static Player* Register(int32_t id);
	static Player* Get(int32_t id);
	static void Unregister(Player* player);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Player"; }
	static sol::table getActive(bool spawned = false);
	static void msgAll(const std::string& msg, sol::variadic_args args);
	static void announceAll(const std::string& msg, sol::variadic_args args);
	
	/*** @Lua meta-functions & extra ***/
	bool operator==(const Player& other) { return this->m_ID == other.m_ID; }
	int32_t operator()() { return this->m_ID; }

	/*** CONSTRUCTORS ***/
	Player(int32_t id);

	/*** METHODS ***/
	void msg(const std::string&, sol::variadic_args);
	void announce(const std::string&, sol::variadic_args);
	int32_t getAlpha() const;
	void setAlpha(int32_t, uint32_t);
	bool getOption(vcmpPlayerOption) const;
	void setOption(vcmpPlayerOption, bool);
	bool isPlayerStreamed(Player*) const;
	void forceSpawn() const;
	void playSound(int32_t) const;
	void playSound3D(int32_t) const; // Player's position
	void playSound3DEx(int32_t, sol::table) const;
	void playSound3DEx2(int32_t, float, float, float) const;

	std::string getIP();
	std::string getUID();
	std::string getUID2();
	int32_t getID() const;
	int32_t getKey() const;
	int32_t getState() const;
	int32_t getUniqueWorld() const;
	int32_t getClass() const;
	int32_t getPing() const;
	int32_t getAction() const;
	int32_t getAmmo(int32_t) const;
	int32_t getVehicleSlot() const;
	int32_t getVehicleStatus() const;
	int32_t getWeapon() const;
	int32_t get3DArrowToPlayer(Player*) const;
	sol::as_table_t<std::vector<float>> getAimPos() const;
	sol::as_table_t<std::vector<float>> getAimDir() const;

	bool isOnline() const;
	bool isSpawned() const;
	bool isTyping() const;
	bool isCrouching() const;
	bool isCameraLocked() const;
	bool isAway() const;

	Object* isStandingOnObject() const;
	Vehicle* isStandingOnVehicle() const;

	float getFPS() const;
	void getModules() const;

	void setWeapon(int32_t, int32_t) const;
	void giveWeapon(int32_t, int32_t) const;
	void removeWeapon(int32_t) const;
	void disarm() const;

	void setAnimationCompact(int32_t) const;
	void setAnimation(int32_t, int32_t) const;

	void redirect(const std::string&, uint32_t, const std::string&) const;
	void restoreCamera() const;
	void selectClass() const;
	void eject() const;
	void setCam(sol::table, sol::table) const;
	void setCamEx(float, float, float, float, float, float) const;
	void interpolateCamLookAt(float, float, float, uint32_t) const;

	void kill() const;

	void kick() const;
	void ban() const;

	/*** PROPERTIES ***/
	bool getAdmin() const;
	int32_t getWorld() const;
	int32_t getSecWorld() const;
	int32_t getTeam() const;
	int32_t getSkin() const;
	int32_t getColor() const;
	int32_t getCash() const;
	int32_t getScore() const;
	int32_t getWantedLevel() const;
	int32_t getWeaponSlot() const;
	int32_t getDrunkHandling() const;
	int32_t getDrunkVisuals() const;
	uint32_t getImmunity() const;
	float getHP() const;
	float getArmour() const;
	std::string getName() const;
	Player* getSpectateTarget() const;
	Vehicle* getVehicle() const;

	/***  ***/

	void setAdmin(bool);
	void setWorld(int32_t);
	void setSecWorld(int32_t);
	void setTeam(int32_t);
	void setSkin(int32_t);
	void setColor(int32_t);
	void setCash(int32_t);
	void setScore(int32_t);
	void setWantedLevel(int32_t);
	void setWeaponSlot(int32_t);
	void setSpectateTarget(Player*);
	void setImmunity(uint32_t);
	void setHP(float);
	void setArmour(float);
	void setName(const std::string&);
	void setVehicle(Vehicle*);
	void setVehicleWithSlot(Vehicle*, int32_t);
	void set3DArrowToPlayer(Player*, bool) const;
	void setDrunkHandling(int32_t) const;
	void setDrunkVisuals(int32_t) const;

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Player::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);
	
	float getRotation() const;
	void setRotation(float);
	/******/

private:
	int32_t m_ID;
	char m_Name[24];
	sol::table m_LuaData;
};