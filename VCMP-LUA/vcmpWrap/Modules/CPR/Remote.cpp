#include "Remote.h"

extern sol::state Lua;

sol::table Remote::fetchHTTP(const std::string& url, sol::table params, sol::table auth)
{
	cpr::Parameters cprParams;
	std::string authUser(""), authPassword("");

	for (auto [key, value] : params)
	{
		cpr::Parameter cprParameter = cpr::Parameter(key.as<std::string>(), value.as<std::string>());
		cprParams.Add(cprParameter);
	}

	if (auth.size() == 2)
	{
		if (auth["user"].valid())
			authUser = auth["user"].get<std::string>();

		if (auth["password"].valid())
			authPassword = auth["password"].get<std::string>();
	}

	cpr::Authentication cprAuth(authUser, authPassword);

	cpr::Response r = cpr::Get(
		cpr::Url(url),
		cprParams,
		cprAuth
	);

	sol::table resultTable = Lua.create_table();
	resultTable["status_code"] = r.status_code;
	resultTable["error"] = r.error.code;
	resultTable["error_message"] = r.error.message;
	resultTable["header"] = sol::as_table(r.header);
	resultTable["text"] = r.text;
	return resultTable;
}

void Remote::Init(sol::state* Lua)
{
	sol::usertype<Remote> userdata = Lua->new_usertype<Remote>("Remote");

	userdata["fetch"] = &Remote::fetchHTTP;
}