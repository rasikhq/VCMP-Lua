#include "MySQLConnection.h"

void MySQLConnection::Init(sol::state* Lua) {
	sol::usertype<MySQLConnection> userdata = Lua->new_usertype<MySQLConnection>("__INTERNAL__MYSQLCONNECTION");

	userdata["execute"] = &MySQLConnection::execute;
}

bool MySQLConnection::execute(const std::string& query) {
	try {
		if (!m_Connection->execute(query))
			return false;
		return true;
	}
	catch (std::exception e) {
		spdlog::error("MySQL::execute: {}", e.what());
		return false;
	}
}