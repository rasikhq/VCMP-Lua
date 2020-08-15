#pragma once
#include "pch.h"

#define DEFAULT_DURATION 1000

class Player;

class Object
{
public:
	static std::vector<Object*> s_Objects;

	static void Init(sol::state*);
	static void Register(Object*);
	static Object* Get(int32_t id);
	static void Unregister(Object*);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Object"; }

	/*** CONSTRUCTORS ***/
	Object(int32_t, int32_t, float, float, float, int32_t = 255);
	Object(int32_t, int32_t, sol::table);

	/*** DESTRUCTOR(S) ***/
	~Object() = default;

	/*** METHODS ***/
	bool destroy();
	int32_t getAlpha() const;
	void setAlpha(int32_t);
	void setAlphaEx(int32_t, uint32_t);
	
	void moveToDefault(float, float, float, uint32_t);
	void moveTo(float, float, float);
	void moveToExDefault(sol::table, uint32_t);
	void moveToEx(sol::table);

	void moveByDefault(float, float, float, uint32_t);
	void moveBy(float, float, float);
	void moveByExDefault(sol::table, uint32_t);
	void moveByEx(sol::table);

	void rotateToDefault(sol::table, uint32_t);
	void rotateTo(sol::table);
	void rotateByDefault(sol::table, uint32_t);
	void rotateBy(sol::table);

	/*** READ-ONLY ***/
	bool isStreamedForPlayer(Player*) const;
	int32_t getID() const;
	int32_t getModel() const;

	/*** PROPERTIES ***/
	int32_t getWorld() const;
	bool getShotReports() const;
	bool getBumpReports() const;
	
	void setWorld(int32_t);
	void setShotReports(bool);
	void setBumpReports(bool);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Object::getStaticType(); }

	sol::as_table_t<std::vector<float>> getPosition() const;
	void setPosition(sol::table);

	sol::as_table_t<std::vector<float>> getRotation() const;
	void setRotation(sol::table);
	/******/
private:
	int32_t m_ID;
};

