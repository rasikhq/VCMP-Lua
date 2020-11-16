#pragma once
#include "pch.h"

class Bind
{
public:
	static std::vector<Bind*> s_Binds;

	static void Init(sol::state*);
	static void Register(Bind*);
	static Bind* Get(int32_t);
	static Bind* GetByTag(const std::string&);
	static void Unregister(Bind*);

	/*** @Lua static ***/
	static inline const char* getStaticType() { return "Bind"; }
	static void clearAllBinds();

	/*** @Lua meta-functions & extra ***/
	bool operator==(const Bind& other) { return this->m_ID == other.m_ID; }
	int32_t operator()() { return this->m_ID; }

	/*** CONSTRUCTORS ***/
	Bind(bool, int32_t, int32_t = 0, int32_t = 0);

	/*** DESTRUCTOR(S) ***/
	~Bind() { destroy(); }

	/*** METHODS ***/
	void destroy();
	sol::table getData() const;

	/*** READ-ONLY ***/
	int32_t getID() const;

	/*** PROPERTIES ***/
	std::string getTag() const;

	void setTag(const std::string&);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	inline const char* getType() { return Bind::getStaticType(); }

private:
	int32_t m_ID;
	std::string m_Tag = "";
	sol::table m_LuaData;
};