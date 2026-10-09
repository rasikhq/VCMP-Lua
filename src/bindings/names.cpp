// Names and ids of skins and weapons: v1's Server.getSkinID/getSkinName and
// Weapon.getID/getName, ported unchanged except for memory handling (v1
// returned NULL as a std::string for an unknown skin id, which crashed).
// The name heuristics come from SqMod via v1.
#include "bindings/names.hpp"

#include <array>
#include <cctype>
#include <string>
#include <utility>

namespace vcmp_lua::bindings {
namespace {

// The SKIN_ID_* constants of v1's Constants.h.
enum SkinIdValue : int {
    SKIN_ID_UNKNOWN = -1,
    SKIN_ID_TOMMY_VERCETTI = 0,
    SKIN_ID_COP = 1,
    SKIN_ID_SWAT = 2,
    SKIN_ID_FBI = 3,
    SKIN_ID_ARMY = 4,
    SKIN_ID_PARAMEDIC = 5,
    SKIN_ID_FIREMAN = 6,
    SKIN_ID_GOLF_GUY_A = 7,
    SKIN_ID_BUM_LADY_A = 9,
    SKIN_ID_BUM_LADY_B = 10,
    SKIN_ID_PUNK_A = 11,
    SKIN_ID_LAWYER = 12,
    SKIN_ID_SPANISH_LADY_A = 13,
    SKIN_ID_SPANISH_LADY_B = 14,
    SKIN_ID_COOL_GUY_A = 15,
    SKIN_ID_ARABIC_GUY = 16,
    SKIN_ID_BEACH_LADY_A = 17,
    SKIN_ID_BEACH_LADY_B = 18,
    SKIN_ID_BEACH_GUY_A = 19,
    SKIN_ID_BEACH_GUY_B = 20,
    SKIN_ID_OFFICE_LADY_A = 21,
    SKIN_ID_WAITRESS_A = 22,
    SKIN_ID_FOOD_LADY = 23,
    SKIN_ID_PROSTITUTE_A = 24,
    SKIN_ID_BUM_LADY_C = 25,
    SKIN_ID_BUM_GUY_A = 26,
    SKIN_ID_GARBAGEMAN_A = 27,
    SKIN_ID_TAXI_DRIVER_A = 28,
    SKIN_ID_HATIAN_A = 29,
    SKIN_ID_CRIMINAL_A = 30,
    SKIN_ID_HOOD_LADY = 31,
    SKIN_ID_GRANNY_A = 32,
    SKIN_ID_BUSINESS_MAN_A = 33,
    SKIN_ID_CHURCH_GUY = 34,
    SKIN_ID_CLUB_LADY = 35,
    SKIN_ID_CHURCH_LADY = 36,
    SKIN_ID_PIMP = 37,
    SKIN_ID_BEACH_LADY_C = 38,
    SKIN_ID_BEACH_GUY_C = 39,
    SKIN_ID_BEACH_LADY_D = 40,
    SKIN_ID_BEACH_GUY_D = 41,
    SKIN_ID_BUSINESS_MAN_B = 42,
    SKIN_ID_PROSTITUTE_B = 43,
    SKIN_ID_BUM_LADY_D = 44,
    SKIN_ID_BUM_GUY_B = 45,
    SKIN_ID_HATIAN_B = 46,
    SKIN_ID_CONSTRUCTION_WORKER_A = 47,
    SKIN_ID_PUNK_B = 48,
    SKIN_ID_PROSTITUTE_C = 49,
    SKIN_ID_GRANNY_B = 50,
    SKIN_ID_PUNK_C = 51,
    SKIN_ID_BUSINESS_MAN_C = 52,
    SKIN_ID_SPANISH_LADY_C = 53,
    SKIN_ID_SPANISH_LADY_D = 54,
    SKIN_ID_COOL_GUY_B = 55,
    SKIN_ID_BUSINESS_MAN_D = 56,
    SKIN_ID_BEACH_LADY_E = 57,
    SKIN_ID_BEACH_GUY_E = 58,
    SKIN_ID_BEACH_LADY_F = 59,
    SKIN_ID_BEACH_GUY_F = 60,
    SKIN_ID_CONSTRUCTION_WORKER_B = 61,
    SKIN_ID_GOLF_GUY_B = 62,
    SKIN_ID_GOLF_LADY = 63,
    SKIN_ID_GOLF_GUY_C = 64,
    SKIN_ID_BEACH_LADY_G = 65,
    SKIN_ID_BEACH_GUY_G = 66,
    SKIN_ID_OFFICE_LADY_B = 67,
    SKIN_ID_BUSINESS_MAN_E = 68,
    SKIN_ID_BUSINESS_MAN_F = 69,
    SKIN_ID_PROSTITUTE_D = 70,
    SKIN_ID_BUM_LADY_E = 71,
    SKIN_ID_BUM_GUY_C = 72,
    SKIN_ID_SPANISH_GUY = 73,
    SKIN_ID_TAXI_DRIVER_B = 74,
    SKIN_ID_GYM_LADY = 75,
    SKIN_ID_GYM_GUY = 76,
    SKIN_ID_SKATE_LADY = 77,
    SKIN_ID_SKATE_GUY = 78,
    SKIN_ID_SHOPPER_A = 79,
    SKIN_ID_SHOPPER_B = 80,
    SKIN_ID_TOURIST_A = 81,
    SKIN_ID_TOURIST_B = 82,
    SKIN_ID_CUBAN_A = 83,
    SKIN_ID_CUBAN_B = 84,
    SKIN_ID_HATIAN_C = 85,
    SKIN_ID_HATIAN_D = 86,
    SKIN_ID_SHARK_A = 87,
    SKIN_ID_SHARK_B = 88,
    SKIN_ID_DIAZ_GUY_A = 89,
    SKIN_ID_DIAZ_GUY_B = 90,
    SKIN_ID_DBP_SECURITY_A = 91,
    SKIN_ID_DBP_SECURITY_B = 92,
    SKIN_ID_BIKER_A = 93,
    SKIN_ID_BIKER_B = 94,
    SKIN_ID_VERCETTI_GUY_A = 95,
    SKIN_ID_VERCETTI_GUY_B = 96,
    SKIN_ID_UNDERCOVER_COP_A = 97,
    SKIN_ID_UNDERCOVER_COP_B = 98,
    SKIN_ID_UNDERCOVER_COP_C = 99,
    SKIN_ID_UNDERCOVER_COP_D = 100,
    SKIN_ID_UNDERCOVER_COP_E = 101,
    SKIN_ID_UNDERCOVER_COP_F = 102,
    SKIN_ID_RICH_GUY = 103,
    SKIN_ID_COOL_GUY_C = 104,
    SKIN_ID_PROSTITUTE_E = 105,
    SKIN_ID_PROSTITUTE_F = 106,
    SKIN_ID_LOVE_FIST_A = 107,
    SKIN_ID_KEN_ROSENBURG = 108,
    SKIN_ID_CANDY_SUXX = 109,
    SKIN_ID_HILARY = 110,
    SKIN_ID_LOVE_FIST_B = 111,
    SKIN_ID_PHIL = 112,
    SKIN_ID_ROCKSTAR_GUY = 113,
    SKIN_ID_SONNY = 114,
    SKIN_ID_LANCE_A = 115,
    SKIN_ID_MERCADES_A = 116,
    SKIN_ID_LOVE_FIST_C = 117,
    SKIN_ID_ALEX_SRUB = 118,
    SKIN_ID_LANCE_COP = 119,
    SKIN_ID_LANCE_B = 120,
    SKIN_ID_CORTEZ = 121,
    SKIN_ID_LOVE_FIST_D = 122,
    SKIN_ID_COLUMBIAN_GUY_A = 123,
    SKIN_ID_HILARY_ROBBER = 124,
    SKIN_ID_MERCADES_B = 125,
    SKIN_ID_CAM = 126,
    SKIN_ID_CAM_ROBBER = 127,
    SKIN_ID_PHIL_ONE_ARM = 128,
    SKIN_ID_PHIL_ROBBER = 129,
    SKIN_ID_COOL_GUY_D = 130,
    SKIN_ID_PIZZAMAN = 131,
    SKIN_ID_TAXI_DRIVER_C = 132,
    SKIN_ID_TAXI_DRIVER_D = 133,
    SKIN_ID_SAILOR_A = 134,
    SKIN_ID_SAILOR_B = 135,
    SKIN_ID_SAILOR_C = 136,
    SKIN_ID_CHEF = 137,
    SKIN_ID_CRIMINAL_B = 138,
    SKIN_ID_FRENCH_GUY = 139,
    SKIN_ID_GARBAGEMAN_B = 140,
    SKIN_ID_HATIAN_E = 141,
    SKIN_ID_WAITRESS_B = 142,
    SKIN_ID_SONNY_GUY_A = 143,
    SKIN_ID_SONNY_GUY_B = 144,
    SKIN_ID_SONNY_GUY_C = 145,
    SKIN_ID_COLUMBIAN_GUY_B = 146,
    SKIN_ID_THUG_A = 147,
    SKIN_ID_BEACH_GUY_H = 148,
    SKIN_ID_GARBAGEMAN_C = 149,
    SKIN_ID_GARBAGEMAN_D = 150,
    SKIN_ID_GARBAGEMAN_E = 151,
    SKIN_ID_TRANNY = 152,
    SKIN_ID_THUG_B = 153,
    SKIN_ID_SPANDEX_GUY_A = 154,
    SKIN_ID_SPANDEX_GUY_B = 155,
    SKIN_ID_STRIPPER_A = 156,
    SKIN_ID_STRIPPER_B = 157,
    SKIN_ID_STRIPPER_C = 158,
    SKIN_ID_STORE_CLERK = 159,
};

constexpr std::pair<int, const char*> kSkinNames[] = {
    {0, "Tommy Vercetti"},
    {1, "Cop"},
    {2, "SWAT"},
    {3, "FBI"},
    {4, "Army"},
    {5, "Paramedic"},
    {6, "Firefighter"},
    {7, "Golf Guy #1"},
    {9, "Bum Lady #1"},
    {10, "Bum Lady #2"},
    {11, "Punk #1"},
    {12, "Lawyer"},
    {13, "Spanish Lady #1"},
    {14, "Spanish Lady #2"},
    {15, "Cool Guy #1"},
    {16, "Arabic Guy"},
    {17, "Beach Lady #1"},
    {18, "Beach Lady #2"},
    {19, "Beach Guy #1"},
    {20, "Beach Guy #2"},
    {21, "Office Lady #1"},
    {22, "Waitress #1"},
    {23, "Food Lady"},
    {24, "Prostitute #1"},
    {25, "Bum Lady #3"},
    {26, "Bum Guy #1"},
    {27, "Garbageman #1"},
    {28, "Taxi Driver #1"},
    {29, "Haitian #1"},
    {30, "Criminal #1"},
    {31, "Hood Lady"},
    {32, "Granny #1"},
    {33, "Businessman #1"},
    {34, "Church Guy"},
    {35, "Club Lady"},
    {36, "Church Lady"},
    {37, "Pimp"},
    {38, "Beach Lady #3"},
    {39, "Beach Guy #3"},
    {40, "Beach Lady #4"},
    {41, "Beach Guy #4"},
    {42, "Businessman #2"},
    {43, "Prostitute #2"},
    {44, "Bum Lady #4"},
    {45, "Bum Guy #2"},
    {46, "Haitian #2"},
    {47, "Construction Worker #1"},
    {48, "Punk #2"},
    {49, "Prostitute #3"},
    {50, "Granny #2"},
    {51, "Punk #3"},
    {52, "Businessman #3"},
    {53, "Spanish Lady #3"},
    {54, "Spanish Lady #4"},
    {55, "Cool Guy #2"},
    {56, "Businessman #4"},
    {57, "Beach Lady #5"},
    {58, "Beach Guy #5"},
    {59, "Beach Lady #6"},
    {60, "Beach Guy #6"},
    {61, "Construction Worker #2"},
    {62, "Golf Guy #2"},
    {63, "Golf Lady"},
    {64, "Golf Guy #3"},
    {65, "Beach Lady #7"},
    {66, "Beach Guy #7"},
    {67, "Office Lady #2"},
    {68, "Businessman #5"},
    {69, "Businessman #6"},
    {70, "Prostitute #2"},
    {71, "Bum Lady #4"},
    {72, "Bum Guy #3"},
    {73, "Spanish Guy"},
    {74, "Taxi Driver #2"},
    {75, "Gym Lady"},
    {76, "Gym Guy"},
    {77, "Skate Lady"},
    {78, "Skate Guy"},
    {79, "Shopper #1"},
    {80, "Shopper #2"},
    {81, "Tourist #1"},
    {82, "Tourist #2"},
    {83, "Cuban #1"},
    {84, "Cuban #2"},
    {85, "Haitian #3"},
    {86, "Haitian #4"},
    {87, "Shark #1"},
    {88, "Shark #2"},
    {89, "Diaz Guy #1"},
    {90, "Diaz Guy #2"},
    {91, "DBP Security #1"},
    {92, "DBP Security #2"},
    {93, "Biker #1"},
    {94, "Biker #2"},
    {95, "Vercetti Guy #1"},
    {96, "Vercetti Guy #2"},
    {97, "Undercover Cop #1"},
    {98, "Undercover Cop #2"},
    {99, "Undercover Cop #3"},
    {100, "Undercover Cop #4"},
    {101, "Undercover Cop #5"},
    {102, "Undercover Cop #6"},
    {103, "Rich Guy"},
    {104, "Cool Guy #3"},
    {105, "Prostitute #3"},
    {106, "Prostitute #4"},
    {107, "Love Fist #1"},
    {108, "Ken Rosenburg"},
    {109, "Candy Suxx"},
    {110, "Hilary"},
    {111, "Love Fist #2"},
    {112, "Phil"},
    {113, "Rockstar Guy"},
    {114, "Sonny"},
    {115, "Lance"},
    {116, "Mercedes"},
    {117, "Love Fist #3"},
    {118, "Alex Shrub"},
    {119, "Lance (Cop)"},
    {120, "Lance"},
    {121, "Cortez"},
    {122, "Love Fist #4"},
    {123, "Columbian Guy #1"},
    {124, "Hilary (Robber)"},
    {125, "Mercedes"},
    {126, "Cam"},
    {127, "Cam (Robber)"},
    {128, "Phil (One Arm)"},
    {129, "Phil (Robber)"},
    {130, "Cool Guy #4"},
    {131, "Pizza Man"},
    {132, "Taxi Driver #1"},
    {133, "Taxi Driver #2"},
    {134, "Sailor #1"},
    {135, "Sailor #2"},
    {136, "Sailor #3"},
    {137, "Chef"},
    {138, "Criminal #2"},
    {139, "French Guy"},
    {140, "Garbageman #2"},
    {141, "Haitian #5"},
    {142, "Waitress #2"},
    {143, "Sonny Guy #1"},
    {144, "Sonny Guy #2"},
    {145, "Sonny Guy #3"},
    {146, "Columbian Guy #2"},
    {147, "Haitian #6"},
    {148, "Beach Guy #8"},
    {149, "Garbageman #3"},
    {150, "Garbageman #4"},
    {151, "Garbageman #5"},
    {152, "Tranny"},
    {153, "Thug #5"},
    {154, "SpandEx Guy #1"},
    {155, "SpandEx Guy #2"},
    {156, "Stripper #1"},
    {157, "Stripper #2"},
    {158, "Stripper #3"},
    {159, "Store Clerk"},
    {161, "Tommy with Suit"},
    {162, "Worker Tommy"},
    {163, "Golfer Tommy"},
    {164, "Cuban Tommy"},
    {165, "VCPD Tommy"},
    {166, "Bank Robber Tommy"},
    {167, "Street Tommy"},
    {168, "Mafia Tommy"},
    {169, "Jogger Tommy #1"},
    {170, "Jogger Tommy #2"},
    {171, "Guy With Suit #1"},
    {172, "Guy With Suit #3"},
    {173, "Prostitute #5"},
    {174, "Rico"},
    {175, "Prostitute #3"},
    {176, "Club Lady"},
    {177, "Prostitute #2"},
    {178, "Skull T-Shirt Guy"},
    {179, "Easter Egg Tommy"},
    {180, "Diaz Gangster #1"},
    {181, "Diaz Gangster #2"},
    {182, "Hood Lady"},
    {183, "Punk #1"},
    {184, "Tray Lady"},
    {185, "Kent Paul"},
    {186, "Taxi Driver #1"},
    {187, "Deformed Ken Rosenberg"},
    {188, "Deformed Woman"},
    {189, "Deformed Man"},
    {190, "Deformed Cortez"},
    {191, "Deformed Lance Vance"},
    {192, "Thief #1"},
    {193, "Thief #2"},
    {194, "Thief #3"},
};

constexpr std::pair<int, const char*> kWeaponNames[] = {
    {0, "Unarmed"},
    {1, "Brass Knuckles"},
    {2, "Screwdriver"},
    {3, "Golf Club"},
    {4, "Nightstick"},
    {5, "Knife"},
    {6, "Baseball Bat"},
    {7, "Hammer"},
    {8, "Meat Cleaver"},
    {9, "Machete"},
    {10, "Katana"},
    {11, "Chainsaw"},
    {12, "Grenade"},
    {13, "Remote Detonation Grenade"},
    {14, "Tear Gas"},
    {15, "Molotov Cocktails"},
    {16, "Rocket"},
    {17, "Colt .45"},
    {18, "Python"},
    {19, "Pump-Action Shotgun"},
    {20, "SPAS-12 Shotgun"},
    {21, "Stubby Shotgun"},
    {22, "TEC-9"},
    {23, "Uzi"},
    {24, "Silenced Ingram"},
    {25, "MP5"},
    {26, "M4"},
    {27, "Ruger"},
    {28, "Sniper Rifle"},
    {29, "Laserscope Sniper Rifle"},
    {30, "Rocket Launcher"},
    {31, "Flamethrower"},
    {32, "M60"},
    {33, "Minigun"},
    {34, "Explosion"},
    {35, "Helicannon"},
    {36, "Camera"},
    {39, "Vehicle"},
    {41, "Explosion"},
    {42, "Driveby"},
    {43, "Drowned"},
    {44, "Fall"},
    {51, "Explosion"},
    {70, "Suicide"},
};

char Lower(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

}  // namespace

int SkinId(std::string_view name) {
    // Lowercase, alphanumeric characters only.
    std::string str;
    for (const char ch : name) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            str += Lower(ch);
        }
    }
    if (str.empty()) {
        return SKIN_ID_UNKNOWN;
    }
    int id = SKIN_ID_UNKNOWN;
    const int len = static_cast<int>(str.size());

