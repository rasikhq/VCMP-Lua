#pragma once
#include "pch.h"
#include "Vehicle.h"

class Player {
public:
	static std::vector<Player> s_Players;

	static void Init(sol::state*);
	static Player* Register(int32_t id);
	static Player* Get(int32_t id);
	static void Unregister(Player* player);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Player"; }

	/*** CONSTRUCTORS ***/
	Player(int32_t id);

	/*** METHODS ***/
	void msg(const std::string&);
	bool getOption(vcmpPlayerOption) const;
	void setOption(vcmpPlayerOption, bool);
	bool isPlayerStreamed(Player*) const;

	/*** READ-ONLY ***/
	std::string getIP();
	std::string getUID();
	std::string getUID2();
	int32_t getID() const;
	int32_t getKey() const;
	int32_t getState() const;
	int32_t getUniqueWorld() const;
	int32_t getClass() const;
	int32_t getPing() const;
	bool isOnline() const;
	bool isSpawned() const;
	bool isTyping() const;
	bool isCrouching() const;
	float getFPS() const;
	void getModules() const;

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
};