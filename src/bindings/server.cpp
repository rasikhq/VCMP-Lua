// Server, Map, Radio, Weapon, Blip and Sound: v1's Server.cpp, ported with
// fixes:
// - Server.addClass reads the optional weapons from the right arguments
//   (v1 was off by one), and the {x, y, z, angle} form is no longer
//   shadowed by the {x, y, z}, angle form (A3).
// - banIP/unbanIP/isIPBanned accept strings (A3: v1 refused them).
// - getSkinName of an unknown id is nil (v1 returned NULL as a string).
// - Blip.create returns a Blip handle (see docs/MIGRATION-v2.md).
// - Booleans are true/false.
#include <fmt/format.h>
#include <sol/sol.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "bindings/args.hpp"
#include "bindings/bindings.hpp"
#include "bindings/entity_type.hpp"
#include "bindings/names.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::bindings {
namespace {

// The global Server is the single instance of this usertype, so that
// Server.name and the other properties work like v1's static properties.
struct ServerTag {};

// A property of Server whose getter and setter take a Ctx.
template <typename Get>
auto ServerProperty(Get get) {
    return sol::property([get](const ServerTag&, sol::this_state L) {
        return get(Ctx{&Runtime::Require(L), L});
    });
}

template <typename Get, typename Set>
auto ServerProperty(Get get, Set set) {
    using Value = typename detail::ValueArg<Set>::type;
    return sol::property(
        [get](const ServerTag&, sol::this_state L) { return get(Ctx{&Runtime::Require(L), L}); },
        [set](const ServerTag&, sol::this_state L, Value value) {
            set(Ctx{&Runtime::Require(L), L}, std::move(value));
        });
}

std::string ReadServerText(const Ctx& ctx, vcmpError (*fn)(char*, size_t)) {
    return ReadText<512>(ctx.L, fn);
}

// The server's IP functions take a mutable char*.
std::vector<char> MutableText(const String& text) {
    return std::vector<char>(text.value.c_str(), text.value.c_str() + text.value.size() + 1);
}

// Server.addClass(team, colour, skin, x, y, z, angle[, weapon1, ammo1[, ...]]),
// (team, colour, skin, {x, y, z}, angle[, weapons...]) or
// (team, colour, skin, {x, y, z, angle}[, weapons...]). Returns the class id.
std::int32_t AddClass(Ctx ctx, const ArgReader& args) {
    const auto team = args.Int<std::int32_t>(1);
    const std::uint32_t colour = args.ColourAt(2);
    const auto skin = args.Int<std::int32_t>(3);
    int i = 4;
    const bool table = args.type(4) == LUA_TTABLE;
    const bool angle_in_table = table && TableLength(ctx.L, args.index(4)) >= 4;
    const Vec3 position = args.Vector(i);
    float angle = 0;
    if (angle_in_table) {
        angle = TableNumber(ctx.L, args.index(4), 4);
    } else {
        angle = args.Number(i++);
    }
    std::int32_t weapons[6] = {};
    for (int w = 0; w < 6; ++w) {
        weapons[w] = args.IntOr<std::int32_t>(i + w, 0);
    }
    const std::int32_t id = VCMP_FN(ctx, AddPlayerClass)(
        team, colour, skin, position.x, position.y, position.z, angle, weapons[0], weapons[1],
        weapons[2], weapons[3], weapons[4], weapons[5]);
    if (id < 0) {
        CheckLast(ctx.L, ctx.api());
        throw std::runtime_error("'" + CurrentFunction(ctx.L) + "' failed: the server refused");
    }
    return id;
}

void RegisterServerTable(sol::state& lua) {
    sol::usertype<ServerTag> type = lua.new_usertype<ServerTag>("Server", sol::no_constructor);
    type["type"] = [] { return "Server"; };

    // Server.reload(): at the end of the frame the plugin deletes what the
    // scripts created, closes the Lua state, reads luaconfig.lua again and
    // runs the scripts in a new state.
    type["reload"] = [](Ctx ctx) { ctx.runtime->RequestReload(); };
    // Server.shutdown(): the server shuts down at the end of the frame.
    type["shutdown"] = [](Ctx ctx) { VCMP_FN(ctx, ShutdownServer)(); };

    type["getOption"] = [](Ctx ctx, Int32 option) {
        const bool on =
            VCMP_FN(ctx, GetServerOption)(static_cast<vcmpServerOption>(option.value)) != 0;
        CheckLast(ctx.L, ctx.api());
        return on;
    };
    type["setOption"] = [](Ctx ctx, Int32 option, Boolean on) {
        return Check(ctx.L, VCMP_FN(ctx, SetServerOption)(
                                static_cast<vcmpServerOption>(option.value), on ? 1 : 0));
    };

    // Server.getSettings(): {maxPlayers, port, serverName, serverPassword, flags}.
    type["getSettings"] = [](Ctx ctx) {
        ServerSettings settings{};
        settings.structSize = sizeof(settings);
        Check(ctx.L, VCMP_FN(ctx, GetServerSettings)(&settings));
        settings.serverName[sizeof(settings.serverName) - 1] = '\0';
        return sol::state_view(ctx.L).create_table_with(
            "maxPlayers", settings.maxPlayers, "port", settings.port, "serverName",
            std::string(settings.serverName), "serverPassword",
            ReadServerText(ctx, VCMP_FN(ctx, GetServerPassword)), "flags", settings.flags);
    };

    const auto get_name = [](Ctx ctx) { return ReadServerText(ctx, VCMP_FN(ctx, GetServerName)); };
    const auto set_name = [](Ctx ctx, String name) {
        Check(ctx.L, VCMP_FN(ctx, SetServerName)(name.c_str()));
    };
    type["getName"] = get_name;
    type["setName"] = set_name;
    type["name"] = ServerProperty(get_name, set_name);

    const auto get_max_players = [](Ctx ctx) { return VCMP_FN(ctx, GetMaxPlayers)(); };
    const auto set_max_players = [](Ctx ctx, UInt32 count) {
        Check(ctx.L, VCMP_FN(ctx, SetMaxPlayers)(count));
    };
    type["getMaxPlayers"] = get_max_players;
    type["setMaxPlayers"] = set_max_players;
    type["maxPlayers"] = ServerProperty(get_max_players, set_max_players);

    const auto get_game = [](Ctx ctx) { return ReadServerText(ctx, VCMP_FN(ctx, GetGameModeText)); };
    const auto set_game = [](Ctx ctx, String text) {
        Check(ctx.L, VCMP_FN(ctx, SetGameModeText)(text.c_str()));
    };
    type["getGame"] = get_game;
    type["setGame"] = set_game;
    type["gamemode"] = ServerProperty(get_game, set_game);

    const auto get_password = [](Ctx ctx) {
        return ReadServerText(ctx, VCMP_FN(ctx, GetServerPassword));
    };
    const auto set_password = [](Ctx ctx, String password) {
        Check(ctx.L, VCMP_FN(ctx, SetServerPassword)(password.c_str()));
    };
    type["getPassword"] = get_password;
    type["setPassword"] = set_password;
    type["password"] = ServerProperty(get_password, set_password);

    type["addClass"] = [](Ctx ctx, sol::variadic_args args) {
        return AddClass(ctx, ArgReader(ctx.L, args.stack_index()));
    };
    type["setClassPosition"] = [](Ctx ctx, Vec3 v) {
        VCMP_FN(ctx, SetSpawnPlayerPosition)(v.x, v.y, v.z);
    };
    type["setClassCameraPosition"] = [](Ctx ctx, Vec3 v) {
        VCMP_FN(ctx, SetSpawnCameraPosition)(v.x, v.y, v.z);
    };
    type["setClassCameraLook"] = [](Ctx ctx, Vec3 v) {
        VCMP_FN(ctx, SetSpawnCameraLookAt)(v.x, v.y, v.z);
    };

    // Server.banIP(ip): false if it was banned already. unbanIP: false if
    // it was not banned.
    type["banIP"] = [](Ctx ctx, String ip) {
        std::vector<char> text = MutableText(ip);
        if (VCMP_FN(ctx, IsIPBanned)(text.data()) != 0) {
            return false;
        }
        VCMP_FN(ctx, BanIP)(text.data());
        return true;
    };
    type["unbanIP"] = [](Ctx ctx, String ip) {
        std::vector<char> text = MutableText(ip);
        return VCMP_FN(ctx, UnbanIP)(text.data()) != 0;
    };
    type["isIPBanned"] = [](Ctx ctx, String ip) {
        std::vector<char> text = MutableText(ip);
        return VCMP_FN(ctx, IsIPBanned)(text.data()) != 0;
    };

    // Server.createExplosion(world, type, position[, creator[, atGroundLevel]]).
    type["createExplosion"] = [](Ctx ctx, Int32 world, Int32 explosion, Vec3 at,
                                 Opt<Live<EntityKind::Player>> creator, Opt<Boolean> grounded) {
        return Check(ctx.L, VCMP_FN(ctx, CreateExplosion)(
                                world, explosion, at.x, at.y, at.z,
                                creator.has_value() ? creator->id : -1,
                                grounded.value_or(false) ? 1 : 0));
    };

    type["getSkinID"] = [](String name) { return SkinId(name.value); };
    // Server.getSkinName(id): the name, or nil for an unknown id.
    type["getSkinName"] = [](Int32 id) -> std::optional<std::string> {
        if (const char* name = SkinName(id)) {
            return std::string(name);
        }
        return std::nullopt;
    };

    type["fallTimer"] = ServerProperty(
        [](Ctx ctx) { return VCMP_FN(ctx, GetFallTimer)(); },
        [](Ctx ctx, UInt16 rate) { VCMP_FN(ctx, SetFallTimer)(rate); });
    type["timeRate"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetTimeRate)(); },
                                      [](Ctx ctx, Int32 rate) { VCMP_FN(ctx, SetTimeRate)(rate); });
    type["hour"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetHour)(); },
                                  [](Ctx ctx, Int32 hour) { VCMP_FN(ctx, SetHour)(hour); });
    type["minute"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetMinute)(); },
                                    [](Ctx ctx, Int32 minute) { VCMP_FN(ctx, SetMinute)(minute); });
    type["weather"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetWeather)(); },
                                     [](Ctx ctx, Int32 weather) { VCMP_FN(ctx, SetWeather)(weather); });
    type["gravity"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetGravity)(); },
                                     [](Ctx ctx, Float gravity) { VCMP_FN(ctx, SetGravity)(gravity); });
    type["waterLevel"] = ServerProperty(
        [](Ctx ctx) { return VCMP_FN(ctx, GetWaterLevel)(); },
        [](Ctx ctx, Float level) { VCMP_FN(ctx, SetWaterLevel)(level); });
    type["gameSpeed"] = ServerProperty([](Ctx ctx) { return VCMP_FN(ctx, GetGameSpeed)(); },
                                       [](Ctx ctx, Float speed) { VCMP_FN(ctx, SetGameSpeed)(speed); });
    type["flightAltitude"] = ServerProperty(
        [](Ctx ctx) { return VCMP_FN(ctx, GetMaximumFlightAltitude)(); },
        [](Ctx ctx, Float height) { VCMP_FN(ctx, SetMaximumFlightAltitude)(height); });
    type["vehicleRespawnHeight"] = ServerProperty(
        [](Ctx ctx) { return VCMP_FN(ctx, GetVehiclesForcedRespawnHeight)(); },
        [](Ctx ctx, Float height) { VCMP_FN(ctx, SetVehiclesForcedRespawnHeight)(height); });
    type["killDelay"] = ServerProperty(
        [](Ctx ctx) { return VCMP_FN(ctx, GetKillCommandDelay)(); },
        [](Ctx ctx, Int32 delay) { VCMP_FN(ctx, SetKillCommandDelay)(delay); });

    // Server.wastedSettings: {deathTimer, fadeTimer, fadeInSpeed, fadeOutSpeed,
    // fadeColour, corpseFadeStart, corpseFadeTime}. Assigning a table with
    // only some fields keeps the others.
    type["wastedSettings"] = ServerProperty(
        [](Ctx ctx) {
            std::uint32_t death = 0, fade = 0, colour = 0, corpse_start = 0, corpse_time = 0;
            float fade_in = 0, fade_out = 0;
            VCMP_FN(ctx, GetWastedSettings)(&death, &fade, &fade_in, &fade_out, &colour,
                                            &corpse_start, &corpse_time);
            return sol::state_view(ctx.L).create_table_with(
                "deathTimer", death, "fadeTimer", fade, "fadeInSpeed", fade_in, "fadeOutSpeed",
                fade_out, "fadeColour", colour, "corpseFadeStart", corpse_start, "corpseFadeTime",
                corpse_time);
        },
        [](Ctx ctx, sol::object value) {
            if (value.get_type() != sol::type::table) {
                TypeError(ctx.L, 3, "table");
            }
            std::uint32_t death = 0, fade = 0, colour = 0, corpse_start = 0, corpse_time = 0;
            float fade_in = 0, fade_out = 0;
            VCMP_FN(ctx, GetWastedSettings)(&death, &fade, &fade_in, &fade_out, &colour,
                                            &corpse_start, &corpse_time);
            value.push(ctx.L);
            const int table = lua_gettop(ctx.L);
            const auto integer = [&](const char* key, std::uint32_t& field) {
                if (lua_getfield(ctx.L, table, key) != LUA_TNIL) {
                    field = static_cast<std::uint32_t>(CheckColour(ctx.L, lua_gettop(ctx.L)));
                }
                lua_pop(ctx.L, 1);
            };
            const auto number = [&](const char* key, float& field) {
                if (lua_getfield(ctx.L, table, key) != LUA_TNIL) {
                    field = static_cast<float>(CheckNumber(ctx.L, lua_gettop(ctx.L)));
                }
                lua_pop(ctx.L, 1);
            };
            integer("deathTimer", death);
            integer("fadeTimer", fade);
            number("fadeInSpeed", fade_in);
            number("fadeOutSpeed", fade_out);
            integer("fadeColour", colour);
            integer("corpseFadeStart", corpse_start);
            integer("corpseFadeTime", corpse_time);
            lua_pop(ctx.L, 1);
            VCMP_FN(ctx, SetWastedSettings)(death, fade, fade_in, fade_out, colour, corpse_start,
                                            corpse_time);
        });

    lua["Server"] = ServerTag{};
}

