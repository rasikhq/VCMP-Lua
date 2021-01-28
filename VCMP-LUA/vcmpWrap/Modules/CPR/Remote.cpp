#include "Remote.h"

extern sol::state Lua;

std::vector<RemoteRequest> Remote::s_Container = {};
std::string Remote::certificates = "";

bool Remote::setSSLCerts(const std::string& bundle)
{
	certificates.assign(bundle);
	return true;
}

/*** Lua ***/
sol::table Remote::fetchHTTP(const std::string& url, sol::table params, sol::table auth)
{
	cpr::Parameters cprParams = GetParameters(params);
	cpr::Authentication cprAuth = GetAuthentication(auth);

	cpr::ssl::CaInfo Certs(certificates.c_str());
	cpr::SslOptions opts;
	opts.SetOption(Certs);

	cpr::Response r = cpr::Get(
		cpr::Url(url),
		cprParams,
		cprAuth,
		opts
	);

	sol::table resultTable = Lua.create_table();
	resultTable["status_code"] = r.status_code;
	resultTable["error"] = r.error.code;
	resultTable["error_message"] = r.error.message;
	resultTable["header"] = sol::as_table(r.header);
	resultTable["text"] = r.text;
	return resultTable;
}

void Remote::fetchHTTPAsync(sol::function handler, const std::string& url, sol::table params, sol::table auth)
{
	cpr::ssl::CaInfo Certs(certificates.c_str());
	cpr::SslOptions opts;
	opts.SetOption(Certs);

	cpr::Parameters cprParams = GetParameters(params);
	cpr::Authentication cprAuth = GetAuthentication(auth);
	s_Container.emplace_back(handler, RemoteRequestType::GET, url, cprParams, cpr::Header{}, cpr::Body(""), cpr::Payload{}, cprAuth, opts);
}

sol::table Remote::postHTTP(const std::string& url, sol::table data)
{
	cpr::Parameters cprParams;
	if (data["params"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("params"))
		{
			cprParams.Add(cpr::Parameter(key.as<std::string>(), value.as<std::string>()));
		}
	}

	cpr::Header cprHeader;
	if (data["header"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("header"))
		{
			const std::string strKey = key.as<std::string>();
			const std::string strVal = value.as<std::string>();
			cprHeader.insert({ strKey, strVal });
		}
	}

	cpr::Body cprBody(data.get_or<std::string>("body", ""));
	
	cpr::Payload cprPayload({});
	if (data["payload"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("payload"))
		{
			cprPayload.Add(cpr::Pair(key.as<std::string>(), value.as<std::string>()));
		}
	}

	cpr::ssl::CaInfo Certs(certificates.c_str());
	cpr::SslOptions opts;
	opts.SetOption(Certs);

	cpr::Response r = cpr::Post(
		cpr::Url(url),
		cprHeader,
		cprBody,
		cprParams,
		cprPayload,
		opts
	);

	sol::table resultTable = Lua.create_table();
	resultTable["status_code"] = r.status_code;
	resultTable["error"] = r.error.code;
	resultTable["error_message"] = r.error.message;
	resultTable["header"] = sol::as_table(r.header);
	resultTable["text"] = r.text;
	return resultTable;
}

void Remote::postHTTPAsync(sol::function handler, const std::string& url, sol::table data)
{
	cpr::Parameters cprParams;
	if (data["params"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("params"))
		{
			cprParams.Add(cpr::Parameter(key.as<std::string>(), value.as<std::string>()));
		}
	}

	cpr::Header cprHeader;
	if (data["header"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("header"))
		{
			const std::string strKey = key.as<std::string>();
			const std::string strVal = value.as<std::string>();
			cprHeader.insert({ strKey, strVal });
		}
	}

	cpr::Body cprBody(data.get_or<std::string>("body", ""));

	cpr::Payload cprPayload({});
	if (data["payload"].valid())
	{
		for (auto [key, value] : data.get<sol::table>("payload"))
		{
			cprPayload.Add(cpr::Pair(key.as<std::string>(), value.as<std::string>()));
		}
	}

	cpr::ssl::CaInfo Certs(certificates.c_str());
	cpr::SslOptions opts;
	opts.SetOption(Certs);

	s_Container.emplace_back(handler, RemoteRequestType::POST, url, cprParams, cprHeader, cprBody, cprPayload, cpr::Authentication("", ""), opts);
}

/*** Internal ***/
void Remote::Init(sol::state* Lua)
{
	s_Container.reserve(32);

	sol::usertype<Remote> userdata = Lua->new_usertype<Remote>("Remote");

	userdata["setSSLCerts"] = &Remote::setSSLCerts;
	userdata["fetch"] = sol::overload(&Remote::fetchHTTP, &Remote::fetchHTTPAsync);
	userdata["post"] = sol::overload(&Remote::postHTTP, &Remote::postHTTPAsync);
}

void Remote::Process(float elapsedTime)
{
	if (s_Container.size() == 0) return;
	using namespace std::chrono_literals;

	for (auto it = s_Container.begin(); it != s_Container.end(); it++)
	{
		if (it->future.wait_for(0s) == std::future_status::ready)
		{
			auto r = it->future.get();
			sol::table resultTable = Lua.create_table();
			resultTable["status_code"] = r.status_code;
			resultTable["error"] = r.error.code;
			resultTable["error_message"] = r.error.message;
			resultTable["header"] = sol::as_table(r.header);
			resultTable["text"] = r.text;
			it->handler(resultTable);

			it = s_Container.erase(it);
			if (it == s_Container.end())
				break;
		}
	}
}

cpr::Parameters Remote::GetParameters(sol::table& params)
{
	cpr::Parameters cprParams;

	for (auto [key, value] : params)
	{
		cpr::Parameter cprParameter = cpr::Parameter(key.as<std::string>(), value.as<std::string>());
		cprParams.Add(cprParameter);
	}

	return cprParams;
}

cpr::Authentication Remote::GetAuthentication(sol::table& auth)
{
	std::string authUser(""), authPassword("");

	if (auth.size() == 2)
	{
		if (auth["user"].valid())
			authUser = auth["user"].get<std::string>();

		if (auth["password"].valid())
			authPassword = auth["password"].get<std::string>();
	}

	cpr::Authentication cprAuth(authUser, authPassword);
	return cprAuth;
}