    // The most significant characters (of the name as written, as in v1).
    const char a = Lower(name[0]);
    const char b = name.size() >= 2 ? Lower(name[1]) : 0;
    const char c = name.size() >= 3 ? Lower(name[2]) : 0;
    const char d = str[len - 1];

    // Search for a pattern in the name
    switch (a) {
            // [A]lex Srub, [A]rabic guy, [A]rmy
        case 'a':
            switch (b) {
                    // [Al]ex [S]rub
                case 'l':
                case 's':
                    id = SKIN_ID_ALEX_SRUB;
                    break;
                    // [A]rabic [g]uy
                case 'g':
                    id = SKIN_ID_ARABIC_GUY;
                    break;
                    // [Ara]bic guy, [Arm]y
                case 'r':
                    if (c && c == 'a')
                        id = SKIN_ID_ARABIC_GUY;
                    else if (c && c == 'm')
                        id = SKIN_ID_ARMY;
                    break;
            }
            break;
            // [B]each guy (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)/(#7|G)/(#8|H)
            // [B]each lady (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)/(#7|G)
            // [B]iker (#1|A)/(#2|B)
            // [B]um guy (#1|A)/(#2|B)/(#3|C)
            // [B]um lady (#1|A)/(#2|B)/(#2|C)/(#3|D)/(#4|E)
            // [B]usiness man (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)
        case 'b':
            // [Be]ach [g]uy (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)/(#7|G)/(#8|H)
            if (b && b == 'e' && ((c && c == 'g') || (len >= 6 && str[5] == 'g'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_BEACH_GUY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_BEACH_GUY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_BEACH_GUY_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_BEACH_GUY_D;
                        break;
                    case '5':
                    case 'e':
                        id = SKIN_ID_BEACH_GUY_E;
                        break;
                    case '6':
                    case 'f':
                        id = SKIN_ID_BEACH_GUY_F;
                        break;
                    case '7':
                    case 'g':
                        id = SKIN_ID_BEACH_GUY_G;
                        break;
                    case '8':
                    case 'h':
                        id = SKIN_ID_BEACH_GUY_H;
                        break;
                }
            }
            // [Be]ach [l]ady (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)/(#7|G)
            else if (b && b == 'e' && ((c && c == 'l') || (len >= 6 && str[5] == 'l'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_BEACH_LADY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_BEACH_LADY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_BEACH_LADY_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_BEACH_LADY_D;
                        break;
                    case '5':
                    case 'e':
                        id = SKIN_ID_BEACH_LADY_E;
                        break;
                    case '6':
                    case 'f':
                        id = SKIN_ID_BEACH_LADY_F;
                        break;
                    case '7':
                    case 'g':
                        id = SKIN_ID_BEACH_LADY_G;
                        break;
                }
            }
            // [Bi]ker (#1|A)/(#2|B)
            else if (b && b == 'i' && (d == '1' || d == 'a'))
                id = SKIN_ID_BIKER_A;
            else if (b && b == 'i' && (d == '2' || d == 'b'))
                id = SKIN_ID_BIKER_B;
            // [Bum] [g]uy (#1|A)/(#2|B)/(#3|C)
            // [Bum] [l]ady (#1|A)/(#2|B)/(#2|C)/(#3|D)/(#4|E)
            else if (b && b == 'u' && (c && (c == 'm' || c == 'g' || c == 'l'))) {
                // [Bum] [g]uy (#1|A)/(#2|B)/(#3|C)
                if (c == 'g' || (len >= 4 && str[3] == 'g')) {
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_BUM_GUY_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_BUM_GUY_B;
                    else if (d == '3' || d == 'c')
                        id = SKIN_ID_BUM_GUY_C;
                }
                // [Bum] [l]ady (#1|A)/(#2|B)/(#2|C)/(#3|D)/(#4|E)
                else if (c == 'l' || (len >= 4 && str[3] == 'l')) {
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_BUM_LADY_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_BUM_LADY_B;
                    else if (d == '2' || d == 'c')
                        id = SKIN_ID_BUM_LADY_C;
                    else if (d == '3' || d == 'd')
                        id = SKIN_ID_BUM_LADY_D;
                    else if (d == '4' || d == 'e')
                        id = SKIN_ID_BUM_LADY_E;
                }
            }
            // [Bus]iness [m]an (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)
            else if (b && b == 'u' && ((c && c == 's') || (len >= 10 && str[9] == 'm'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_BUSINESS_MAN_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_BUSINESS_MAN_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_BUSINESS_MAN_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_BUSINESS_MAN_D;
                        break;
                    case '5':
                    case 'e':
                        id = SKIN_ID_BUSINESS_MAN_E;
                        break;
                    case '6':
                    case 'f':
                        id = SKIN_ID_BUSINESS_MAN_F;
                        break;
                }
            }
            break;
            // [C]am, [C]am (Robber), [C]andy Suxx, [C]hef
            // [C]hurch guy, [C]hurch lady, [C]lub lady
            // [C]olumbian guy (#1|A)/(#2|B),
            // [C]onstruction worker (#1|A)/(#2|B)
            // [C]ool guy (#1|A)/(#2|B)/(#3|C)/(#4|D)
            // [C]op, [C]ortez
            // [C]riminal (#1|A)/(#2|B)
            // [C]uban (#1|A)/(#2|B)
        case 'c':
            // [Ca]m, [Ca]m (Robber), [Ca]ndy Suxx
            if (b && b == 'a') {
                // [Cam] ([R]obbe[r])
                if (c && (c == 'm' || c == 'r') && d == 'r') id = SKIN_ID_CAM_ROBBER;
                // [Cam]
                else if (c && c == 'm')
                    id = SKIN_ID_CAM;
                // [Can]dy [S]ux[x]
                else if (c && (c == 'n' || c == 's' || d == 'x'))
                    id = SKIN_ID_CANDY_SUXX;
            }
            // [Ch]ef, [Ch]urch guy, [Ch]urch lady
            else if (b && b == 'h') {
                // [Che][f]
                if (c && (c == 'e' || d == 'f')) id = SKIN_ID_CHEF;
                // [Chu]rch [g]uy
                else if (c && ((c == 'u' && len >= 7 && str[6] == 'g') || (c == 'g')))
                    id = SKIN_ID_CHURCH_GUY;
                // [Chu]rch [l]ady
                else if (c && ((c == 'u' && len >= 7 && str[6] == 'l') || (c == 'l')))
                    id = SKIN_ID_CHURCH_LADY;
            }
            // [Cl]ub [l]ady
            else if (b && b == 'l')
                id = SKIN_ID_CLUB_LADY;
            // [Co]lumbian guy (#1|A)/(#2|B)
            // [Co]nstruction worker (#1|A)/(#2|B)
            // [Co]ol guy (#1|A)/(#2|B)/(#3|C)/(#4|D)
            // [Co]p, [Co]rtez
            else if (b && b == 'o') {
                // [Col]umbian [g]uy (#1|A)/(#2|B)
                if (c && ((c == 'l' && len >= 10 && str[9] == 'g') || (c == 'g'))) {
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_COLUMBIAN_GUY_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_COLUMBIAN_GUY_B;
                }
                // [Con]struction [w]orker (#1|A)/(#2|B)
                else if (c && (c == 'n' || (len >= 13 && str[12] == 'g'))) {
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_CONSTRUCTION_WORKER_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_CONSTRUCTION_WORKER_B;
                }
                // [Coo]l guy (#1|A)/(#2|B)/(#3|C)/(#4|D)
                else if (c && c == 'o') {
                    switch (d) {
                        case '1':
                        case 'a':
                            id = SKIN_ID_COOL_GUY_A;
                            break;
                        case '2':
                        case 'b':
                            id = SKIN_ID_COOL_GUY_B;
                            break;
                        case '3':
                        case 'c':
                            id = SKIN_ID_COOL_GUY_C;
                            break;
                        case '4':
                        case 'd':
                            id = SKIN_ID_COOL_GUY_D;
                            break;
                    }
                }
                // [Cop]
                else if (c && c == 'p')
                    id = SKIN_ID_COP;
                // [Cor]te[z]
                else if (c && (c == 'r' || c == 'z' || d == 'z'))
                    id = SKIN_ID_CORTEZ;
            }
            // [Cr]iminal (#1|A)/(#2|B)
            else if (b && b == 'r' && (d == '1' || d == 'a'))
                id = SKIN_ID_CRIMINAL_A;
            else if (b && b == 'r' && (d == '2' || d == 'b'))
                id = SKIN_ID_CRIMINAL_B;
            // [Cu]ban (#1|A)/(#2|B)
            else if (b && b == 'u' && (d == '1' || d == 'a'))
                id = SKIN_ID_CUBAN_A;
            else if (b && b == 'u' && (d == '2' || d == 'b'))
                id = SKIN_ID_CUBAN_B;
            break;
            // [D]BP security (#1|A)/(#2|B)
            // [D]iaz guy (#1|A)/(#2|B)
        case 'd':
            switch (b) {
                    // [DB]P [s]ecurity (#1|A)/(#2|B)
                case 'b':
                case 's':
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_DBP_SECURITY_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_DBP_SECURITY_B;
                    break;
                    // [Di]a[z] [g]uy (#1|A)/(#2|B)
                case 'i':
                case 'z':
                case 'g':
                    if (d == '1' || d == 'a')
                        id = SKIN_ID_DIAZ_GUY_A;
                    else if (d == '2' || d == 'b')
                        id = SKIN_ID_DIAZ_GUY_B;
                    break;
            }
            break;
            // [F]BI, [F]ireman, [F]ood lady, [F]rench guy
        case 'f':
            switch (b) {
                    // [FB]I
                case 'b':
                    id = SKIN_ID_FBI;
                    break;
                    // [Fi]re[m]an
                case 'i':
                case 'm':
                    id = SKIN_ID_FIREMAN;
                    break;
                    // [Fo]od [l]ady
                case 'o':
                case 'l':
                    id = SKIN_ID_FOOD_LADY;
                    break;
                    // [Fr]ench [g]uy
                case 'r':
                case 'g':
                    id = SKIN_ID_FRENCH_GUY;
                    break;
            }
            break;
            // [G]arbageman (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)
            // [G]olf guy (#1|A)/(#2|B)/(#3|C)
            // [G]olf lady
            // [G]ranny (#1|A)/(#2|B)
            // [G]ym guy, [G]ym lady
        case 'g':
            // [Ga]rbage[m]an (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)
            if (b && (b == 'a' || b == 'm')) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_GARBAGEMAN_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_GARBAGEMAN_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_GARBAGEMAN_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_GARBAGEMAN_D;
                        break;
                    case '5':
                    case 'e':
                        id = SKIN_ID_GARBAGEMAN_E;
                        break;
                }
            }
            // [Go]lf [g]uy (#1|A)/(#2|B)/(#3|C)
            else if (b && b == 'o' && ((c && c == 'g') || (len >= 5 && str[4] == 'g'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_GOLF_GUY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_GOLF_GUY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_GOLF_GUY_C;
                        break;
                }
            }
            // [Go]lf [l]ady
            else if (b && b == 'o' && ((c && c == 'l') || (len >= 5 && str[4] == 'l')))
                id = SKIN_ID_GOLF_LADY;
            // [Gr]anny (#1|A)/(#2|B)
            else if (b && b == 'r') {
                if (d == '1' || d == 'a')
                    id = SKIN_ID_GRANNY_A;
                else if (d == '2' || d == 'b')
                    id = SKIN_ID_GRANNY_B;
            }
            // [Gy]m [g]uy
            else if (b && (b == 'g' || (b == 'y' && len >= 4 && str[3] == 'g')))
                id = SKIN_ID_GYM_GUY;
            // [Gy]m [l]ady
            else if (b && (b == 'l' || (b == 'y' && len >= 4 && str[3] == 'l')))
                id = SKIN_ID_GYM_LADY;
            break;
            // [H]atian (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)
            // [H]ilary, [H]ilary (Robber), [H]ood lady
        case 'h':
            // [H]atian (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)
            if (b && b == 'a') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_HATIAN_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_HATIAN_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_HATIAN_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_HATIAN_D;
                        break;
                    case '5':
                    case 'e':
                        id = SKIN_ID_HATIAN_E;
                        break;
                }
            }
            // [Hi]lary ([R]obbe[r])
            else if (b && (b == 'i' || b == 'r') && d == 'r')
                id = SKIN_ID_HILARY_ROBBER;
            // [Hi]lary
            else if (b && b == 'i')
                id = SKIN_ID_HILARY;
            // [Ho]od [l]ady
            if (b && (b == 'o' || b == 'l')) id = SKIN_ID_HOOD_LADY;
            break;
            // [K]en Rosenburg
        case 'k':
            id = SKIN_ID_KEN_ROSENBURG;
            break;
            // [L]ance (#1|A)/(#1|B)
            // [L]ance (Cop)
            // [L]awyer
            // [L]ove Fist (#1|A)/(#2|B)/(#3|C)/(#3|D)
        case 'l':
            //[Lan]ce ([C]o[p])
            if ((b && b == 'a') && (c && c == 'n') && ((len >= 6 && str[5] == 'c') || d == 'p'))
                id = SKIN_ID_LANCE_COP;
            else if (b && (b == 'c' || (b == 'a' && (c && c == 'n'))))
                id = SKIN_ID_LANCE_COP;
            // [La]nce (#1|A)/(#1|B)
            else if (b && b == 'a' && c && c == 'n') {
                if (d == '1' || d == 'a')
                    id = SKIN_ID_LANCE_A;
                else if (d == '2' || d == 'b')
                    id = SKIN_ID_LANCE_B;
            }
            // [Law]yer
            else if (b && (b == 'w' || (b == 'a' && c && c == 'w')))
                id = SKIN_ID_LAWYER;
            // [Lo]ve [F]ist (#1|A)/(#2|B)/(#3|C)/(#3|D)
            else if (b && (b == 'o' || b == 'f')) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_LOVE_FIST_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_LOVE_FIST_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_LOVE_FIST_C;
                        break;
                    case 'd':
                        id = SKIN_ID_LOVE_FIST_D;
                        break;
                }
            }
            break;
            // [M]ercades
        case 'm':
            if (d == 'b')
                id = SKIN_ID_MERCADES_B;
            else
                id = SKIN_ID_MERCADES_A;
            break;
            // [O]ffice lady (#1|A)/(#2|B)
        case 'o':
            if (d == '1' || d == 'a')
                id = SKIN_ID_OFFICE_LADY_A;
            else if (d == '2' || d == 'b')
                id = SKIN_ID_OFFICE_LADY_B;
            break;
            // [P]aramedic, [P]hil,  [P]hil (One arm), [P]hil (Robber)
            // [P]imp, [P]izzaman
            // [P]rostitute (#1|A)/(#2|B)/(#2|C)/(#2|D)/(#3|D)/(#4|D)
            // [P]unk (#1|A)/(#2|B)/(#3|C)
        case 'p':
            // [Pa]ramedic
            if (b && b == 'a') id = SKIN_ID_PARAMEDIC;
            // [Ph]il (One arm), [Ph]il (Robber)
            else if (b && b == 'h') {
                // [Ph]il ([O]ne ar[m])
                if (b == 'o' || (c && c == 'o') || (len >= 5 && str[4] == 'o') || d == 'm')
                    id = SKIN_ID_PHIL_ONE_ARM;
                // [Ph]il ([R]obbe[r])
                else if (c && (c == 'r' || d == 'r' || (len >= 5 && str[4] == 'r')))
                    id = SKIN_ID_PHIL_ROBBER;
                // [Phi]l
                else if (c && c == 'i')
                    id = SKIN_ID_PHIL;
            }
            // [Pim][p]
            else if (b && b == 'i' && ((c && c == 'm') || d == 'p'))
                id = SKIN_ID_PIMP;
            // [Piz]zama[n]
            else if (b && b == 'i' && ((c && c == 'z') || d == 'n'))
                id = SKIN_ID_PIZZAMAN;
            // [Pr]ostitute (#1|A)/(#2|B)/(#2|C)/(#2|D)/(#3|D)/(#4|D)
            else if (b && b == 'r') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_PROSTITUTE_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_PROSTITUTE_B;
                        break;
                    case 'c':
                        id = SKIN_ID_PROSTITUTE_C;
                        break;
                    case 'd':
                        id = SKIN_ID_PROSTITUTE_D;
                        break;
                    case '3':
                    case 'e':
                        id = SKIN_ID_PROSTITUTE_E;
                        break;
                    case '4':
                    case 'f':
                        id = SKIN_ID_PROSTITUTE_F;
                        break;
                }
            }
            // [Pu]nk (#1|A)/(#2|B)/(#3|C)
            else if (b && b == 'u') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_PUNK_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_PUNK_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_PUNK_C;
                        break;
                }
            }
            break;
            // [R]ich guy, [R]ockstar guy
        case 'r':
            // [Ri]ch guy
            if (b && b == 'i') id = SKIN_ID_RICH_GUY;
            // [Ro]ckstar guy
            else if (b && b == 'o')
                id = SKIN_ID_ROCKSTAR_GUY;
            break;
            // [S]ailor (#1|A)/(#2|B)/(#3|C)
            // [S]hark (#1|A)/(#2|B)
            // [S]hopper (#1|A)/(#2|B)
            // [S]kate guy, [S]kate lady, [S]onny
            // [S]onny guy (#1|A)/(#2|B)/(#3|C)
            // [S]pandEx (#1|A)/(#2|B)
            // [S]panish guy
            // [S]panish lady (#1|A)/(#2|B)/(#3|C)/(#4|D)
            // [S]tore clerk
            // [S]tripper (#1|A)/(#2|B)/(#3|C)
            // [S]wat
        case 's':
            // [Sa]ilor (#1|A)/(#2|B)/(#3|C)
            if (b && b == 'a') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SAILOR_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SAILOR_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_SAILOR_C;
                        break;
                }
            }
            // [S]hark (#1|A)/(#2|B)
            else if (b && b == 'h' && (c && c == 'a')) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SHARK_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SHARK_B;
                        break;
                }
            }
            // [S]hopper (#1|A)/(#2|B)
            else if (b && b == 'h' && (c && c == 'o')) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SHOPPER_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SHOPPER_B;
                        break;
                }
            }
            // [Sk]ate [g]uy
            else if (b && b == 'k' && ((c && c == 'g') || (len >= 6 && str[5] == 'g')))
                id = SKIN_ID_SKATE_GUY;
            // [Sk]ate [l]ady
            else if (b && b == 'k' && ((c && c == 'l') || (len >= 6 && str[5] == 'l')))
                id = SKIN_ID_SKATE_LADY;
            // [So]nny
            // [So]nny guy (#1|A)/(#2|B)/(#3|C)
            else if (b && b == 'o') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SONNY_GUY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SONNY_GUY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_SONNY_GUY_C;
                        break;
                }
            } else if (b && b == 'g') {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SONNY_GUY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SONNY_GUY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_SONNY_GUY_C;
                        break;
                }
            }
            // [Sp]andE[x] (#1|A)/(#2|B)
            else if (b && b == 'p' && ((c && c == 'x') || (len >= 7 && str[6] == 'x'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SPANDEX_GUY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SPANDEX_GUY_B;
                        break;
                }
            }
            // [Sp]anish [g]uy
            else if (b && b == 'p' && ((c && c == 'g') || (len >= 8 && str[7] == 'g')))
                id = SKIN_ID_SPANISH_GUY;
            // [Sp]anish [l]ady (#1|A)/(#2|B)/(#3|C)/(#4|D)
            else if (b && b == 'p' && ((c && c == 'l') || (len >= 8 && str[7] == 'l'))) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_SPANISH_LADY_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_SPANISH_LADY_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_SPANISH_LADY_C;
                        break;
                    case '4':
                    case 'd':
                        id = SKIN_ID_SPANISH_LADY_D;
                        break;
                }
            }
            // [Sto]re clerk
            else if ((b && b == 't') && (c && c == 'o'))
                id = SKIN_ID_STORE_CLERK;
            // [Str]ipper (#1|A)/(#2|B)/(#3|C)
            else if ((b && b == 't') && (c && c == 'r')) {
                switch (d) {
                    case '1':
                    case 'a':
                        id = SKIN_ID_STRIPPER_A;
                        break;
                    case '2':
                    case 'b':
                        id = SKIN_ID_STRIPPER_B;
                        break;
                    case '3':
                    case 'c':
                        id = SKIN_ID_STRIPPER_C;
                        break;
                }
            }
            // [Sw]at
            else if (b && b == 'w')
                id = SKIN_ID_SWAT;
            break;
            // [T]axi driver (#1|A)/(#1|B)/(#2|C)/(#2|D)
            // [T]hug (#1|A)/(#2|B)
            // [T]ommy Vercetti
            // [T]ourist (#1|A)/(#2|B)
            // [T]ranny
        case 't':
            switch (b) {
                    // [Ta]xi driver (#1|A)/(#1|B)/(#2|C)/(#2|D)
                case 'a':
                    switch (d) {
                        case '1':
                        case 'a':
                            id = SKIN_ID_TAXI_DRIVER_A;
                            break;
                        case '2':
                        case 'b':
                            id = SKIN_ID_TAXI_DRIVER_B;
                            break;
                        case 'c':
                            id = SKIN_ID_TAXI_DRIVER_C;
                            break;
                        case 'd':
                            id = SKIN_ID_TAXI_DRIVER_D;
                            break;
                    }
                    break;
                    // [Th]ug (#1|A)/(#2|B)
                case 'h':
                    switch (d) {
                        case '1':
                        case 'a':
                            id = SKIN_ID_THUG_A;
                            break;
                        case '5':
                        case 'b':
                            id = SKIN_ID_THUG_B;
                            break;
                    }
                    break;
                    // [To]mmy [V]ercetti
                    // [To]urist (#1|A)/(#2|B)
                case 'v':
                    id = SKIN_ID_TOMMY_VERCETTI;
                    break;
                case 'o':
                    if (c && c == 'm')
                        id = SKIN_ID_TOMMY_VERCETTI;
                    else if (c && c == 'u' && (d == '1' || d == 'a'))
                        id = SKIN_ID_TOURIST_A;
                    else if (c && c == 'u' && (d == '2' || d == 'b'))
                        id = SKIN_ID_TOURIST_B;
                    break;
                case 'r':
                    id = SKIN_ID_TRANNY;
                    break;
            }
            break;
            // [U]ndercover cop (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)
        case 'u':
            switch (d) {
                case '1':
                case 'a':
                    id = SKIN_ID_UNDERCOVER_COP_A;
                    break;
                case '2':
                case 'b':
                    id = SKIN_ID_UNDERCOVER_COP_B;
                    break;
                case '3':
                case 'c':
                    id = SKIN_ID_UNDERCOVER_COP_C;
                    break;
                case '4':
                case 'd':
                    id = SKIN_ID_UNDERCOVER_COP_D;
                    break;
                case '5':
                case 'e':
                    id = SKIN_ID_UNDERCOVER_COP_E;
                    break;
                case '6':
                case 'f':
                    id = SKIN_ID_UNDERCOVER_COP_F;
                    break;
            }
            break;
            // [V]ercetti guy (#1|A)/(#2|B)
        case 'v':
            switch (d) {
                case '1':
                case 'a':
                    id = SKIN_ID_VERCETTI_GUY_A;
                    break;
                case '2':
                case 'b':
                    id = SKIN_ID_VERCETTI_GUY_B;
                    break;
            }
            break;
            // [W]aitress (#1|A)/(#2|B)
        case 'w':
            switch (d) {
                case '1':
                case 'a':
                    id = SKIN_ID_WAITRESS_A;
                    break;
                case '2':
                case 'b':
                    id = SKIN_ID_WAITRESS_B;
                    break;
            }
            break;
    }
    return id;
}