// --- Map -------------------------------------------------------------------------

const char* DistrictName(float x, float y) {
    if (x > -1613.03f && y > 413.218f && x < -213.73f && y < 1677.32f) return "Downtown Vice City";
    if (x > 163.656f && y > -351.153f && x < 1246.03f && y < 1398.85f) return "Vice Point";
    if (x > -103.97f && y > -930.526f && x < 1246.03f && y < -351.153f) return "Washington Beach";
    if (x > -253.206f && y > -1805.37f && x < 1254.9f && y < -930.526f) return "Ocean Beach";
    if (x > -1888.21f && y > -1779.61f && x < -1208.21f && y < 230.39f)
        return "Escobar International Airport";
    if (x > -748.206f && y > -818.266f && x < -104.505f && y < -241.467f) return "Starfish Island";
    if (x > -213.73f && y > 797.605f && x < 163.656f && y < 1243.47f) return "Prawn Island";
    if (x > -213.73f && y > -241.429f && x < 163.656f && y < 797.605f) return "Leaf Links";
    if (x > -1396.76f && y > -42.9113f && x < -1208.21f && y < 230.39f) return "Junkyard";
    if (x > -1208.21f && y > -1779.61f && x < -253.206f && y < -898.738f) return "Viceport";
    if (x > -1208.21f && y > -898.738f && x < -748.206f && y < -241.467f) return "Little Havana";
    if (x > -1208.21f && y > -241.467f && x < -578.289f && y < 412.66f) return "Little Haiti";
    return "Vice City";
}

