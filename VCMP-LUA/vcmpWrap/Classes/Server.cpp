#include "Server.h"
#include "Player.h"
#include "Constants.h"
#include "utility.h"

extern PluginFuncs* g_Funcs;
extern sol::state Lua;

static ServerSettings s_Settings;

bool Server::getOption(vcmpServerOption option) {
	return g_Funcs->GetServerOption(option);
}

bool Server::setOption(vcmpServerOption option, bool toggle) {
	g_Funcs->SetServerOption(option, toggle);
	return true;
}

sol::object Server::getSettings() {
	if (g_Funcs->GetServerSettings(&s_Settings) != vcmpErrorNone) {
		spdlog::error("[ERROR] Failed to retrieve server settings from plugin!");
		return sol::nil;
	}
	sol::table settings = Lua.create_table();

	char PasswordBuffer[128];
	g_Funcs->GetServerPassword(PasswordBuffer, sizeof(PasswordBuffer));

	settings["maxPlayers"] = s_Settings.maxPlayers;
	settings["port"] = s_Settings.port;
	settings["serverName"] = std::string(s_Settings.serverName);
	settings["serverPassword"] = std::string(PasswordBuffer);
	settings["flags"] = s_Settings.flags;

	return settings;
}

std::string Server::getName()
{
	char buffer[512];
	g_Funcs->GetServerName(buffer, sizeof(buffer));
	return std::string(buffer);
}

void Server::setName(const std::string& newName)
{
	g_Funcs->SetServerName(newName.c_str());
}

std::string Server::getPassword()
{
	char buffer[512];
	g_Funcs->GetServerPassword(buffer, sizeof(buffer));
	return std::string(buffer);
}

void Server::setPassword(const std::string& newPassword)
{
	g_Funcs->SetServerPassword(newPassword.c_str());
}

uint16_t Server::getFallTimer()
{
	return g_Funcs->GetFallTimer();
}

int32_t Server::getTimeRate()
{
	return g_Funcs->GetTimeRate();
}

int32_t Server::getHour()
{
	return g_Funcs->GetHour();
}

int32_t Server::getMinute()
{
	return g_Funcs->GetMinute();
}

int32_t Server::getWeather()
{
	return g_Funcs->GetWeather();
}

float Server::getGravity()
{
	return g_Funcs->GetGravity();
}

float Server::getWaterLevel()
{
	return g_Funcs->GetWaterLevel();
}

float Server::getGameSpeed()
{
	return g_Funcs->GetGameSpeed();
}

float Server::getFlightAltitude()
{
	return g_Funcs->GetMaximumFlightAltitude();
}

float Server::getVehicleRespawnHeight()
{
	return g_Funcs->GetVehiclesForcedRespawnHeight();
}

void Server::setFallTimer(uint16_t value)
{
	g_Funcs->SetFallTimer(value);
}

void Server::setTimeRate(int32_t value)
{
	g_Funcs->SetTimeRate(value);
}

void Server::setHour(int32_t value)
{
	g_Funcs->SetHour(value);
}

void Server::setMinute(int32_t value)
{
	g_Funcs->SetMinute(value);
}

void Server::setWeather(int32_t value)
{
	g_Funcs->SetWeather(value);
}

void Server::setGravity(float value)
{
	g_Funcs->SetGravity(value);
}

void Server::setWaterLevel(float value)
{
	g_Funcs->SetWaterLevel(value);
}

void Server::setGameSpeed(float value)
{
	g_Funcs->SetGameSpeed(value);
}

void Server::setFlightAltitude(float value)
{
	g_Funcs->SetMaximumFlightAltitude(value);
}

void Server::setVehicleRespawnHeight(float value)
{
	g_Funcs->SetVehiclesForcedRespawnHeight(value);
}

bool Server::banIP(char* ip)
{
	if (isIPBanned(ip))
		return false;
	g_Funcs->BanIP(ip);
	return true;
}

bool Server::unbanIP(char* ip)
{
	if (!isIPBanned(ip))
		return false;
	g_Funcs->UnbanIP(ip);
	return true;
}

bool Server::isIPBanned(char* ip)
{
	if (g_Funcs->IsIPBanned(ip))
		return true;
	return false;
}

