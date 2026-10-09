// Player: v1's members, ported with fixes:
// - Handles are cached per lifetime and checked on every use, never
//   pointers into a vector (A2).
// - Messages are sent with a "%s" format: v1 passed the text as the format,
//   so a "%n" in a chat message reached printf (A0-class bug).
// - name is read from the server, not cached (it went stale); setting it
//   checks the length instead of strcpy into 24 bytes (A0).
// - ip, uid and uid2 check the server's result (A2).
// - vehicle = nil and spectateTarget = nil work instead of dereferencing
//   null (A2).
// - ammo is the current weapon's ammo (v1's always raised an error), and
//   getActive(true) returns only spawned players (A3).
// - Booleans are true/false, not 0/1 (A3).
#include <sol/sol.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include "bindings/args.hpp"
#include "bindings/entity_type.hpp"

namespace vcmp_lua::bindings {
namespace {

constexpr EntityKind kPlayer = EntityKind::Player;
constexpr EntityKind kVehicle = EntityKind::Vehicle;
using Self = Live<kPlayer>;

constexpr std::uint32_t kWhite = 0xFFFFFFFF;

bool Message(const Self& self, std::uint32_t colour, const std::string& text) {
    return Check(self.L, VCMP_FN(self, SendClientMessage)(self.id, colour, "%s", text.c_str()));
}

bool Announce(const Self& self, std::int32_t type, const std::string& text) {
    return Check(self.L, VCMP_FN(self, SendGameMessage)(self.id, type, "%s", text.c_str()));
}

Vec3 Position(const Self& self) {
    Vec3 position;
    Check(self.L, VCMP_FN(self, GetPlayerPosition)(self.id, &position.x, &position.y, &position.z));
    return position;
}

bool PlaySoundAt(const Self& self, std::int32_t sound, const Vec3& at) {
    const std::int32_t world = VCMP_FN(self, GetPlayerUniqueWorld)(self.id);
    return Check(self.L, VCMP_FN(self, PlaySound)(world, sound, at.x, at.y, at.z));
}

// Every connected player, for msgAll and announceAll.
template <typename Fn>
void ForEachPlayer(Ctx ctx, Fn&& fn) {
    EntityPool& players = ctx.runtime->Entities().Players();
    for (const std::int32_t id : players.AliveIds()) {
        const PlayerRef ref = players.Ref<kPlayer>(id);
        fn(Self{ctx.runtime, ctx.L, ref.id, ref.generation});
    }
}

}  // namespace

void RegisterPlayer(sol::state&, PlayerType& type) {
    // --- Static ----------------------------------------------------------------

    // Player.getActive([spawnedOnly]): {[id] = player}.
    type["getActive"] = [](Ctx ctx, Opt<Boolean> spawned_only) {
        EntityPool& players = ctx.runtime->Entities().Players();
        const bool only_spawned = spawned_only.value_or(false);
        sol::table result = sol::state_view(ctx.L).create_table();
        for (const std::int32_t id : players.AliveIds()) {
            if (only_spawned && VCMP_FN(ctx, IsPlayerSpawned)(id) == 0) {
                continue;
            }
            result.raw_set(id, players.Ref<kPlayer>(id));
        }
        return result;
    };

    // Player.findByName(name): the connected player with that name, or nil.
    type["findByName"] = [](Ctx ctx, String name) {
        const std::int32_t id = VCMP_FN(ctx, GetPlayerIdFromName)(name.c_str());
        return RefOf<kPlayer>(*ctx.runtime, id);
    };

    type["msgAll"] = [](Ctx ctx, String text, Opt<Colour> colour) {
        ForEachPlayer(ctx, [&](const Self& player) {
            VCMP_FN(player, SendClientMessage)
            (player.id, colour.value_or(kWhite), "%s", text.c_str());
        });
    };

    type["announceAll"] = [](Ctx ctx, String text, Opt<Int32> announce_type) {
        ForEachPlayer(ctx, [&](const Self& player) {
            VCMP_FN(player, SendGameMessage)
            (player.id, announce_type.value_or(0), "%s", text.c_str());
        });
    };

    // --- Methods ---------------------------------------------------------------

    type["msg"] = [](Self self, String text, Opt<Colour> colour) {
        return Message(self, colour.value_or(kWhite), text.value);
    };
    type["announce"] = [](Self self, String text, Opt<Int32> announce_type) {
        return Announce(self, announce_type.value_or(0), text.value);
    };

    type["getAlpha"] = [](Self self) { return VCMP_FN(self, GetPlayerAlpha)(self.id); };
    type["setAlpha"] = [](Self self, Int32 alpha, Opt<UInt32> fade_time) {
        return Check(self.L, VCMP_FN(self, SetPlayerAlpha)(self.id, alpha, fade_time.value_or(0u)));
    };

    type["getOption"] = [](Self self, Int32 option) {
        const bool value = VCMP_FN(self, GetPlayerOption)(
                               self.id, static_cast<vcmpPlayerOption>(option.value)) != 0;
        CheckLast(self.L, self.api());
        return value;
    };
    type["setOption"] = [](Self self, Int32 option, Boolean on) {
        return Check(self.L, VCMP_FN(self, SetPlayerOption)(
                                 self.id, static_cast<vcmpPlayerOption>(option.value), on ? 1 : 0));
    };

    // player:isPlayerStreamed(other): whether this player is streamed in for other.
    type["isPlayerStreamed"] = [](Self self, Self other) {
        return VCMP_FN(self, IsPlayerStreamedForPlayer)(self.id, other.id) != 0;
    };

    type["forceSpawn"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, ForcePlayerSpawn)(self.id));
    };
    type["selectClass"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, ForcePlayerSelect)(self.id));
    };

    // player:playSound(sound): for this player only, not positional.
    type["playSound"] = [](Self self, Int32 sound) {
        const float none = std::numeric_limits<float>::quiet_NaN();
        return PlaySoundAt(self, sound, Vec3{none, none, none});
    };
    // player:playSound3D(sound[, position]): at the position, or at the player.
    type["playSound3D"] = [](Self self, Int32 sound, Opt<Vec3> at) {
        return PlaySoundAt(self, sound, at.has_value() ? *at : Position(self));
    };

    type["setWeapon"] = [](Self self, Int32 weapon, Int32 ammo) {
        return Check(self.L, VCMP_FN(self, SetPlayerWeapon)(self.id, weapon, ammo));
    };
    type["giveWeapon"] = [](Self self, Int32 weapon, Int32 ammo) {
        return Check(self.L, VCMP_FN(self, GivePlayerWeapon)(self.id, weapon, ammo));
    };
    type["removeWeapon"] = [](Self self, Int32 weapon) {
        return Check(self.L, VCMP_FN(self, RemovePlayerWeapon)(self.id, weapon));
    };
    type["disarm"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, RemoveAllWeapons)(self.id));
    };
    type["getWeaponAtSlot"] = [](Self self, Int32 slot) {
        const std::int32_t weapon = VCMP_FN(self, GetPlayerWeaponAtSlot)(self.id, slot);
        CheckLast(self.L, self.api());
        return weapon;
    };
    type["getAmmoAtSlot"] = [](Self self, Int32 slot) {
        const std::int32_t ammo = VCMP_FN(self, GetPlayerAmmoAtSlot)(self.id, slot);
        CheckLast(self.L, self.api());
        return ammo;
    };
    type["giveMoney"] = [](Self self, Int32 amount) {
        return Check(self.L, VCMP_FN(self, GivePlayerMoney)(self.id, amount));
    };
    type["addSpeed"] = [](Self self, Vec3 speed) {
        return Check(self.L, VCMP_FN(self, AddPlayerSpeed)(self.id, speed.x, speed.y, speed.z));
    };

    // player:setVehicle(vehicle[, slot]): false if the server refuses.
    type["setVehicle"] = [](Self self, Live<kVehicle> vehicle, Opt<Int32> slot) {
        return Check(
            self.L, VCMP_FN(self, PutPlayerInVehicle)(self.id, vehicle.id, slot.value_or(0), 1, 1));
    };
    type["eject"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, RemovePlayerFromVehicle)(self.id));
    };

    // player:redirect(ip, port[, serverPassword[, userPassword]]).
    type["redirect"] = [](Self self, String ip, UInt32 port, Opt<String> server_password,
                          Opt<String> user_password) {
        const std::string name = ReadText(self.L, VCMP_FN(self, GetPlayerName), self.id);
        return Check(self.L, VCMP_FN(self, RedirectPlayerToServer)(
                                 self.id, ip.c_str(), port, name.c_str(),
                                 server_password.value_or(std::string()).c_str(),
                                 user_password.value_or(std::string()).c_str()));
    };

    // player:setCamera(position, lookAt): two tables or six numbers.
    type["setCamera"] = [](Self self, Vec3 position, Vec3 look_at) {
        return Check(self.L,
                     VCMP_FN(self, SetCameraPosition)(self.id, position.x, position.y, position.z,
                                                      look_at.x, look_at.y, look_at.z));
    };
    type["restoreCamera"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, RestoreCamera)(self.id));
    };
    // player:interpolateCamLookAt(lookAt, ms).
    type["interpolateCamLookAt"] = [](Self self, Vec3 look_at, UInt32 ms) {
        return Check(self.L, VCMP_FN(self, InterpolateCameraLookAt)(self.id, look_at.x, look_at.y,
                                                                    look_at.z, ms));
    };

    // player:kick() and player:ban(): the server reports the disconnect
    // before they return, so the handle is dead afterwards.
    type["kick"] = [](Self self) { Check(self.L, VCMP_FN(self, KickPlayer)(self.id)); };
    type["ban"] = [](Self self) { Check(self.L, VCMP_FN(self, BanPlayer)(self.id)); };
    type["kill"] = [](Self self) { return Check(self.L, VCMP_FN(self, KillPlayer)(self.id)); };

    // player:setAnimation(animation) or player:setAnimation(group, animation).
    type["setAnimation"] = [](Self self, Int32 first, Opt<Int32> second) {
        const std::int32_t group = second.has_value() ? first.value : 0;
        const std::int32_t animation = second.has_value() ? second->value : first.value;
        return Check(self.L, VCMP_FN(self, SetPlayerAnimation)(self.id, group, animation));
    };

    type["set3DArrowToPlayer"] = [](Self self, Self target, Boolean on) {
        return Check(self.L,
                     VCMP_FN(self, SetPlayer3DArrowForPlayer)(self.id, target.id, on ? 1 : 0));
    };
    type["get3DArrowToPlayer"] = [](Self self, Self target) {
        return VCMP_FN(self, GetPlayer3DArrowForPlayer)(self.id, target.id) != 0;
    };

    type["setDrunkHandling"] = [](Self self, UInt32 level) {
        return Check(self.L, VCMP_FN(self, SetPlayerDrunkHandling)(self.id, level));
    };
    type["setDrunkVisuals"] = [](Self self, UInt8 level) {
        return Check(self.L, VCMP_FN(self, SetPlayerDrunkVisuals)(self.id, level));
    };

    // player:getNetworkStatistics(NetworkStatistics.x): new (plugin API 2.1).
    type["getNetworkStatistics"] = [](Self self, Int32 option) {
        const double value = VCMP_FN(self, GetNetworkStatistics)(
            self.id, static_cast<vcmpNetworkStatisticsOption>(option.value));
        CheckLast(self.L, self.api());
        return value;
    };

    // player:getModules(): the list arrives in onPlayerModuleList.
    type["getModules"] = [](Self self) {
        return Check(self.L, VCMP_FN(self, GetPlayerModuleList)(self.id));
    };

    // --- Read-only, as v1's get*/is* methods and as properties -----------------

    const auto ip = [](Self self) { return ReadText(self.L, VCMP_FN(self, GetPlayerIP), self.id); };
    const auto uid = [](Self self) {
        return ReadText(self.L, VCMP_FN(self, GetPlayerUID), self.id);
    };
    const auto uid2 = [](Self self) {
        return ReadText(self.L, VCMP_FN(self, GetPlayerUID2), self.id);
    };
    const auto key = [](Self self) { return VCMP_FN(self, GetPlayerKey)(self.id); };
    const auto state = [](Self self) {
        return static_cast<std::int32_t>(VCMP_FN(self, GetPlayerState)(self.id));
    };
    const auto unique_world = [](Self self) {
        return VCMP_FN(self, GetPlayerUniqueWorld)(self.id);
    };
    const auto player_class = [](Self self) { return VCMP_FN(self, GetPlayerClass)(self.id); };
    const auto online = [](Self self) { return VCMP_FN(self, IsPlayerConnected)(self.id) != 0; };
    const auto spawned = [](Self self) { return VCMP_FN(self, IsPlayerSpawned)(self.id) != 0; };
    const auto typing = [](Self self) { return VCMP_FN(self, IsPlayerTyping)(self.id) != 0; };
    const auto crouching = [](Self self) { return VCMP_FN(self, IsPlayerCrouching)(self.id) != 0; };
    const auto away = [](Self self) { return VCMP_FN(self, IsPlayerAway)(self.id) != 0; };
    const auto ping = [](Self self) { return VCMP_FN(self, GetPlayerPing)(self.id); };
    const auto fps = [](Self self) { return VCMP_FN(self, GetPlayerFPS)(self.id); };
    const auto drunk_handling = [](Self self) {
        return VCMP_FN(self, GetPlayerDrunkHandling)(self.id);
    };
    const auto drunk_visuals = [](Self self) {
        return static_cast<std::int32_t>(VCMP_FN(self, GetPlayerDrunkVisuals)(self.id));
    };

    type["getIP"] = ip;
    type["getUID"] = uid;
    type["getUID2"] = uid2;
    type["getKey"] = key;
    type["getState"] = state;
    type["getUniqueWorld"] = unique_world;
    type["getClass"] = player_class;
    type["isOnline"] = online;
    type["isSpawned"] = spawned;
    type["isTyping"] = typing;
    type["isCrouching"] = crouching;
    type["isAway"] = away;
    type["getPing"] = ping;
    type["getFPS"] = fps;
    type["getDrunkHandling"] = drunk_handling;
    type["getDrunkVisuals"] = drunk_visuals;

    type["ip"] = Property<kPlayer>(ip);
    type["uid"] = Property<kPlayer>(uid);
    type["uid2"] = Property<kPlayer>(uid2);
    type["key"] = Property<kPlayer>(key);
    type["state"] = Property<kPlayer>(state);
    type["uniqueWorld"] = Property<kPlayer>(unique_world);
    type["class"] = Property<kPlayer>(player_class);
    type["online"] = Property<kPlayer>(online);
    type["spawned"] = Property<kPlayer>(spawned);
    type["typing"] = Property<kPlayer>(typing);
    type["crouching"] = Property<kPlayer>(crouching);
    type["away"] = Property<kPlayer>(away);
    type["ping"] = Property<kPlayer>(ping);
    type["fps"] = Property<kPlayer>(fps);
    type["cameraLocked"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, IsCameraLocked)(self.id) != 0; });
    type["onFire"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, IsPlayerOnFire)(self.id) != 0; });
    type["action"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerAction)(self.id); });
    type["gameKeys"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerGameKeys)(self.id); });
    type["weapon"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerWeapon)(self.id); });
    type["ammo"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerWeaponAmmo)(self.id); });
    type["vehicleSlot"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerInVehicleSlot)(self.id); });
    type["vehicleStatus"] = Property<kPlayer>([](Self self) {
        return static_cast<std::int32_t>(VCMP_FN(self, GetPlayerInVehicleStatus)(self.id));
    });
    type["aimPosition"] = Property<kPlayer>([](Self self) {
        Vec3 v;
        Check(self.L, VCMP_FN(self, GetPlayerAimPosition)(self.id, &v.x, &v.y, &v.z));
        return v;
    });
    type["aimDirection"] = Property<kPlayer>([](Self self) {
        Vec3 v;
        Check(self.L, VCMP_FN(self, GetPlayerAimDirection)(self.id, &v.x, &v.y, &v.z));
        return v;
    });
    type["standingOnVehicle"] = Property<kPlayer>([](Self self) {
        return RefOf<kVehicle>(*self.runtime, VCMP_FN(self, GetPlayerStandingOnVehicle)(self.id));
    });
    type["standingOnObject"] = Property<kPlayer>([](Self self) {
        return RefOf<EntityKind::Object>(*self.runtime,
                                         VCMP_FN(self, GetPlayerStandingOnObject)(self.id));
    });

    // --- Properties ------------------------------------------------------------

    type["admin"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, IsPlayerAdmin)(self.id) != 0; },
                          [](Self self, Boolean on) {
                              Check(self.L, VCMP_FN(self, SetPlayerAdmin)(self.id, on ? 1 : 0));
                          });
    type["world"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerWorld)(self.id); },
                          [](Self self, Int32 world) {
                              Check(self.L, VCMP_FN(self, SetPlayerWorld)(self.id, world));
                          });
    type["secondaryWorld"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerSecondaryWorld)(self.id); },
                          [](Self self, Int32 world) {
                              Check(self.L, VCMP_FN(self, SetPlayerSecondaryWorld)(self.id, world));
                          });
    type["team"] = Property<kPlayer>(
        [](Self self) { return VCMP_FN(self, GetPlayerTeam)(self.id); },
        [](Self self, Int32 team) { Check(self.L, VCMP_FN(self, SetPlayerTeam)(self.id, team)); });
    type["skin"] = Property<kPlayer>(
        [](Self self) { return VCMP_FN(self, GetPlayerSkin)(self.id); },
        [](Self self, Int32 skin) { Check(self.L, VCMP_FN(self, SetPlayerSkin)(self.id, skin)); });
    type["color"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerColour)(self.id); },
                          [](Self self, Colour colour) {
                              Check(self.L, VCMP_FN(self, SetPlayerColour)(self.id, colour));
                          });
    type["cash"] = Property<kPlayer>(
        [](Self self) { return VCMP_FN(self, GetPlayerMoney)(self.id); },
        [](Self self, Int32 cash) { Check(self.L, VCMP_FN(self, SetPlayerMoney)(self.id, cash)); });
    type["score"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerScore)(self.id); },
                          [](Self self, Int32 score) {
                              Check(self.L, VCMP_FN(self, SetPlayerScore)(self.id, score));
                          });
    type["wantedLevel"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerWantedLevel)(self.id); },
                          [](Self self, Int32 level) {
                              Check(self.L, VCMP_FN(self, SetPlayerWantedLevel)(self.id, level));
                          });
    type["immunity"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerImmunityFlags)(self.id); },
                          [](Self self, UInt32 flags) {
                              Check(self.L, VCMP_FN(self, SetPlayerImmunityFlags)(self.id, flags));
                          });
    type["health"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerHealth)(self.id); },
                          [](Self self, Float health) {
                              Check(self.L, VCMP_FN(self, SetPlayerHealth)(self.id, health));
                          });
    type["armour"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerArmour)(self.id); },
                          [](Self self, Float armour) {
                              Check(self.L, VCMP_FN(self, SetPlayerArmour)(self.id, armour));
                          });
    type["name"] = Property<kPlayer>(
        [](Self self) { return ReadText(self.L, VCMP_FN(self, GetPlayerName), self.id); },
        [](Self self, String name) {
            Check(self.L, VCMP_FN(self, SetPlayerName)(self.id, name.c_str()));
        });
    type["weaponSlot"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerWeaponSlot)(self.id); },
                          [](Self self, Int32 slot) {
                              Check(self.L, VCMP_FN(self, SetPlayerWeaponSlot)(self.id, slot));
                          });
    type["drunkHandling"] = Property<kPlayer>(drunk_handling, [](Self self, UInt32 level) {
        Check(self.L, VCMP_FN(self, SetPlayerDrunkHandling)(self.id, level));
    });
    type["drunkVisuals"] = Property<kPlayer>(drunk_visuals, [](Self self, UInt8 level) {
        Check(self.L, VCMP_FN(self, SetPlayerDrunkVisuals)(self.id, level));
    });

    // player.vehicle: the vehicle the player is in, or nil. Assigning nil
    // removes the player from it; assigning a vehicle puts them in as driver.
    type["vehicle"] = Property<kPlayer>(
        [](Self self) {
            return RefOf<kVehicle>(*self.runtime, VCMP_FN(self, GetPlayerVehicleId)(self.id));
        },
        [](Self self, Opt<Live<kVehicle>> vehicle) {
            if (!vehicle.has_value()) {
                if (VCMP_FN(self, GetPlayerVehicleId)(self.id) > 0) {
                    Check(self.L, VCMP_FN(self, RemovePlayerFromVehicle)(self.id));
                }
                return;
            }
            Check(self.L, VCMP_FN(self, PutPlayerInVehicle)(self.id, vehicle->id, 0, 1, 1));
        });

    // player.spectateTarget: the player being spectated, or nil to stop.
    type["spectateTarget"] = Property<kPlayer>(
        [](Self self) {
            return RefOf<kPlayer>(*self.runtime, VCMP_FN(self, GetPlayerSpectateTarget)(self.id));
        },
        [](Self self, Opt<Self> target) {
            Check(self.L, VCMP_FN(self, SetPlayerSpectateTarget)(
                              self.id, target.has_value() ? target->id : -1));
        });

    type["position"] = Property<kPlayer>(
        [](Self self) { return Position(self); },
        [](Self self, Vec3 position) {
            Check(self.L,
                  VCMP_FN(self, SetPlayerPosition)(self.id, position.x, position.y, position.z));
        });
    type["speed"] = Property<kPlayer>(
        [](Self self) {
            Vec3 v;
            Check(self.L, VCMP_FN(self, GetPlayerSpeed)(self.id, &v.x, &v.y, &v.z));
            return v;
        },
        [](Self self, Vec3 speed) {
            Check(self.L, VCMP_FN(self, SetPlayerSpeed)(self.id, speed.x, speed.y, speed.z));
        });
    type["angle"] =
        Property<kPlayer>([](Self self) { return VCMP_FN(self, GetPlayerHeading)(self.id); },
                          [](Self self, Float angle) {
                              Check(self.L, VCMP_FN(self, SetPlayerHeading)(self.id, angle));
                          });
}

}  // namespace vcmp_lua::bindings