// World units to the tenths HideMapObject takes, rounded to the nearest.
// (v1 truncated floor(x * 10) + 0.5, which is off by one for negative
// coordinates: -2.0 became -19.)
std::int16_t Tenths(lua_State* L, float value, int index) {
    const double tenths = std::round(static_cast<double>(value) * 10.0);
    if (!(tenths >= -32768.0 && tenths <= 32767.0)) {
        ArgError(L, index, "coordinate out of range");
    }
    return static_cast<std::int16_t>(tenths);
}

void RegisterMap(sol::state& lua) {
    sol::table map = lua.create_named_table("Map");
    map["setBounds"] = [](Ctx ctx, Float max_x, Float min_x, Float max_y, Float min_y) {
        VCMP_FN(ctx, SetWorldBounds)(max_x, min_x, max_y, min_y);
    };
    // Map.getBounds(): {max_x, min_x, max_y, min_y}, as in v1.
    map["getBounds"] = [](Ctx ctx) {
        float max_x = 0, min_x = 0, max_y = 0, min_y = 0;
        VCMP_FN(ctx, GetWorldBounds)(&max_x, &min_x, &max_y, &min_y);
        return sol::state_view(ctx.L).create_table_with("max_x", max_x, "min_x", min_x, "max_y",
                                                        max_y, "min_y", min_y);
    };
    // Map.hideObject(model, x, y, z): a map object at a position in world units.
    map["hideObject"] = [](Ctx ctx, Int32 model, Vec3 at) {
        VCMP_FN(ctx, HideMapObject)(model, Tenths(ctx.L, at.x, 2), Tenths(ctx.L, at.y, 3),
                                    Tenths(ctx.L, at.z, 4));
    };
    // Map.hideObjectRaw(model, x, y, z): the position in tenths, as the server takes it.
    map["hideObjectRaw"] = [](Ctx ctx, Int32 model, Int16 x, Int16 y, Int16 z) {
        VCMP_FN(ctx, HideMapObject)(model, x, y, z);
    };
    const auto show = [](Ctx ctx, Int32 model, Vec3 at) {
        VCMP_FN(ctx, ShowMapObject)(model, Tenths(ctx.L, at.x, 2), Tenths(ctx.L, at.y, 3),
                                    Tenths(ctx.L, at.z, 4));
    };
    map["showMapObject"] = show;
    map["showObject"] = show;
    map["showAllObjects"] = [](Ctx ctx) { VCMP_FN(ctx, ShowAllMapObjects)(); };
    // Map.getDistrictName(x, y) or Map.getDistrictName({x, y[, z]}).
    map["getDistrictName"] = [](sol::this_state L, sol::variadic_args args) {
        const ArgReader reader(L, args.stack_index());
        if (reader.type(1) == LUA_TTABLE) {
            return DistrictName(TableNumber(L, reader.index(1), 1), TableNumber(L, reader.index(1), 2));
        }
        return DistrictName(reader.Number(1), reader.Number(2));
    };
}

