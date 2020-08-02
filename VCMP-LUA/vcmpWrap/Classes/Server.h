#pragma once
#include "pch.h"

typedef std::unordered_map<std::string, vcmpServerOption> optionUMType;

class Server {
public:
	static void Init(sol::state*);

private:
	static bool getOption(const std::string& option);
	static bool setOption(const std::string& option, bool toggle);
	static sol::object getSettings();

public:
	static optionUMType s_OptionCode;
};