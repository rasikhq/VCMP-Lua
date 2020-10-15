#pragma once
#include "pch.h"

class MySQLAccount {
public:
	static void Init(sol::state* Lua);

	MySQLAccount(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port = 3306U, const std::string& unix_socket = "")
	{
		m_AccountRef = mariadb::account::create(host, user, password, database, port, unix_socket);
	}

	inline mariadb::account_ref getRef() { return this->m_AccountRef; }
private:
	mariadb::account_ref m_AccountRef = nullptr;

public:

	/*** METHODS ***/
	
};

