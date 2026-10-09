// The read-only constant tables (ServerOption, PlayerOption, ...).
#include <vcmp.h>
#include <sol/sol.hpp>

#include <cstdint>
#include <initializer_list>
#include <string_view>
#include <utility>

#include "bindings/bindings.hpp"

namespace vcmp_lua::bindings {
namespace {

using Items = std::initializer_list<std::pair<std::string_view, std::int64_t>>;

void Enum(sol::state& lua, std::string_view name, Items items) {
    lua.new_enum<std::int64_t, true>(name, items);
}

}  // namespace

void RegisterEnums(sol::state& lua) {
    Enum(lua, "ServerOption",
         {
             {"syncFrameLimiter", vcmpServerOptionSyncFrameLimiter},
             {"frameLimit", vcmpServerOptionFrameLimiter},
             {"taxiBoostJump", vcmpServerOptionTaxiBoostJump},
             {"driveOnWater", vcmpServerOptionDriveOnWater},
             {"fastSwitch", vcmpServerOptionFastSwitch},
             {"friendlyFire", vcmpServerOptionFriendlyFire},
             {"driveBy", vcmpServerOptionDisableDriveBy},
             {"perfectHandling", vcmpServerOptionPerfectHandling},
             {"flyingCars", vcmpServerOptionFlyingCars},
             {"jumpSwitch", vcmpServerOptionJumpSwitch},
             {"showMarkers", vcmpServerOptionShowMarkers},
             {"teamMarkersOnly", vcmpServerOptionOnlyShowTeamMarkers},
             {"stuntBike", vcmpServerOptionStuntBike},
             {"shootInAir", vcmpServerOptionShootInAir},
             {"nametags", vcmpServerOptionShowNameTags},
             {"joinMessages", vcmpServerOptionJoinMessages},
             {"deathMessages", vcmpServerOptionDeathMessages},
             {"chatTags", vcmpServerOptionChatTagsEnabled},
             {"classes", vcmpServerOptionUseClasses},
             {"wallGlitch", vcmpServerOptionWallGlitch},
             {"backfaceCulling", vcmpServerOptionDisableBackfaceCulling},
             {"heliBladeDamage", vcmpServerOptionDisableHeliBladeDamage},
             {"disableCrouch", vcmpServerOptionDisableCrouch},
         });

    Enum(lua, "DisconnectReason",
         {
             {"timeout", vcmpDisconnectReasonTimeout},
             {"quit", vcmpDisconnectReasonQuit},
             {"kick", vcmpDisconnectReasonKick},
             {"ban", vcmpDisconnectReasonKick},
             {"kickBan", vcmpDisconnectReasonKick},
             {"crash", vcmpDisconnectReasonCrash},
             {"ac", vcmpDisconnectReasonAntiCheat},
         });

    Enum(lua, "BodyPart",
         {
             {"body", vcmpBodyPartBody},
             {"torso", vcmpBodyPartTorso},
             {"leftArm", vcmpBodyPartLeftArm},
             {"rightArm", vcmpBodyPartRightArm},
             {"leftLeg", vcmpBodyPartLeftLeg},
             {"rightLeg", vcmpBodyPartRightLeg},
             {"head", vcmpBodyPartHead},
             {"inVehicle", vcmpBodyPartInVehicle},
         });

    Enum(lua, "PlayerState",
         {
             {"none", vcmpPlayerStateNone},
             {"normal", vcmpPlayerStateNormal},
             {"aim", vcmpPlayerStateAim},
             {"driver", vcmpPlayerStateDriver},
             {"passenger", vcmpPlayerStatePassenger},
             {"enterDriver", vcmpPlayerStateEnterDriver},
             {"enterPassenger", vcmpPlayerStateEnterPassenger},
             {"exit", vcmpPlayerStateExit},
             {"unspawned", vcmpPlayerStateUnspawned},
         });

    Enum(lua, "PlayerUpdate",
         {
             {"normal", vcmpPlayerUpdateNormal},
             {"aiming", vcmpPlayerUpdateAimingDeprecated},
             {"driver", vcmpPlayerUpdateDriver},
             {"passenger", vcmpPlayerUpdatePassenger},
         });

    Enum(lua, "PlayerOption",
         {
             {"controllable", vcmpPlayerOptionControllable},
             {"driveBy", vcmpPlayerOptionDriveBy},
             {"whiteScanLines", vcmpPlayerOptionWhiteScanlines},
             {"greenScanLines", vcmpPlayerOptionGreenScanlines},
             {"widescreen", vcmpPlayerOptionWidescreen},
             {"showMarkers", vcmpPlayerOptionShowMarkers},
             {"canAttack", vcmpPlayerOptionCanAttack},
             {"hasMarker", vcmpPlayerOptionHasMarker},
             {"showOnRadar", vcmpPlayerOptionHasMarker},
             {"chatTags", vcmpPlayerOptionChatTagsEnabled},
             {"drunkEffects", vcmpPlayerOptionDrunkEffectsDeprecated},
             {"bleeding", vcmpPlayerOptionBleeding},
         });

    Enum(lua, "PlayerVehicle",
         {
             {"outside", vcmpPlayerVehicleOut},
             {"entering", vcmpPlayerVehicleEntering},
             {"exiting", vcmpPlayerVehicleExiting},
             {"inside", vcmpPlayerVehicleIn},
         });

    Enum(lua, "VehicleUpdate",
         {
             {"driverSync", vcmpVehicleUpdateDriverSync},
             {"otherSync", vcmpVehicleUpdateOtherSync},
             {"position", vcmpVehicleUpdatePosition},
             {"health", vcmpVehicleUpdateHealth},
             {"color", vcmpVehicleUpdateColour},
             {"colour", vcmpVehicleUpdateColour},
             {"rotation", vcmpVehicleUpdateRotation},
         });

    Enum(lua, "VehicleOption",
         {
             {"lockDoors", vcmpVehicleOptionDoorsLocked},
             {"alarm", vcmpVehicleOptionAlarm},
             {"lights", vcmpVehicleOptionLights},
             {"radioLocked", vcmpVehicleOptionRadioLocked},
             {"ghost", vcmpVehicleOptionGhost},
             {"siren", vcmpVehicleOptionSiren},
             {"singleUse", vcmpVehicleOptionSingleUse},
             {"engine", vcmpVehicleOptionEngineDisabled},
             {"boot", vcmpVehicleOptionBootOpen},
             {"bonnet", vcmpVehicleOptionBonnetOpen},
         });

    // Vehicle:getSpeed / setSpeed: which speed.
    Enum(lua, "VehicleSpeed",
         {
             {"normal", 1},
             {"normalRelative", 2},
             {"turn", 3},
             {"turnRelative", 4},
         });

    Enum(lua, "PickupOption",
         {
             {"singleUse", vcmpPickupOptionSingleUse},
             {"forceSize", forceSizeVcmpPickupOption},
         });

    Enum(lua, "EntityType",
         {
             {"vehicle", vcmpEntityPoolVehicle},
             {"object", vcmpEntityPoolObject},
             {"pickup", vcmpEntityPoolPickup},
             {"radio", vcmpEntityPoolRadio},
             {"player", vcmpEntityPoolPlayer},
             {"blip", vcmpEntityPoolBlip},
             {"checkpoint", vcmpEntityPoolCheckPoint},
         });

    // For Player:getNetworkStatistics(option); needs plugin API 2.1.
    Enum(lua, "NetworkStatistics",
         {
             {"dataSentPerSecond", vcmpNetworkStatisticsOptionDataSentPerSecond},
             {"dataResentPerSecond", vcmpNetworkStatisticsOptionDataResentPerSecond},
             {"dataReceivedPerSecond", vcmpNetworkStatisticsOptionDataReceivedPerSecond},
             {"dataDiscardedPerSecond", vcmpNetworkStatisticsOptionDataDiscardedPerSecond},
             {"allBytesSentPerSecond", vcmpNetworkStatisticsOptionAllBytesSentPerSecond},
             {"allBytesReceivedPerSecond", vcmpNetworkStatisticsOptionAllBytesReceivedPerSecond},
             {"dataSentTotal", vcmpNetworkStatisticsOptionDataSentTotal},
             {"dataResentTotal", vcmpNetworkStatisticsOptionDataResentTotal},
             {"dataReceivedTotal", vcmpNetworkStatisticsOptionDataReceivedTotal},
             {"dataDiscardedTotal", vcmpNetworkStatisticsOptionDataDiscardedTotal},
             {"allBytesSentTotal", vcmpNetworkStatisticsOptionAllBytesSentTotal},
             {"allBytesReceivedTotal", vcmpNetworkStatisticsOptionAllBytesReceivedTotal},
             {"messagesWaiting", vcmpNetworkStatisticsOptionMessagesWaiting},
             {"messagesResending", vcmpNetworkStatisticsOptionMessagesResending},
             {"bytesResending", vcmpNetworkStatisticsOptionBytesResending},
             {"packetLossPerSecond", vcmpNetworkStatisticsOptionPacketLossPerSecond},
             {"packetLossTotal", vcmpNetworkStatisticsOptionPacketLossTotal},
         });
}

}  // namespace vcmp_lua::bindings
