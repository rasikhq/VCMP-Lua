#pragma once
#include <pch.h>
#include "../Constants.h"

class Player;

class Checkpoint
{
public:
	static std::vector<Checkpoint*> s_Checkpoints;

	static void Init(sol::state*);
	static void Register(Checkpoint*);
	static Checkpoint* Get(int32_t id);
	static void Unregister(Checkpoint*);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Checkpoint"; }
	static sol::table getActive();

	/*** @Lua meta-functions & extra ***/
	bool operator==(const Checkpoint& other) { return this->m_ID == other.m_ID; }
	int32_t operator()() { return this->m_ID; }

	/*** CONSTRUCTORS ***/
	Checkpoint(Player*, int32_t, bool, sol::table, sol::table, float);
	Checkpoint(Player*, int32_t, bool, float, float, float, sol::table, float);

	/*** DESTRUCTOR(S) ***/
	~Checkpoint() { destroy(); };

	/*** METHODS ***/
	bool destroy();

	/*** READ-ONLY ***/
	bool isStreamedForPlayer(Player*) const;
	int32_t getID() const;
	Player* getOwner() const;
	bool isSphere() const;

	/*** PROPERTIES ***/
	int32_t getWorld() const;
	float getRadius() const;
	sol::as_table_t<std::vector<int32_t>> getColor() const;
	int32_t getAlpha() const;

	void setWorld(int32_t);
	void setRadius(float);
	void setColor(sol::table);
	void setAlpha(int32_t);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Checkpoint::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);
private:
	int32_t m_ID;
	sol::table m_LuaData;
};