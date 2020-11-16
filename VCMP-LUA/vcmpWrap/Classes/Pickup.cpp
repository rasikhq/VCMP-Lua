#include "Pickup.h"
#include "Player.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

std::vector<Pickup*> Pickup::s_Pickups;

void Pickup::Register(Pickup* inst) {
	inst->m_LuaData = Lua.create_table();
	s_Pickups.emplace_back(inst);
}

Pickup* Pickup::Get(int32_t id) {
	for (auto& Pickup : s_Pickups) {
		if (Pickup->m_ID == id)
			return Pickup;
	}
	return nullptr;
}

void Pickup::Unregister(Pickup* Pickup) {
	for (auto it = s_Pickups.begin(); it != s_Pickups.end(); it++) {
		if ((*it)->m_ID == Pickup->getID()) {
			s_Pickups.erase(it);
			break;
		}
	}
}

sol::table Pickup::getActive()
{
	sol::table entities = Lua.create_table_with();
	for (auto entity : s_Pickups)
		entities[entity->getID()] = entity;
	return entities;
}

Pickup::Pickup(int32_t model, int32_t world, int32_t quantity, sol::table position, int32_t alpha, bool automatic)
{
	if (s_Pickups.size() >= s_Pickups.capacity()) {
		spdlog::error("Max Pickup entity limit of {} reached!", MAX_PICKUPS);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreatePickup(model, world, quantity, position[1], position[2], position[3], alpha, automatic);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

Pickup::Pickup(int32_t model, int32_t world, int32_t quantity, float x, float y, float z, int32_t alpha, bool automatic)
{
	if (s_Pickups.size() >= s_Pickups.capacity()) {
		spdlog::error("Max Pickup entity limit of {} reached!", MAX_PICKUPS);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreatePickup(model, world, quantity, x, y, z, alpha, automatic);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

/*** METHODS ***/
bool Pickup::destroy() {
	bool bDestroyed = g_Funcs->DeletePickup(m_ID) == vcmpError::vcmpErrorNone;
	if (bDestroyed)
		Unregister(this);
	return bDestroyed;
}

bool Pickup::respawn()
{
	return (g_Funcs->RefreshPickup(m_ID) == vcmpError::vcmpErrorNone);
}

bool Pickup::getOption(vcmpPickupOption option) const {
	return g_Funcs->GetPickupOption(m_ID, option);
}

void Pickup::setOption(vcmpPickupOption option, bool status) {
	g_Funcs->SetPickupOption(m_ID, option, static_cast<uint8_t>(status));
}

bool Pickup::isStreamedForPlayer(Player* player) const { return (g_Funcs->IsPickupStreamedForPlayer(m_ID, player->getID())); }

int32_t Pickup::getID() const { return m_ID; }
int32_t Pickup::getQuantity() const { return g_Funcs->GetPickupQuantity(m_ID); }

/*** PROPERTIES ***/

int32_t Pickup::getWorld() const
{
	return g_Funcs->GetPickupWorld(m_ID);
}

void Pickup::setWorld(int32_t world)
{
	g_Funcs->SetPickupWorld(m_ID, world);
}

int32_t Pickup::getAlpha() const
{
	return g_Funcs->GetPickupAlpha(m_ID);
}

void Pickup::setAlpha(int32_t alpha)
{
	g_Funcs->SetPickupAlpha(m_ID, alpha);
}

bool Pickup::getAuto() const
{
	return g_Funcs->IsPickupAutomatic(m_ID);
}

void Pickup::setAuto(bool automatic)
{
	g_Funcs->SetPickupIsAutomatic(m_ID, automatic);
}

uint32_t Pickup::getAutoTimer() const
{
	return g_Funcs->GetPickupAutoTimer(m_ID);
}

void Pickup::setAutoTimer(uint32_t timer)
{
	g_Funcs->SetPickupAutoTimer(m_ID, timer);
}


/*** COMMON PROPERTIES AMONGST ENTITIES ***/

sol::as_table_t<std::vector<float>> Pickup::getPosition() const
{
	float x, y, z;
	g_Funcs->GetPickupPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}
void Pickup::setPosition(sol::table position) {
	if (position.size() < 3) {
		spdlog::error("Pickup::setPosition: Invalid position table!");
		throw("Invalid position forwarded, requires 3 floats");
		return;
	}
	g_Funcs->SetPickupPosition(m_ID, position[1], position[2], position[3]);
}

/******/

void Pickup::Init(sol::state* L) {
	s_Pickups.reserve(MAX_PICKUPS);

	sol::usertype<Pickup> userdata = L->new_usertype<Pickup>("Pickup",
		sol::constructors<
		Pickup(int32_t, int32_t, int32_t, sol::table, int32_t, bool),
		Pickup(int32_t, int32_t, int32_t, float, float, float, int32_t, bool)
		>()
	);

	userdata["type"] = &Pickup::getStaticType;
	userdata["findByID"] = &Pickup::Get;
	userdata["count"] = []() { return s_Pickups.size(); };
	userdata["getActive"] = &Pickup::getActive;

	/*** METHODS ***/
	userdata["destroy"] = &Pickup::destroy;
	userdata["respawn"] = &Pickup::respawn;
	userdata["getOption"] = &Pickup::getOption;
	userdata["setOption"] = &Pickup::setOption;

	/*** READ-ONLY ***/
	userdata.set("getType", &Pickup::getType);
	userdata["streamedForPlayer"] = &Pickup::isStreamedForPlayer;
	userdata.set("getID", &Pickup::getID);

	/*** PROPERTIES ***/
	userdata["world"] = sol::property(&Pickup::getWorld, &Pickup::setWorld);
	userdata["alpha"] = sol::property(&Pickup::getAlpha, &Pickup::setAlpha);
	userdata["auto"] = sol::property(&Pickup::getAuto, &Pickup::setAuto);
	userdata["autoTimer"] = sol::property(&Pickup::getAutoTimer, &Pickup::setAutoTimer);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["data"] = &Pickup::m_LuaData;
	userdata["position"] = sol::property(&Pickup::getPosition, &Pickup::setPosition);
}