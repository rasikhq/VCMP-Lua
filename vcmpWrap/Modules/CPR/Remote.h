#pragma once
#include <pch.h>

#include <cpr/cpr.h>

enum RemoteRequestType
{
	GET,
	POST
};

struct RemoteRequest
{
	RemoteRequest(sol::function luaHandler, 
				RemoteRequestType requestType, 
				const std::string& url, 
				cpr::Parameters cprParams, 
				cpr::Header cprHeader, 
				cpr::Body cprBody, 
				cpr::Payload cprPayload, 
				cpr::Authentication cprAuth,
				cpr::SslOptions opts
	)
		: handler(luaHandler)
	{
		if (requestType == RemoteRequestType::GET)
			future = cpr::GetAsync(cpr::Url(url),
				cprParams,
				cprAuth,
				opts
			);
		else if (requestType == RemoteRequestType::POST)
			future = cpr::PostAsync(cpr::Url(url),
				cprParams,
				cprHeader,
				cprBody,
				cprPayload,
				cprAuth,
				opts
			);
	}

	std::future<cpr::Response> future;
	sol::function handler;
};

class Remote
{
public:
	static void Init(sol::state* Lua);
	static void Process(float elapsedTime);

	static std::vector<RemoteRequest> s_Container;

	/*** Internal ***/
	static cpr::Parameters GetParameters(sol::table& params);
	static cpr::Authentication GetAuthentication(sol::table& auth);

	/*** Lua ***/
	static std::string certificates;

	static bool setSSLCerts(const std::string& bundle);

	static sol::table fetchHTTP(const std::string& url, sol::table params, sol::table auth);
	static void fetchHTTPAsync(sol::function handler, const std::string& url, sol::table params, sol::table auth);

	static sol::table postHTTP(const std::string& url, sol::table data);
	static void postHTTPAsync(sol::function handler, const std::string& url, sol::table data);
};