// --- Radio -----------------------------------------------------------------------

void RegisterRadio(sol::state& lua) {
    sol::table radio = lua.create_named_table("Radio");
    // Radio.createStream([id,] name, url[, listed]): true on success. Without
    // an id the server picks one.
    radio["createStream"] = [](Ctx ctx, sol::variadic_args args) {
        const ArgReader reader(ctx.L, args.stack_index());
        const int first = reader.type(1) == LUA_TNUMBER ? 2 : 1;
        const auto id = first == 2 ? reader.Int<std::int32_t>(1) : -1;
        const std::string name = CheckString(ctx.L, reader.index(first));
        const std::string url = CheckString(ctx.L, reader.index(first + 1));
        const bool listed = reader.BoolOr(first + 2, true);
        return Check(ctx.L,
                     VCMP_FN(ctx, AddRadioStream)(id, name.c_str(), url.c_str(), listed ? 1 : 0));
    };
    radio["destroyStream"] = [](Ctx ctx, Int32 id) {
        return Check(ctx.L, VCMP_FN(ctx, RemoveRadioStream)(id));
    };
}

// --- Weapon ----------------------------------------------------------------------

void RegisterWeapon(sol::state& lua) {
    sol::table weapon = lua.create_named_table("Weapon");
    // Weapon.data(weapon, field) reads a value; Weapon.data(weapon, field, value) sets it.
    weapon["data"] = [](Ctx ctx, Int32 id, Int32 field, Opt<Double> value) -> std::optional<double> {
        if (value.has_value()) {
            Check(ctx.L, VCMP_FN(ctx, SetWeaponDataValue)(id, field, value->value));
            return std::nullopt;
        }
        const double current = VCMP_FN(ctx, GetWeaponDataValue)(id, field);
        CheckLast(ctx.L, ctx.api());
        return current;
    };
    weapon["isFieldModified"] = [](Ctx ctx, Int32 id, Int32 field) {
        const bool modified = VCMP_FN(ctx, IsWeaponDataValueModified)(id, field) != 0;
        CheckLast(ctx.L, ctx.api());
        return modified;
    };
    weapon["resetField"] = [](Ctx ctx, Int32 id, Int32 field) {
        return Check(ctx.L, VCMP_FN(ctx, ResetWeaponDataValue)(id, field));
    };
    weapon["reset"] = [](Ctx ctx, Int32 id) {
        return Check(ctx.L, VCMP_FN(ctx, ResetWeaponData)(id));
    };
    weapon["resetAll"] = [](Ctx ctx) { VCMP_FN(ctx, ResetAllWeaponData)(); };
    weapon["getName"] = [](Int32 id) { return WeaponName(id); };
    weapon["getID"] = [](String name) { return WeaponId(name.value); };
}

