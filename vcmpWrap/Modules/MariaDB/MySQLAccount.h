#pragma once
#include <pch.h>

class MySQLAccount {
public:
	static void Init(sol::state* Lua);
//
//	static inline void LogError() {
//		mariadb::last_error error;
//		if (error.error_no() == 0) return;
//		spdlog::error("MySQLAccount Error: [{}] {}", error.error_no(), error.error());
//	}
//
//	MySQLAccount(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port = 3306U, const std::string& unix_socket = "");
//
//	inline mariadb::account_ref getRef() { return this->m_AccountRef; }
//private:
//	mariadb::account_ref m_AccountRef = nullptr;

public:

	/*** METHODS ***/
	
};