int WeaponId(std::string_view name) {
    if (name.empty()) {
        return 0;
    }
    const char char1 = Lower(name[0]);
    const char char2 = name.size() >= 2 ? Lower(name[1]) : 0;
    const char char3 = name.size() >= 3 ? Lower(name[2]) : 0;

    switch (char1) {
            // [F]ists, [F]lamethrower
        case 'f': {
            // [Fi]sts
            if (char2 && char2 == 'i') return 0;

            // Default to flamethrower
            else
                return 31;

            break;
        }

        case 'b':
            // [Br]ass Knuckles
            if (char2 && char2 == 'r') return 1;

            // [Ba]seball Bat
            return 6;

            // [S]crewdriver, [S]hotgun, [S]PAS-12 Shotgun, [S]tubby/[S]awnoff Shotgun, [Si]lenced
            // Ingram [S]niper Rifle
        case 's': {
            switch (char2) {
                    // [Sc]rewdriver
                case 'c':
                    return 2;

                    // [Sh]otgun
                case 'h':
                    return 19;

                    // [SP]AS-12 / [Sp]az Shotgun
                case 'p':
                    return 20;

                    // [St]ubby / [Sa]wnoff Shotgun
                case 't':
                case 'a':
                    return 21;

                    // [Si]lenced Ingram
                case 'i':
                    return 24;

                    // [Sn]iper
                case 'n':
                    return 28;

                    // Default to screwdriver
                default:
                    return 2;
            }
        }

        // [G]olf Club, [G]renade
        case 'g': {
            // [Go]lf Club
            if (char2 && char2 == 'o') return 3;

            // Grenades being more popular in servers, default to grenade
            else
                return 12;

            break;
        }

        // [N]ightstick
        case 'n':
            return 4;

            // [K]nife, [K]atana
        case 'k': {
            if (char2 && char2 == 'n') {
                // [Kn]ife
                if (char3 == 'i') return 5;
                // [Knu]ckles
                else if (char3 == 'u')
                    return 1;
            }

            // Default to katana
            else
                return 10;

            break;
        }

        // [H]ammer
        case 'h':
            return 7;

            // [M]eat Cleaver, [M]achete, [M]olotov Cocktail, [M]P5, [M]4, [M]60, [M]inigun
        case 'm': {
            switch (char2) {
                    // [Me]at Cleaver
                case 'e':
                    return 8;

                    // [Ma]chete
                case 'a':
                    return 9;

                    // [Mo]lotov Cocktail
                case 'o':
                    return 15;

                    // [MP]5
                case 'p':
                    return 25;

                    // [M4]
                case '4':
                    return 26;

                    // [M6]0
                case '6':
                    return 32;

                    // [Mi]nigun
                case 'i':
                    return 33;

                    // Default to M4
                default:
                    return 26;
            }

            break;
        }

        // [C]leaver, [C]hainsaw, [C]olt .45
        case 'c': {
            switch (char2) {
                    // [Cl]eaver
                case 'l':
                    return 8;

                    // [Ch]ainsaw
                case 'h':
                    return 11;

                    // Default to Colt .45
                default:
                    return 17;
            }

            break;
        }

        // [R]emote Detonation Grenade, [R]uger, [R]ocket Launcher / [R]PG
        case 'r': {
            switch (char2) {
                    // [Re]mote Detonation Grenade
                case 'e':
                    return 13;

                    // [Ro]cket Launcher, [RP]G
                case 'o':
                case 'p':
                    return 30;

                    // [Ru]ger
                case 'u':
                    return 27;

                    // Default to ruger
                default:
                    return 27;
            }
        }

        // [T]ear Gas, [T]EC-9
        case 't': {
            // Both of them have E as a second character anyways.
            if (char2) {
                // [Tea]r Gas
                if (char3 && char3 == 'a') return 14;

                // Default to TEC-9
                else
                    return 22;
            }

            // Default to TEC-9 if no second character exists.
            else
                return 22;

            break;
        }

        // [P]ython
        case 'p':
            return 18;

            // [U]zi
        case 'u':
            return 23;

            // [I]ngram
        case 'i':
            return 24;

            // [L]aserscope Sniper
        case 'l':
            return 29;

            // Default to fists
        default:
            return 255;
    }

    return 255;
    return 255;
}

const char* SkinName(int id) {
    for (const auto& [skin, text] : kSkinNames) {
        if (skin == id) {
            return text;
        }
    }
    return nullptr;
}

const char* WeaponName(int id) {
    for (const auto& [weapon, text] : kWeaponNames) {
        if (weapon == id) {
            return text;
        }
    }
    return "Unknown";
}

}  // namespace vcmp_lua::bindings