// --- Blip ------------------------------------------------------------------------

constexpr EntityKind kBlip = EntityKind::Blip;

// Blip.create([index,] world, position, scale, colour, sprite): position is
// {x, y, z} or three numbers; index picks the id (-1 or none: any).
EntityRef<kBlip> CreateBlip(Ctx ctx, const ArgReader& args) {
    // The forms differ in length: with an index and three numbers there are
    // eight arguments, with a table six; without an index seven or five.
    const bool table = args.type(2) == LUA_TTABLE || args.type(3) == LUA_TTABLE;
    const bool indexed = table ? args.type(3) == LUA_TTABLE : args.count() >= 8;
    int i = 1;
    const auto index = indexed ? args.Int<std::int32_t>(i++) : -1;
    const auto world = args.Int<std::int32_t>(i++);
    const Vec3 at = args.Vector(i);
    const auto scale = args.Int<std::int32_t>(i);
    const std::uint32_t colour = args.ColourAt(i + 1);
    const auto sprite = args.Int<std::int32_t>(i + 2);
    const std::int32_t id =
        VCMP_FN(ctx, CreateCoordBlip)(index, world, at.x, at.y, at.z, scale, colour, sprite);
    return Created<kBlip>(ctx.L, *ctx.runtime, id);
}

// A blip argument: a Blip handle, or a blip id as v1's Blip.create returned.
std::int32_t BlipId(Ctx ctx, int index) {
    if (lua_type(ctx.L, index) == LUA_TNUMBER) {
        return static_cast<std::int32_t>(CheckInteger(ctx.L, index, -1, 0x7FFFFFFF));
    }
    return CheckLive<kBlip>(ctx.L, index).id;
}

