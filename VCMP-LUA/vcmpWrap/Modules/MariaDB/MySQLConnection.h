#pragma once
#include "pch.h"
#include "MySQLAccount.h"

class MySQLConnection {
public:
	static void Init(sol::state* Lua);

	MySQLConnection(MySQLAccount* account) : m_AccountHandle(account)
	{
		m_Connection = mariadb::connection::create(m_AccountHandle->getRef());
	}
private:
	MySQLAccount* m_AccountHandle = nullptr;
	mariadb::connection_ref m_Connection = nullptr;
public:

	/*** METHODS ***/
	bool execute(const std::string& query);
};