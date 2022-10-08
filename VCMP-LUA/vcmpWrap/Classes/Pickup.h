#pragma once
#include "../Constants.h"

class Player;

class Pickup
{
public:
	static std::vector<Pickup*> s_Pickups;

	static void Init(sol::state*);
	static void Register(Pickup*);
	static Pickup* Get(int32_t id);
	static void Unregister(Pickup*);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Pickup"; }
	static sol::table getActive();

	/*** @Lua meta-functions & extra ***/
	bool operator==(const Pickup& other) { return this->m_ID == other.m_ID; }
	int32_t operator()() { return this->m_ID; }

	/*** CONSTRUCTORS ***/
	Pickup(int32_t, int32_t, int32_t, sol::table, int32_t, bool);
	Pickup(int32_t, int32_t, int32_t, float, float, float, int32_t, bool);

	/*** DESTRUCTOR(S) ***/
	~Pickup() { destroy(); };

	/*** METHODS ***/
	bool destroy();
	bool respawn();
	bool getOption(vcmpPickupOption) const;
	void setOption(vcmpPickupOption, bool status);

	/*** READ-ONLY ***/
	bool isStreamedForPlayer(Player*) const;
	int32_t getID() const;
	int32_t getQuantity() const;
	int32_t getModel() const;

	/*** PROPERTIES ***/
	int32_t getWorld() const;
	int32_t getAlpha() const;
	bool getAuto() const;
	uint32_t getAutoTimer() const;

	void setWorld(int32_t);
	void setAlpha(int32_t);
	void setAuto(bool);
	void setAutoTimer(uint32_t);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Pickup::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);
private:
	int32_t m_ID;
	sol::table m_LuaData;
};