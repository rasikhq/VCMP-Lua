#pragma once

#include <string_view>

namespace vcmp_lua::bindings {

// The skin id a name refers to ("Tommy", "cop", "Beach Lady #2", ...), or -1.
int SkinId(std::string_view name);

// The skin's name, or null for an unknown id.
const char* SkinName(int id);

// The weapon id a name refers to ("M4", "uzi", ...); 255 if unknown, 0 for
// an empty name (v1's values).
int WeaponId(std::string_view name);

// The weapon's (or death reason's) name; "Unknown" for an unknown id.
const char* WeaponName(int id);

}  // namespace vcmp_lua::bindings