int Server::GetSkinID(const char* name) {
	// Verify that the name is valid and could actually represent a skin name
	if (name == NULL || strlen(name) < 1) return SKIN_ID_UNKNOWN;

	// Create a temporary variable that will hold the actual ID
	int id = SKIN_ID_UNKNOWN;

	// Allocate memory for an editable version of the string
	char* str = (char*)malloc(strlen(name) + 1);

	// Verify that the memory could be allocated and prepare the string
	if (str != NULL)
	{
		int i, j = 0;
		// Convert the string to lowercase and remove non alpha-numeric characters
		for (i = 0; name[i]; i++)
		{
			if (isalnum(name[i]) != 0)
			{
				str[j] = tolower(name[i]);
				j++;
			}
		}
		// Add the null character to the final string
		str[j] = '\0';
		// Make sure the string had any valid characters
		if (strlen(str) < 1) {
			// Free the previously allocated resources
			free(str);
			// Return the found ID
			return id;
		}
	}
	// No point in going forward
	else return id;

	// Grab the actual length of the string
	int len = (int) strlen(str);

	// Get the most significant characters used to identify a skin
	char a, b, c, d;
	a = tolower(name[0]);
	b = 0;
	c = 0;
	d = tolower(str[len - 1]);

	// Look for deeper specifiers
	if (strlen(name) >= 3)
	{
		b = tolower(name[1]);
		c = tolower(name[2]);
	}
	else if (strlen(name) >= 2)
	{
		b = tolower(name[1]);
	}

	// Search for a pattern in the name
	switch (a)
	{
		// [A]lex Srub, [A]rabic guy, [A]rmy
	case 'a':
		switch (b)
		{
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
			if (c && c == 'a') id = SKIN_ID_ARABIC_GUY;
			else if (c && c == 'm') id = SKIN_ID_ARMY;
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
		if (b && b == 'e' && ((c && c == 'g') || (len >= 6 && str[5] == 'g')))
		{
			switch (d)
			{
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
		else if (b && b == 'e' && ((c && c == 'l') || (len >= 6 && str[5] == 'l')))
		{
			switch (d)
			{
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
		else if (b && b == 'i' && (d == '1' || d == 'a')) id = SKIN_ID_BIKER_A;
		else if (b && b == 'i' && (d == '2' || d == 'b')) id = SKIN_ID_BIKER_B;
		// [Bum] [g]uy (#1|A)/(#2|B)/(#3|C)
		// [Bum] [l]ady (#1|A)/(#2|B)/(#2|C)/(#3|D)/(#4|E)
		else if (b && b == 'u' && (c && (c == 'm' || c == 'g' || c == 'l')))
		{
			// [Bum] [g]uy (#1|A)/(#2|B)/(#3|C)
			if (c == 'g' || (len >= 4 && str[3] == 'g'))
			{
				if (d == '1' || d == 'a') id = SKIN_ID_BUM_GUY_A;
				else if (d == '2' || d == 'b') id = SKIN_ID_BUM_GUY_B;
				else if (d == '3' || d == 'c') id = SKIN_ID_BUM_GUY_C;
			}
			// [Bum] [l]ady (#1|A)/(#2|B)/(#2|C)/(#3|D)/(#4|E)
			else if (c == 'l' || (len >= 4 && str[3] == 'l'))
			{
				if (d == '1' || d == 'a') id = SKIN_ID_BUM_LADY_A;
				else if (d == '2' || d == 'b') id = SKIN_ID_BUM_LADY_B;
				else if (d == '2' || d == 'c') id = SKIN_ID_BUM_LADY_C;
				else if (d == '3' || d == 'd') id = SKIN_ID_BUM_LADY_D;
				else if (d == '4' || d == 'e') id = SKIN_ID_BUM_LADY_E;
			}
		}
		// [Bus]iness [m]an (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)
		else if (b && b == 'u' && ((c && c == 's') || (len >= 10 && str[9] == 'm')))
		{
			switch (d)
			{
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
		if (b && b == 'a')
		{
			// [Cam] ([R]obbe[r])
			if (c && (c == 'm' || c == 'r') && d == 'r') id = SKIN_ID_CAM_ROBBER;
			// [Cam]
			else if (c && c == 'm') id = SKIN_ID_CAM;
			// [Can]dy [S]ux[x]
			else if (c && (c == 'n' || c == 's' || d == 'x')) id = SKIN_ID_CANDY_SUXX;
		}
		// [Ch]ef, [Ch]urch guy, [Ch]urch lady
		else if (b && b == 'h')
		{
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
		else if (b && b == 'l') id = SKIN_ID_CLUB_LADY;
		// [Co]lumbian guy (#1|A)/(#2|B)
		// [Co]nstruction worker (#1|A)/(#2|B)
		// [Co]ol guy (#1|A)/(#2|B)/(#3|C)/(#4|D)
		// [Co]p, [Co]rtez
		else if (b && b == 'o')
		{
			// [Col]umbian [g]uy (#1|A)/(#2|B)
			if (c && ((c == 'l' && len >= 10 && str[9] == 'g') || (c == 'g')))
			{
				if (d == '1' || d == 'a') id = SKIN_ID_COLUMBIAN_GUY_A;
				else if (d == '2' || d == 'b') id = SKIN_ID_COLUMBIAN_GUY_B;
			}
			// [Con]struction [w]orker (#1|A)/(#2|B)
			else if (c && (c == 'n' || (len >= 13 && str[12] == 'g')))
			{
				if (d == '1' || d == 'a') id = SKIN_ID_CONSTRUCTION_WORKER_A;
				else if (d == '2' || d == 'b') id = SKIN_ID_CONSTRUCTION_WORKER_B;
			}
			// [Coo]l guy (#1|A)/(#2|B)/(#3|C)/(#4|D)
			else if (c && c == 'o')
			{
				switch (d)
				{
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
			else if (c && c == 'p') id = SKIN_ID_COP;
			// [Cor]te[z]
			else if (c && (c == 'r' || c == 'z' || d == 'z')) id = SKIN_ID_CORTEZ;
		}
		// [Cr]iminal (#1|A)/(#2|B)
		else if (b && b == 'r' && (d == '1' || d == 'a')) id = SKIN_ID_CRIMINAL_A;
		else if (b && b == 'r' && (d == '2' || d == 'b')) id = SKIN_ID_CRIMINAL_B;
		// [Cu]ban (#1|A)/(#2|B)
		else if (b && b == 'u' && (d == '1' || d == 'a')) id = SKIN_ID_CUBAN_A;
		else if (b && b == 'u' && (d == '2' || d == 'b')) id = SKIN_ID_CUBAN_B;
		break;
		// [D]BP security (#1|A)/(#2|B)
		// [D]iaz guy (#1|A)/(#2|B)
	case 'd':
		switch (b)
		{
			// [DB]P [s]ecurity (#1|A)/(#2|B)
		case 'b':
		case 's':
			if (d == '1' || d == 'a') id = SKIN_ID_DBP_SECURITY_A;
			else if (d == '2' || d == 'b') id = SKIN_ID_DBP_SECURITY_B;
			break;
			// [Di]a[z] [g]uy (#1|A)/(#2|B)
		case 'i':
		case 'z':
		case 'g':
			if (d == '1' || d == 'a') id = SKIN_ID_DIAZ_GUY_A;
			else if (d == '2' || d == 'b') id = SKIN_ID_DIAZ_GUY_B;
			break;
		}
		break;
		// [F]BI, [F]ireman, [F]ood lady, [F]rench guy
	case 'f':
		switch (b)
		{
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
		if (b && (b == 'a' || b == 'm'))
		{
			switch (d)
			{
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
		else if (b && b == 'o' && ((c && c == 'g') || (len >= 5 && str[4] == 'g')))
		{
			switch (d)
			{
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
		else if (b && b == 'r')
		{
			if (d == '1' || d == 'a') id = SKIN_ID_GRANNY_A;
			else if (d == '2' || d == 'b') id = SKIN_ID_GRANNY_B;
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
		if (b && b == 'a')
		{
			switch (d)
			{
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
		else if (b && (b == 'i' || b == 'r') && d == 'r') id = SKIN_ID_HILARY_ROBBER;
		// [Hi]lary
		else if (b && b == 'i') id = SKIN_ID_HILARY;
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
		else if (b && b == 'a' && c && c == 'n')
		{
			if (d == '1' || d == 'a') id = SKIN_ID_LANCE_A;
			else if (d == '2' || d == 'b') id = SKIN_ID_LANCE_B;
		}
		// [Law]yer
		else if (b && (b == 'w' || (b == 'a' && c && c == 'w'))) id = SKIN_ID_LAWYER;
		// [Lo]ve [F]ist (#1|A)/(#2|B)/(#3|C)/(#3|D)
		else if (b && (b == 'o' || b == 'f'))
		{
			switch (d)
			{
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
		if (d == 'b') id = SKIN_ID_MERCADES_B;
		else id = SKIN_ID_MERCADES_A;
		break;
		// [O]ffice lady (#1|A)/(#2|B)
	case 'o':
		if (d == '1' || d == 'a') id = SKIN_ID_OFFICE_LADY_A;
		else if (d == '2' || d == 'b') id = SKIN_ID_OFFICE_LADY_B;
		break;
		// [P]aramedic, [P]hil,  [P]hil (One arm), [P]hil (Robber)
		// [P]imp, [P]izzaman
		// [P]rostitute (#1|A)/(#2|B)/(#2|C)/(#2|D)/(#3|D)/(#4|D)
		// [P]unk (#1|A)/(#2|B)/(#3|C)
	case 'p':
		// [Pa]ramedic
		if (b && b == 'a') id = SKIN_ID_PARAMEDIC;
		// [Ph]il (One arm), [Ph]il (Robber)
		else if (b && b == 'h')
		{
			// [Ph]il ([O]ne ar[m])
			if (b == 'o' || (c && c == 'o') || (len >= 5 && str[4] == 'o') || d == 'm')
				id = SKIN_ID_PHIL_ONE_ARM;
			// [Ph]il ([R]obbe[r])
			else if (c && (c == 'r' || d == 'r' || (len >= 5 && str[4] == 'r')))
				id = SKIN_ID_PHIL_ROBBER;
			// [Phi]l
			else if (c && c == 'i') id = SKIN_ID_PHIL;
		}
		// [Pim][p]
		else if (b && b == 'i' && ((c && c == 'm') || d == 'p')) id = SKIN_ID_PIMP;
		// [Piz]zama[n]
		else if (b && b == 'i' && ((c && c == 'z') || d == 'n')) id = SKIN_ID_PIZZAMAN;
		// [Pr]ostitute (#1|A)/(#2|B)/(#2|C)/(#2|D)/(#3|D)/(#4|D)
		else if (b && b == 'r')
		{
			switch (d)
			{
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
		else if (b && b == 'u')
		{
			switch (d)
			{
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
		else if (b && b == 'o') id = SKIN_ID_ROCKSTAR_GUY;
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
		if (b && b == 'a')
		{
			switch (d)
			{
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
		else if (b && b == 'h' && (c && c == 'a'))
		{
			switch (d)
			{
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
		else if (b && b == 'h' && (c && c == 'o'))
		{
			switch (d)
			{
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
		else if (b && b == 'o')
		{
			switch (d)
			{
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
		else if (b && b == 'g')
		{
			switch (d)
			{
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
		else if (b && b == 'p' && ((c && c == 'x') || (len >= 7 && str[6] == 'x')))
		{
			switch (d)
			{
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
		else if (b && b == 'p' && ((c && c == 'l') || (len >= 8 && str[7] == 'l')))
		{
			switch (d)
			{
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
		else if ((b && b == 't') && (c && c == 'o')) id = SKIN_ID_STORE_CLERK;
		// [Str]ipper (#1|A)/(#2|B)/(#3|C)
		else if ((b && b == 't') && (c && c == 'r'))
		{
			switch (d)
			{
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
		else if (b && b == 'w') id = SKIN_ID_SWAT;
		break;
		// [T]axi driver (#1|A)/(#1|B)/(#2|C)/(#2|D)
		// [T]hug (#1|A)/(#2|B)
		// [T]ommy Vercetti
		// [T]ourist (#1|A)/(#2|B)
		// [T]ranny
	case 't':
		switch (b)
		{
			// [Ta]xi driver (#1|A)/(#1|B)/(#2|C)/(#2|D)
		case 'a':
			switch (d)
			{
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
			switch (d)
			{
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
			if (c && c == 'm') id = SKIN_ID_TOMMY_VERCETTI;
			else if (c && c == 'u' && (d == '1' || d == 'a')) id = SKIN_ID_TOURIST_A;
			else if (c && c == 'u' && (d == '2' || d == 'b')) id = SKIN_ID_TOURIST_B;
			break;
		case 'r':
			id = SKIN_ID_TRANNY;
			break;
		}
		break;
		// [U]ndercover cop (#1|A)/(#2|B)/(#3|C)/(#4|D)/(#5|E)/(#6|F)
	case 'u':
		switch (d)
		{
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
		switch (d)
		{
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
		switch (d)
		{
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
	// Free the previously allocated resources
	free(str);
	// Return the found ID
	return id;
}

void Server::Init(sol::state* L) {
	sol::usertype<Server> usertype = L->new_usertype<Server>("Server");
	usertype["getOption"] = &Server::getOption;
	usertype["setOption"] = &Server::setOption;

	usertype["setName"] = &Server::setName;
	usertype["getName"] = &Server::getName;
	usertype["name"] = sol::property(&Server::getName, &Server::setName);

	usertype["setPassword"] = &Server::setPassword;
	usertype["getPassword"] = &Server::getPassword;
	usertype["password"] = sol::property(&Server::getPassword, &Server::setPassword);

	usertype["setClassPosition"] =
		sol::overload(
			[](sol::table position) {
				g_Funcs->SetSpawnPlayerPosition(position[1], position[2], position[3]);
			},
			[](float x, float y, float z) {
				g_Funcs->SetSpawnPlayerPosition(x, y, z);
			}
		);

	usertype["setClassCameraPosition"] =
		sol::overload(
			[](sol::table position) {
				g_Funcs->SetSpawnCameraPosition(position[1], position[2], position[3]);
			},
			[](float x, float y, float z) {
				g_Funcs->SetSpawnCameraPosition(x, y, z);
			}
		);

	usertype["setClassCameraLook"] =
		sol::overload(
			[](sol::table position) {
				g_Funcs->SetSpawnCameraLookAt(position[1], position[2], position[3]);
			},
			[](float x, float y, float z) {
				g_Funcs->SetSpawnCameraLookAt(x, y, z);
			}
		);

	usertype["fallTimer"] = sol::property(&Server::getFallTimer, &Server::setFallTimer);
	usertype["timeRate"] = sol::property(&Server::getTimeRate, &Server::setTimeRate);
	usertype["hour"] = sol::property(&Server::getHour, &Server::setHour);
	usertype["minute"] = sol::property(&Server::getMinute, &Server::setMinute);
	usertype["weather"] = sol::property(&Server::getWeather, &Server::setWeather);
	usertype["gravity"] = sol::property(&Server::getGravity, &Server::setGravity);
	usertype["waterLevel"] = sol::property(&Server::getWaterLevel, &Server::setWaterLevel);
	usertype["gameSpeed"] = sol::property(&Server::getGameSpeed, &Server::setGameSpeed);
	usertype["flightAltitude"] = sol::property(&Server::getFlightAltitude, &Server::setFlightAltitude);
	usertype["vehicleRespawnHeight"] = sol::property(&Server::getVehicleRespawnHeight, &Server::setVehicleRespawnHeight);

	usertype["getSkinID"] = [](std::string name) -> int {
		return GetSkinID(name.c_str());
	};
	usertype["getSkinName"] = [](int skinID) -> std::string {
		switch (skinID)
		{
			case 0:   return "Tommy Vercetti";
			case 1:   return "Cop";
			case 2:   return "SWAT";
			case 3:   return "FBI";
			case 4:   return "Army";
			case 5:   return "Paramedic";
			case 6:   return "Firefighter";
			case 7:   return "Golf Guy #1";
			case 8:   return NULL;
			case 9:   return "Bum Lady #1";
			case 10:  return "Bum Lady #2";
			case 11:  return "Punk #1";
			case 12:  return "Lawyer";
			case 13:  return "Spanish Lady #1";
			case 14:  return "Spanish Lady #2";
			case 15:  return "Cool Guy #1";
			case 16:  return "Arabic Guy";
			case 17:  return "Beach Lady #1";
			case 18:  return "Beach Lady #2";
			case 19:  return "Beach Guy #1";
			case 20:  return "Beach Guy #2";
			case 21:  return "Office Lady #1";
			case 22:  return "Waitress #1";
			case 23:  return "Food Lady";
			case 24:  return "Prostitute #1";
			case 25:  return "Bum Lady #3";
			case 26:  return "Bum Guy #1";
			case 27:  return "Garbageman #1";
			case 28:  return "Taxi Driver #1";
			case 29:  return "Haitian #1";
			case 30:  return "Criminal #1";
			case 31:  return "Hood Lady";
			case 32:  return "Granny #1";
			case 33:  return "Businessman #1";
			case 34:  return "Church Guy";
			case 35:  return "Club Lady";
			case 36:  return "Church Lady";
			case 37:  return "Pimp";
			case 38:  return "Beach Lady #3";
			case 39:  return "Beach Guy #3";
			case 40:  return "Beach Lady #4";
			case 41:  return "Beach Guy #4";
			case 42:  return "Businessman #2";
			case 43:  return "Prostitute #2";
			case 44:  return "Bum Lady #4";
			case 45:  return "Bum Guy #2";
			case 46:  return "Haitian #2";
			case 47:  return "Construction Worker #1";
			case 48:  return "Punk #2";
			case 49:  return "Prostitute #3";
			case 50:  return "Granny #2";
			case 51:  return "Punk #3";
			case 52:  return "Businessman #3";
			case 53:  return "Spanish Lady #3";
			case 54:  return "Spanish Lady #4";
			case 55:  return "Cool Guy #2";
			case 56:  return "Businessman #4";
			case 57:  return "Beach Lady #5";
			case 58:  return "Beach Guy #5";
			case 59:  return "Beach Lady #6";
			case 60:  return "Beach Guy #6";
			case 61:  return "Construction Worker #2";
			case 62:  return "Golf Guy #2";
			case 63:  return "Golf Lady";
			case 64:  return "Golf Guy #3";
			case 65:  return "Beach Lady #7";
			case 66:  return "Beach Guy #7";
			case 67:  return "Office Lady #2";
			case 68:  return "Businessman #5";
			case 69:  return "Businessman #6";
			case 70:  return "Prostitute #2";
			case 71:  return "Bum Lady #4";
			case 72:  return "Bum Guy #3";
			case 73:  return "Spanish Guy";
			case 74:  return "Taxi Driver #2";
			case 75:  return "Gym Lady";
			case 76:  return "Gym Guy";
			case 77:  return "Skate Lady";
			case 78:  return "Skate Guy";
			case 79:  return "Shopper #1";
			case 80:  return "Shopper #2";
			case 81:  return "Tourist #1";
			case 82:  return "Tourist #2";
			case 83:  return "Cuban #1";
			case 84:  return "Cuban #2";
			case 85:  return "Haitian #3";
			case 86:  return "Haitian #4";
			case 87:  return "Shark #1";
			case 88:  return "Shark #2";
			case 89:  return "Diaz Guy #1";
			case 90:  return "Diaz Guy #2";
			case 91:  return "DBP Security #1";
			case 92:  return "DBP Security #2";
			case 93:  return "Biker #1";
			case 94:  return "Biker #2";
			case 95:  return "Vercetti Guy #1";
			case 96:  return "Vercetti Guy #2";
			case 97:  return "Undercover Cop #1";
			case 98:  return "Undercover Cop #2";
			case 99:  return "Undercover Cop #3";
			case 100: return "Undercover Cop #4";
			case 101: return "Undercover Cop #5";
			case 102: return "Undercover Cop #6";
			case 103: return "Rich Guy";
			case 104: return "Cool Guy #3";
			case 105: return "Prostitute #3";
			case 106: return "Prostitute #4";
			case 107: return "Love Fist #1";
			case 108: return "Ken Rosenburg";
			case 109: return "Candy Suxx";
			case 110: return "Hilary";
			case 111: return "Love Fist #2";
			case 112: return "Phil";
			case 113: return "Rockstar Guy";
			case 114: return "Sonny";
			case 115: return "Lance";
			case 116: return "Mercedes";
			case 117: return "Love Fist #3";
			case 118: return "Alex Shrub";
			case 119: return "Lance (Cop)";
			case 120: return "Lance";
			case 121: return "Cortez";
			case 122: return "Love Fist #4";
			case 123: return "Columbian Guy #1";
			case 124: return "Hilary (Robber)";
			case 125: return "Mercedes";
			case 126: return "Cam";
			case 127: return "Cam (Robber)";
			case 128: return "Phil (One Arm)";
			case 129: return "Phil (Robber)";
			case 130: return "Cool Guy #4";
			case 131: return "Pizza Man";
			case 132: return "Taxi Driver #1";
			case 133: return "Taxi Driver #2";
			case 134: return "Sailor #1";
			case 135: return "Sailor #2";
			case 136: return "Sailor #3";
			case 137: return "Chef";
			case 138: return "Criminal #2";
			case 139: return "French Guy";
			case 140: return "Garbageman #2";
			case 141: return "Haitian #5";
			case 142: return "Waitress #2";
			case 143: return "Sonny Guy #1";
			case 144: return "Sonny Guy #2";
			case 145: return "Sonny Guy #3";
			case 146: return "Columbian Guy #2";
			case 147: return "Haitian #6";
			case 148: return "Beach Guy #8";
			case 149: return "Garbageman #3";
			case 150: return "Garbageman #4";
			case 151: return "Garbageman #5";
			case 152: return "Tranny";
			case 153: return "Thug #5";
			case 154: return "SpandEx Guy #1";
			case 155: return "SpandEx Guy #2";
			case 156: return "Stripper #1";
			case 157: return "Stripper #2";
			case 158: return "Stripper #3";
			case 159: return "Store Clerk";
			case 160: return NULL;
			case 161: return "Tommy with Suit";
			case 162: return "Worker Tommy";
			case 163: return "Golfer Tommy";
			case 164: return "Cuban Tommy";
			case 165: return "VCPD Tommy";
			case 166: return "Bank Robber Tommy";
			case 167: return "Street Tommy";
			case 168: return "Mafia Tommy";
			case 169: return "Jogger Tommy #1";
			case 170: return "Jogger Tommy #2";
			case 171: return "Guy With Suit #1";
			case 172: return "Guy With Suit #3";
			case 173: return "Prostitute #5";
			case 174: return "Rico";
			case 175: return "Prostitute #3";
			case 176: return "Club Lady";
			case 177: return "Prostitute #2";
			case 178: return "Skull T-Shirt Guy";
			case 179: return "Easter Egg Tommy";
			case 180: return "Diaz Gangster #1";
			case 181: return "Diaz Gangster #2";
			case 182: return "Hood Lady";
			case 183: return "Punk #1";
			case 184: return "Tray Lady";
			case 185: return "Kent Paul";
			case 186: return "Taxi Driver #1";
			case 187: return "Deformed Ken Rosenberg";
			case 188: return "Deformed Woman";
			case 189: return "Deformed Man";
			case 190: return "Deformed Cortez";
			case 191: return "Deformed Lance Vance";
			case 192: return "Thief #1";
			case 193: return "Thief #2";
			case 194: return "Thief #3";
			default:  return NULL;
		}
	};

	usertype["createExplosion"] = sol::overload(
		[](int32_t world, int32_t type, sol::table position, Player* creator, bool grounded) {
			g_Funcs->CreateExplosion(world, type, position[1], position[2], position[3], creator != nullptr ? creator->getID() : -1, grounded);
		},
		[](int32_t world, int32_t type, float x, float y, float z, Player* creator, bool grounded) {
			g_Funcs->CreateExplosion(world, type, x, y, z, creator != nullptr ? creator->getID() : -1, grounded);
		}
	);

#ifdef _x32
	ServerTable["tickCount"] = []() -> uint32_t {
		return static_cast<uint32_t>(GetCurrentSysTime());
	};
#else
	usertype["tick"] = sol::property([]() -> uint64_t { return GetCurrentSysTime(); });
#endif

	//
	Map::Init(L);
	Radio::Init(L);
	Weapon::Init(L);
	Blip::Init(L);
	Sound::Init(L);
}

/*****************************************************************************************************/
std::string Map::GetDistrictName(float x, float y) {
	if (x > -1613.03f && y > 413.218f && x < -213.73f && y < 1677.32f)
		return "Downtown Vice City";
	else if (x > 163.656f && y > -351.153f && x < 1246.03f && y < 1398.85f)
		return "Vice Point";
	else if (x > -103.97f && y > -930.526f && x < 1246.03f && y < -351.153f)
		return "Washington Beach";
	else if (x > -253.206f && y > -1805.37f && x < 1254.9f && y < -930.526f)
		return "Ocean Beach";
	else if (x > -1888.21f && y > -1779.61f && x < -1208.21f && y < 230.39f)
		return "Escobar International Airport";
	else if (x > -748.206f && y > -818.266f && x < -104.505f && y < -241.467f)
		return "Starfish Island";
	else if (x > -213.73f && y > 797.605f && x < 163.656f && y < 1243.47f)
		return "Prawn Island";
	else if (x > -213.73f && y > -241.429f && x < 163.656f && y < 797.605f)
		return "Leaf Links";
	else if (x > -1396.76f && y > -42.9113f && x < -1208.21f && y < 230.39f)
		return "Junkyard";
	else if (x > -1208.21f && y > -1779.61f && x < -253.206f && y < -898.738f)
		return "Viceport";
	else if (x > -1208.21f && y > -898.738f && x < -748.206f && y < -241.467f)
		return "Little Havana";
	else if (x > -1208.21f && y > -241.467f && x < -578.289f && y < 412.66f)
		return "Little Haiti";
	else
		return "Vice City";
}

void Map::Init(sol::state* L) {
	sol::usertype<Map> usertype = L->new_usertype<Map>("Map");

	usertype["setBounds"] = [](float maxX, float minX, float maxY, float minY) {
		g_Funcs->SetWorldBounds(maxX, minX, maxY, minY);
	};
	usertype["getBounds"] = []() -> sol::table {
		sol::table bounds = Lua.create_table_with();
		float maxX, minX, maxY, minY;
		g_Funcs->GetWorldBounds(&maxX, &minX, &maxY, &minY);
		bounds["max_x"] = maxX;
		bounds["min_x"] = minX;
		bounds["max_y"] = maxY;
		bounds["min_y"] = minY;
		return bounds;
	};

	usertype["hideObject"] = [](int32_t model, int16_t x, int16_t y, int16_t z) -> void {
		int16_t x2 = (int16_t)(floor(x * 10.0f) + 0.5f);
		int16_t y2 = (int16_t)(floor(y * 10.0f) + 0.5f);
		int16_t z2 = (int16_t)(floor(z * 10.0f) + 0.5f);
		g_Funcs->HideMapObject(model, x, y, z);
	};

	usertype["hideObjectRaw"] = [](int32_t model, int16_t x, int16_t y, int16_t z) -> void {
		g_Funcs->HideMapObject(model, x, y, z);
	};

	usertype["showMapObject"] = [](int32_t model, int16_t x, int16_t y, int16_t z) -> void {
		g_Funcs->ShowMapObject(model, x, y, z);
	};

	usertype["showAllObjects"] = []() -> void {
		g_Funcs->ShowAllMapObjects();
	};

	usertype["getDistrictName"] = sol::overload(
		[](sol::table position) -> std::string {
			return GetDistrictName(position[1], position[2]);
		},
		[](float x, float y) -> std::string {
			return GetDistrictName(x, y);
		}
	);
}

/*****************************************************************************************************/

void Radio::Init(sol::state* L) {
	sol::usertype<Radio> usertype = L->new_usertype<Radio>("Radio");

	usertype["createStream"] =
		sol::overload(
			[](int32_t id, std::string name, std::string url, bool isListed) -> int32_t {
				return g_Funcs->AddRadioStream(id, name.c_str(), url.c_str(), isListed);
			},
			[](std::string name, std::string url, bool isListed) -> bool {
				return (g_Funcs->AddRadioStream(-1, name.c_str(), url.c_str(), isListed) == vcmpError::vcmpErrorNone);
			}
		);

	usertype["destroyStream"] = [](int32_t id) {
		g_Funcs->RemoveRadioStream(id);
	};
}

/*****************************************************************************************************/
int Weapon::GetWeaponID(const char* name) {
	if (name == NULL || strlen(name) < 1)
		return 0;

	// Get characters we might need
	char char1, char2, char3;
	char1 = 0;
	char2 = 0;
	char3 = 0;

	char1 = tolower(name[0]);
	if (strlen(name) >= 3)
	{
		char3 = tolower(name[2]);
		char2 = tolower(name[1]);
	}
	else if (strlen(name) >= 2)
		char2 = tolower(name[1]);

	switch (char1)
	{
		// [F]ists, [F]lamethrower
	case 'f':
	{
		// [Fi]sts
		if (char2 && char2 == 'i')
			return 0;

		// Default to flamethrower
		else
			return 31;

		break;
	}

	case 'b':
		// [Br]ass Knuckles
		if (char2 && char2 == 'r')
			return 1;

		// [Ba]seball Bat
		return 6;

		// [S]crewdriver, [S]hotgun, [S]PAS-12 Shotgun, [S]tubby/[S]awnoff Shotgun, [Si]lenced Ingram [S]niper Rifle
	case 's':
	{
		switch (char2)
		{
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
	case 'g':
	{
		// [Go]lf Club
		if (char2 && char2 == 'o')
			return 3;

		// Grenades being more popular in servers, default to grenade
		else
			return 12;

		break;
	}

	// [N]ightstick
	case 'n':
		return 4;

		// [K]nife, [K]atana
	case 'k':
	{
		if (char2 && char2 == 'n')
		{
			// [Kn]ife
			if (char3 == 'i')
				return 5;
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
	case 'm':
	{
		switch (char2)
		{
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
	case 'c':
	{
		switch (char2)
		{
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
	case 'r':
	{
		switch (char2)
		{
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
	case 't':
	{
		// Both of them have E as a second character anyways.
		if (char2)
		{
			// [Tea]r Gas
			if (char3 && char3 == 'a')
				return 14;

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
}

void Weapon::setData(int32_t weapon, int32_t field, double value)
{
	g_Funcs->SetWeaponDataValue(weapon, field, value);
}

double Weapon::getData(int32_t weapon, int32_t field)
{
	return g_Funcs->GetWeaponDataValue(weapon, field);
}

void Weapon::Init(sol::state* L) {
	sol::usertype<Weapon> usertype = L->new_usertype<Weapon>("Weapon");

	usertype["data"] = sol::overload(
		[](int32_t weapon, int32_t field) -> double {
			return getData(weapon, field);
		},
		[](int32_t weapon, int32_t field, double value) -> void {
			setData(weapon, field, value);
		}
	);

	usertype["isFieldModified"] = [](int32_t weapon, int32_t field) -> bool {
		return g_Funcs->IsWeaponDataValueModified(weapon, field);
	};

	usertype["resetField"] = [](int32_t weapon, int32_t field) -> void {
		g_Funcs->ResetWeaponDataValue(weapon, field);
	};

	usertype["reset"] = [](int32_t weapon) -> void {
		g_Funcs->ResetWeaponData(weapon);
	};

	usertype["resetAll"] = [](int32_t weapon) -> void {
		g_Funcs->ResetAllWeaponData();
	};

	usertype["getName"] = [](int weaponID) {
		switch (weaponID)
		{
		case 0:
			return "Unarmed";
		case 1:
			return "Brass Knuckles";
		case 2:
			return "Screwdriver";
		case 3:
			return "Golf Club";
		case 4:
			return "Nightstick";
		case 5:
			return "Knife";
		case 6:
			return "Baseball Bat";
		case 7:
			return "Hammer";
		case 8:
			return "Meat Cleaver";
		case 9:
			return "Machete";
		case 10:
			return "Katana";
		case 11:
			return "Chainsaw";
		case 12:
			return "Grenade";
		case 13:
			return "Remote Detonation Grenade";
		case 14:
			return "Tear Gas";
		case 15:
			return "Molotov Cocktails";
		case 16:
			return "Rocket";
		case 17:
			return "Colt .45";
		case 18:
			return "Python";
		case 19:
			return "Pump-Action Shotgun";
		case 20:
			return "SPAS-12 Shotgun";
		case 21:
			return "Stubby Shotgun";
		case 22:
			return "TEC-9";
		case 23:
			return "Uzi";
		case 24:
			return "Silenced Ingram";
		case 25:
			return "MP5";
		case 26:
			return "M4";
		case 27:
			return "Ruger";
		case 28:
			return "Sniper Rifle";
		case 29:
			return "Laserscope Sniper Rifle";
		case 30:
			return "Rocket Launcher";
		case 31:
			return "Flamethrower";
		case 32:
			return "M60";
		case 33:
			return "Minigun";
		case 34:
			return "Explosion";
		case 35:
			return "Helicannon";
		case 36:
			return "Camera";
		case 39:
			return "Vehicle";
		case 41:
			return "Explosion";
		case 42:
			return "Driveby";
		case 43:
			return "Drowned";
		case 44:
			return "Fall";
		case 51:
			return "Explosion";
		case 70:
			return "Suicide";
		default:
			return "Unknown";
		}
	};
	usertype["getID"] = [](std::string weaponName) {
		return GetWeaponID(weaponName.c_str());
	};
}

/*****************************************************************************************************/

void Blip::Init(sol::state* L) {
	sol::usertype<Blip> usertype = L->new_usertype<Blip>("Blip");

	usertype["create"] = 
		sol::overload(
			[](int32_t id, int32_t world, sol::table position, int32_t scale, uint32_t color, int32_t sprite) -> int32_t
			{
				return g_Funcs->CreateCoordBlip(id, world, position[1], position[2], position[3], scale, color, sprite);
			},
			[](int32_t world, sol::table position, int32_t scale, uint32_t color, int32_t sprite) -> int32_t
			{
				return g_Funcs->CreateCoordBlip(-1, world, position[1], position[2], position[3], scale, color, sprite);
			},
				[](int32_t id, int32_t world, float x, float y, float z, int32_t scale, uint32_t color, int32_t sprite) -> int32_t
			{
				return g_Funcs->CreateCoordBlip(id, world, x, y, z, scale, color, sprite);
			},
				[](int32_t world, float x, float y, float z, int32_t scale, uint32_t color, int32_t sprite) -> int32_t
			{
				return g_Funcs->CreateCoordBlip(-1, world, x, y, z, scale, color, sprite);
			}
		);

	usertype["destroy"] = [](int32_t id) -> bool {
		return (g_Funcs->DestroyCoordBlip(id) == vcmpError::vcmpErrorNone);
	};

	usertype["getInfo"] = [](int32_t id) -> sol::table {
		int32_t world, scale, sprite;
		float x, y, z;
		uint32_t color;

		sol::table blipInfo = Lua.create_table_with();
		
		if (g_Funcs->GetCoordBlipInfo(id, &world, &x, &y, &z, &scale, &color, &sprite) != vcmpError::vcmpErrorNone)
			return sol::nil;

		blipInfo["world"] = world;
		blipInfo["position"] = { x, y, z };
		blipInfo["scale"] = scale;
		blipInfo["color"] = color;
		blipInfo["sprite"] = sprite;

		return blipInfo;
	};
}

/*****************************************************************************************************/

void Sound::Init(sol::state* L) {
	sol::usertype<Sound> usertype = L->new_usertype<Sound>("Sound");

	usertype["play"] = sol::overload(
		[](int32_t sound) {
			g_Funcs->PlaySound(0, sound, NAN, NAN, NAN);
		},
		[](int32_t sound, sol::table position) {
			g_Funcs->PlaySound(0, sound, position[1], position[2], position[3]);
		},
		[](int32_t sound, float x, float y, float z) {
			g_Funcs->PlaySound(0, sound, x, y, z);
		},

		[](int32_t world, int32_t sound) {
			g_Funcs->PlaySound(world, sound, NAN, NAN, NAN);
		},
		[](int32_t world, int32_t sound, sol::table position) {
			g_Funcs->PlaySound(world, sound, position[1], position[2], position[3]);
		},
		[](int32_t world, int32_t sound, float x, float y, float z) {
			g_Funcs->PlaySound(world, sound, x, y, z);
		}
	);
}