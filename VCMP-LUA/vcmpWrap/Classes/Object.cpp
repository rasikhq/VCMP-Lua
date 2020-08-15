#include "Object.h"
#include "Player.h"

extern PluginFuncs* g_Funcs;

std::vector<Object*> Object::s_Objects;

void Object::Register(Object* inst) {
	s_Objects.emplace_back(inst);
}

Object* Object::Get(int32_t id) {
	for (auto& Object : s_Objects) {
		if (Object->m_ID == id)
			return Object;
	}
	return nullptr;
}

void Object::Unregister(Object* Object) {
	for (auto it = s_Objects.begin(); it != s_Objects.end(); it++) {
		if ((*it)->m_ID == Object->getID()) {
			s_Objects.erase(it);
			break;
		}
	}
}

Object::Object(int32_t model, int32_t world, float x, float y, float z, int32_t alpha)
{
	m_ID = g_Funcs->CreateObject(model, world, x, y, z, alpha);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

Object::Object(int32_t model, int32_t world, sol::table transform)
{
	float x = transform.get_or(1, 0.0f);
	float y = transform.get_or(2, 0.0f);
	float z = transform.get_or(3, 0.0f);
	uint32_t alpha = transform.get_or(4, 255);
	m_ID = g_Funcs->CreateObject(model, world, x, y, z, alpha);
	if (g_Funcs->GetLastError() == vcmpError::vcmpErrorNone)
		Register(this);
}

/*** METHODS ***/
bool Object::destroy() {
	bool bDestroyed = g_Funcs->DeleteObject(m_ID) == vcmpError::vcmpErrorNone;
	if (bDestroyed)
		Unregister(this);
	return bDestroyed;
}

bool Object::isStreamedForPlayer(Player* player) const { return (g_Funcs->IsObjectStreamedForPlayer(m_ID, player->getID())); }

int32_t Object::getAlpha() const {
	return g_Funcs->GetObjectAlpha(m_ID);
}

void Object::setAlpha(int32_t alpha) { g_Funcs->SetObjectAlpha(m_ID, alpha, 0); }
void Object::setAlphaEx(int32_t alpha, uint32_t duration) { g_Funcs->SetObjectAlpha(m_ID, alpha, duration); }

void Object::moveToDefault(float x, float y, float z, uint32_t duration) { g_Funcs->MoveObjectTo(m_ID, x, y, z, duration); }
void Object::moveTo(float x, float y, float z) { g_Funcs->MoveObjectTo(m_ID, x, y, z, DEFAULT_DURATION); }
void Object::moveToExDefault(sol::table position, uint32_t duration) { g_Funcs->MoveObjectTo(m_ID, position.get_or(1, 0.0f), position.get_or(2, 0.0f), position.get_or(3, 0.0f), duration); }
void Object::moveToEx(sol::table transform) { g_Funcs->MoveObjectTo(m_ID, transform.get_or(1, 0.0f), transform.get_or(2, 0.0f), transform.get_or(3, 0.0f), transform.get_or(4, DEFAULT_DURATION)); }

void Object::moveByDefault(float x, float y, float z, uint32_t duration) { g_Funcs->MoveObjectBy(m_ID, x, y, z, duration); }
void Object::moveBy(float x, float y, float z) { g_Funcs->MoveObjectBy(m_ID, x, y, z, DEFAULT_DURATION); }
void Object::moveByExDefault(sol::table position, uint32_t duration) { g_Funcs->MoveObjectBy(m_ID, position.get_or(1, 0.0f), position.get_or(2, 0.0f), position.get_or(3, 0.0f), duration); }
void Object::moveByEx(sol::table transform) { g_Funcs->MoveObjectBy(m_ID, transform.get_or(1, 0.0f), transform.get_or(2, 0.0f), transform.get_or(3, 0.0f), transform.get_or(4, DEFAULT_DURATION)); }

void Object::rotateToDefault(sol::table rotation, uint32_t duration) {
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	float w = rotation.get_or(4, -1.0f);
	if (w == -1.0f)
		g_Funcs->RotateObjectToEuler(m_ID, x, y, z, duration);
	else
		g_Funcs->RotateObjectTo(m_ID, x, y, z, w, duration);
}

void Object::rotateTo(sol::table rotation) {
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	float w = rotation.get_or(4, -1.0f);
	if (w == -1.0f)
		g_Funcs->RotateObjectToEuler(m_ID, x, y, z, DEFAULT_DURATION);
	else
		g_Funcs->RotateObjectTo(m_ID, x, y, z, w, DEFAULT_DURATION);
}

void Object::rotateByDefault(sol::table rotation, uint32_t duration) {
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	float w = rotation.get_or(4, -1.0f);
	if (w == -1.0f)
		g_Funcs->RotateObjectByEuler(m_ID, x, y, z, duration);
	else
		g_Funcs->RotateObjectBy(m_ID, x, y, z, w, duration);
}

void Object::rotateBy(sol::table rotation) {
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	float w = rotation.get_or(4, -1.0f);
	if (w == -1.0f)
		g_Funcs->RotateObjectByEuler(m_ID, x, y, z, DEFAULT_DURATION);
	else
		g_Funcs->RotateObjectBy(m_ID, x, y, z, w, DEFAULT_DURATION);
}

/*** PROPERTIES ***/
int32_t Object::getID() const { return m_ID; }
int32_t Object::getModel() const { return g_Funcs->GetObjectModel(m_ID); }

int32_t Object::getWorld() const { return g_Funcs->GetObjectWorld(m_ID); }
void Object::setWorld(int32_t world) { g_Funcs->SetObjectWorld(m_ID, world); }

bool Object::getShotReports() const { return static_cast<bool>(g_Funcs->IsObjectShotReportEnabled(m_ID)); }
void Object::setShotReports(bool status) { g_Funcs->SetObjectShotReportEnabled(m_ID, static_cast<uint8_t>(status)); }

bool Object::getBumpReports() const { return static_cast<bool>(g_Funcs->IsObjectTouchedReportEnabled(m_ID)); }
void Object::setBumpReports(bool status) { g_Funcs->SetObjectTouchedReportEnabled(m_ID, static_cast<uint8_t>(status)); }

/*** COMMON PROPERTIES AMONGST ENTITIES ***/
sol::as_table_t<std::vector<float>> Object::getPosition() const
{
	float x, y, z;
	g_Funcs->GetObjectPosition(m_ID, &x, &y, &z);
	std::vector<float> position = { x, y, z };
	return sol::as_table(position);
}
void Object::setPosition(sol::table position) { g_Funcs->SetObjectPosition(m_ID, position[1], position[2], position[3]); }

sol::as_table_t<std::vector<float>> Object::getRotation() const
{
	float x, y, z;
	g_Funcs->GetObjectRotationEuler(m_ID, &x, &y, &z);
	std::vector<float> rotation = { x, y, z };
	return sol::as_table(rotation);
}

void Object::setRotation(sol::table rotation)
{
	float x = rotation.get_or(1, 0.0f);
	float y = rotation.get_or(2, 0.0f);
	float z = rotation.get_or(3, 0.0f);
	g_Funcs->RotateObjectByEuler(m_ID, x, y, z, 0);
}

/******/

void Object::Init(sol::state* L) {
	sol::usertype<Object> userdata = L->new_usertype<Object>("Object",
		sol::constructors<
				Object(int32_t, int32_t, float, float, float),
				Object(int32_t, int32_t, float, float, float, int32_t),

				Object(int32_t, int32_t, sol::table)
			>()
		);

	userdata["type"] = &Object::getStaticType;
	userdata["findByID"] = &Object::Get;
	userdata["count"] = []() { return s_Objects.size(); };

	/*** METHODS ***/
	userdata["destroy"] = &Object::destroy;
	userdata["getAlpha"] = &Object::getAlpha;
	userdata["setAlpha"] = sol::overload(&Object::setAlpha, &Object::setAlphaEx);
	userdata["moveTo"] = sol::overload(&Object::moveToDefault, &Object::moveTo, &Object::moveToExDefault, &Object::moveToEx);
	userdata["moveBy"] = sol::overload(&Object::moveByDefault, &Object::moveBy, &Object::moveByExDefault, &Object::moveByEx);
	userdata["rotateTo"] = sol::overload(&Object::rotateToDefault, &Object::rotateTo);
	userdata["rotateBy"] = sol::overload(&Object::rotateByDefault, &Object::rotateBy);
	
	/*** READ-ONLY ***/
	userdata["streamedForPlayer"] = &Object::isStreamedForPlayer;
	userdata.set("getID", &Object::getID);
	userdata.set("getModel", &Object::getModel);

	/*** PROPERTIES ***/
	userdata["world"] = sol::property(&Object::getWorld, &Object::setWorld);

	/*** COMMON PROPERTIES AMONGST ENTITIES ***/
	userdata["position"] = sol::property(&Object::getPosition, &Object::setPosition);
	userdata["angle"] = sol::property(&Object::getRotation, &Object::setRotation);
}