std::optional<sol::table> BlipInfo(Ctx ctx, std::int32_t id) {
    std::int32_t world = 0, scale = 0, sprite = 0;
    float x = 0, y = 0, z = 0;
    std::uint32_t colour = 0;
    if (VCMP_FN(ctx, GetCoordBlipInfo)(id, &world, &x, &y, &z, &scale, &colour, &sprite) !=
        vcmpErrorNone) {
        return std::nullopt;
    }
    sol::state_view lua(ctx.L);
    return lua.create_table_with("world", world, "position", Vec3{x, y, z}, "scale", scale,
                                 "color", colour, "sprite", sprite);
}

bool DestroyBlip(Ctx ctx, std::int32_t id) {
    const vcmpError error = VCMP_FN(ctx, DestroyCoordBlip)(id);
    ctx.runtime->Entities().Get(kBlip).Release(id);
    return error == vcmpErrorNoSuchEntity ? false : Check(ctx.L, error);
}

void RegisterBlipMembers(BlipType& type) {
    type["create"] = [](Ctx ctx, sol::variadic_args args) {
        return CreateBlip(ctx, ArgReader(ctx.L, args.stack_index()));
    };
    // Blip.destroy(blip) or blip:destroy(); also takes an id. False if no such blip.
    type["destroy"] = [](Ctx ctx, sol::variadic_args args) {
        return DestroyBlip(ctx, BlipId(ctx, args.stack_index()));
    };
    // Blip.getInfo(blip) or blip:getInfo(): {world, position, scale, color,
    // sprite}, or nil.
    type["getInfo"] = [](Ctx ctx, sol::variadic_args args) {
        return BlipInfo(ctx, BlipId(ctx, args.stack_index()));
    };
}

// --- Sound -----------------------------------------------------------------------

void RegisterSound(sol::state& lua) {
    sol::table sound = lua.create_named_table("Sound");
    // Sound.play(sound), (sound, position), (world, sound) or (world, sound,
    // position); position is {x, y, z} or three numbers. Without a position
    // the sound is not positional; the world defaults to 0.
    sound["play"] = [](Ctx ctx, sol::variadic_args args) {
        const ArgReader reader(ctx.L, args.stack_index());
        const int count = reader.count();
        const bool table_second = reader.type(2) == LUA_TTABLE;
        // With a world: (world, sound), (world, sound, {..}), (world, sound, x, y, z).
        const bool has_world = (count == 2 && !table_second) || count == 3 || count >= 5;
        int i = 1;
        const auto world = has_world ? reader.Int<std::int32_t>(i++) : 0;
        const auto id = reader.Int<std::int32_t>(i++);
        const float none = std::numeric_limits<float>::quiet_NaN();
        Vec3 at{none, none, none};
        if (!reader.missing(i)) {
            at = reader.Vector(i);
        }
        return Check(ctx.L, VCMP_FN(ctx, PlaySound)(world, id, at.x, at.y, at.z));
    };
}

}  // namespace

void RegisterServer(sol::state& lua) {
    RegisterServerTable(lua);
    RegisterMap(lua);
    RegisterRadio(lua);
    RegisterWeapon(lua);
    RegisterSound(lua);
}

void RegisterBlip(sol::state&, BlipType& type) {
    RegisterBlipMembers(type);
}

}  // namespace vcmp_lua::bindings
