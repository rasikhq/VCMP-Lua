#pragma once
#include "pch.h"

class Player {
public:
	static std::vector<Player> s_Players;

	static void init(lua_State*);
	static Player* Register(int32_t id);
	static Player* Get(int32_t id);
	static void Unregister(Player* player);

	/*** @Lua static ***/
	static const char* getStaticType();
	static Player findByID(int32_t);

	Player(int32_t id);

	const char* getType();

	std::string getIP();
	std::string getUID();
	std::string getUID2();
	int32_t getID();
	int32_t getKey();
	int32_t getState();
	int32_t getUniqueWorld();
	int32_t getClass();
	int32_t getPing();
	bool isOnline();
	bool isSpawned();
	bool isTyping();
	float getFPS();

	bool getAdmin() const;
	int32_t getWorld() const;
	int32_t getSecWorld() const;
	int32_t getTeam() const;
	int32_t getSkin() const;
	int32_t getColor() const;
	int32_t getCash() const;
	int32_t getScore() const;
	int32_t getWantedLevel() const;
	float getHP() const;
	float getArmour() const;
	std::string getName() const;
	
	void setAdmin(bool);
	void setScore(int32_t);
	void setWorld(int32_t);
	void setSecWorld(int32_t);
	void setTeam(int32_t);
	void setSkin(int32_t);
	void setColor(int32_t);
	void setCash(int32_t);
	void setWantedLevel(int32_t);
	void setHP(float);
	void setArmour(float);
	void setName(const std::string&);

	void msg(const std::string&);

private:
	int32_t m_ID;
	char m_Name[24];
};