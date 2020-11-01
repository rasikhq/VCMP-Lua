#include "Bind.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

std::vector<Bind*> Bind::s_Binds;

void Bind::clearAllBinds() {
	g_Funcs->RemoveAllKeyBinds();
	s_Binds.clear();
}

void Bind::Register(Bind* bind) {
	s_Binds.emplace_back(bind);
}

void Bind::Unregister(Bind* bind) {
	for (auto it = s_Binds.begin(); it != s_Binds.end(); it++) {
		if ((*it)->m_ID == bind->getID()) {
			s_Binds.erase(it);
			break;
		}
	}
}

Bind* Bind::Get(int32_t id) {
	for (auto& bind : s_Binds) {
		if (bind->getID() == id)
			return bind;
	}
	return nullptr;
}

Bind* Bind::GetByTag(const std::string& tag) {
	for (auto& bind : s_Binds) {
		if (bind->getTag() == tag)
			return bind;
	}
	return nullptr;
}

Bind::Bind(bool signalOnRelease, int32_t keyOne, int32_t keyTwo, int32_t keyThree)
	: m_ID(g_Funcs->GetKeyBindUnusedSlot())
{
	g_Funcs->RegisterKeyBind(m_ID, static_cast<uint8_t>(signalOnRelease), keyOne, keyTwo, keyThree);
	Register(this);
}

/*** METHODS ***/
void Bind::destroy() {
	if(g_Funcs->RemoveKeyBind(m_ID) == vcmpError::vcmpErrorNone)
		Unregister(this);
}

int32_t Bind::getID() const {
	return m_ID;
}

sol::table Bind::getData() const {
	sol::table data;
	int32_t key1, key2, key3;
	uint8_t signalsOnRelease;
	if (g_Funcs->GetKeyBindData(m_ID, &signalsOnRelease, &key1, &key2, &key3) == vcmpError::vcmpErrorNoSuchEntity)
		return sol::nil;

	data = Lua.create_table();
	data["keyOne"] = key1;
	data["keyTwo"] = key2;
	data["keyThree"] = key3;
	data["signalsOnRelease"] = static_cast<bool>(signalsOnRelease);

	return data;
}

/*** PROPERTIES ***/
std::string Bind::getTag() const {
	return m_Tag;
}

void Bind::setTag(const std::string& newTag) {
	m_Tag = newTag;
}

void Bind::Init(sol::state* L) {
	/*** CONSTRUCTORS ***/
	sol::usertype<Bind> userdata = L->new_usertype<Bind>("Bind",
		sol::constructors<
			Bind(bool, int32_t),
			Bind(bool, int32_t, int32_t),
			Bind(bool, int32_t, int32_t, int32_t)
		>()
	);

	/*** @Lua static ***/
	userdata["type"] = &Bind::getStaticType;
	userdata["findByID"] = &Bind::Get;
	userdata["findByTag"] = &Bind::GetByTag;
	userdata["clearAllBinds"] = &Bind::clearAllBinds;

	/*** METHODS ***/

	/*** READ-ONLY ***/
	userdata.set("getID", &Bind::getID);
	userdata.set("getData", &Bind::getData);

	/*** PROPERTIES ***/
	userdata["tag"] = sol::property(&Bind::getTag, &Bind::setTag);
}