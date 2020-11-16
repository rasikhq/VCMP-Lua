#include "Checkpoint.h"
#include "Player.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

std::vector<Checkpoint*> Checkpoint::s_Checkpoints;

void Checkpoint::Register(Checkpoint* inst) {
	inst->m_LuaData = Lua.create_table();
	s_Checkpoints.emplace_back(inst);
}

Checkpoint* Checkpoint::Get(int32_t id) {
	for (auto& Checkpoint : s_Checkpoints) {
		if (Checkpoint->m_ID == id)
			return Checkpoint;
	}
	return nullptr;
}

void Checkpoint::Unregister(Checkpoint* Checkpoint) {
	for (auto it = s_Checkpoints.begin(); it != s_Checkpoints.end(); it++) {
		if ((*it)->m_ID == Checkpoint->getID()) {
			s_Checkpoints.erase(it);
			break;
		}
	}
}

sol::table Checkpoint::getActive()
{
	sol::table entities = Lua.create_table_with();
	for (auto entity : s_Checkpoints)
		entities[entity->getID()] = entity;
	return entities;
}

Checkpoint::Checkpoint(Player* player, int32_t world, bool isSphere, sol::table position, sol::table color, float radius)
{
	if (s_Checkpoints.size() >= s_Checkpoints.capacity()) {
		spdlog::error("Max Checkpoint entity limit of {} reached!", MAX_CHECKPOINTS);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreateCheckPoint(player != nullptr ? player->getID() : -1, world, isSphere, position[1], position[2], position[3], color[1], color[2], color[3], color[4].get_or<int32_t>(255u), radius);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

Checkpoint::Checkpoint(Player* player, int32_t world, bool isSphere, float x, float y, float z, sol::table color, float radius)
{
	if (s_Checkpoints.size() >= s_Checkpoints.capacity()) {
		spdlog::error("Max Checkpoint entity limit of {} reached!", MAX_CHECKPOINTS);
		throw("Entity limit reached!");
		return;
	}
	m_ID = g_Funcs->CreateCheckPoint(player != nullptr ? player->getID() : -1, world, isSphere, x, y, z, color[1], color[2], color[3], color[4].get_or<int32_t>(255u), radius);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

/*** METHODS ***/
bool Checkpoint::destroy() {
	bool bDestroyed = g_Funcs->DeleteCheckPoint(m_ID) == vcmpError::vcmpErrorNone;
	if (bDestroyed)
		Unregister(this);
	return bDestroyed;
}

bool Checkpoint::isStreamedForPlayer(Player* player) const { return (g_Funcs->IsCheckPointStreamedForPlayer(m_ID, player->getID())); }

int32_t Checkpoint::getID() const { return m_ID; }
Player* Checkpoint::getOwner() const { return Player::Get(g_Funcs->GetCheckPointOwner(m_ID)); }
bool Checkpoint::isSphere() const { return g_Funcs->IsCheckPointSphere(m_ID); }

/*** PROPERTIES ***/

int32_t Checkpoint::getWorld() const {
	return g_Funcs->GetCheckPointWorld(m_ID);
}
void Checkpoint::setWorld(int32_t world) {
	if (getWorld() == world)
		return;
	g_Funcs->SetCheckPointWorld(m_ID, world);
}

float Checkpoint::getRadius() const {
	return g_Funcs->GetCheckPointRadius(m_ID);
}
void Checkpoint::setRadius(float radius) {
	if (getWorld() == radius)
		return;
	g_Funcs->SetCheckPointRadius(m_ID, radius);
}

sol::as_table_t<std::vector<int32_t>> Checkpoint::getColor() const
{
	int32_t r, g, b, a;
	g_Funcs->GetCheckPointColour(m_ID, &r, &g, &b, &a);
	std::vector<int32_t> color = { r, g, b, a };
	return sol::as_table(color);
}

void Checkpoint::setColor(sol::table color)
{
	if (color.size() < 3) {
		spdlog::error("Checkpoint::setColor: Invalid color table!");
		throw("Invalid color forwarded, requires at-least 3 numbers!");
		return;
	}
	int32_t r = color[1], g = color[2], b = color[3];
	int32_t a = color.get_or(4, getAlpha());
	g_Funcs->SetCheckPointColour(m_ID, r, g, b, a);
}

int32_t Checkpoint::getAlpha() const
{
	int32_t r, g, b, a;
	g_Funcs->GetCheckPointColour(m_ID, &r, &g, &b, &a);
	return a;
}

void Checkpoint::setAlpha(int32_t alpha) 
{
	const auto& color = getColor().value();
	g_Funcs->SetCheckPointColour(m_ID, color.at(0), color.at(1), color.at(2), alpha);
}

/*** COMMON PROPERTIES AMONGST ENTITIES ***/
sol::as_table_t<std::vector<float>> Checkpoint::getPosition() const
{
	float x, y, z;
	g_Funcs->GetCheckPointPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}
void Checkpoint::setPosition(sol::table position) { 
	if (position.size() < 3) {
		spdlog::error("Checkpoint::setPosition: Invalid position table!");
		throw("Invalid position forwarded, requires 3 floats");
		return;
	}
	g_Funcs->SetCheckPointPosition(m_ID, position[1], position[2], position[3]); 
}

/******/

void Checkpoint::Init(sol::state* L) {
	s_Checkpoints.reserve(MAX_CHECKPOINTS);

	sol::usertype<Checkpoint> userdata = L->new_usertype<Checkpoint>("Checkpoint",
		sol::constructors<
		Checkpoint(Player*, int32_t, bool, sol::table, sol::table, float),
		Checkpoint(Player*, int32_t, bool, float, float, float, sol::table, float)
		>()
	);

	userdata["type"] = &Checkpoint::getStaticType;
	userdata["findByID"] = &Checkpoint::Get;
	userdata["count"] = []() { return s_Checkpoints.size(); };
	userdata["getActive"] = &Checkpoint::getActive;

	/*** METHODS ***/
	userdata["destroy"] = &Checkpoint::destroy;

	/*** READ-ONLY ***/
	userdata.set("getType", &Checkpoint::getType);
	userdata["streamedForPlayer"] = &Checkpoint::isStreamedForPlayer;
	userdata.set("getID", &Checkpoint::getID);
	userdata.set("getOwner", &Checkpoint::getOwner);
	userdata.set("isSphere", &Checkpoint::isSphere);

	/*** PROPERTIES ***/
	userdata["world"] = sol::property(&Checkpoint::getWorld, &Checkpoint::setWorld);
	userdata["radius"] = sol::property(&Checkpoint::getRadius, &Checkpoint::setRadius);
	userdata["color"] = sol::property(&Checkpoint::getColor, &Checkpoint::setColor);
	userdata["alpha"] = sol::property(&Checkpoint::getAlpha, &Checkpoint::setAlpha);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["data"] = &Checkpoint::m_LuaData;
	userdata["position"] = sol::property(&Checkpoint::getPosition, &Checkpoint::setPosition);
}