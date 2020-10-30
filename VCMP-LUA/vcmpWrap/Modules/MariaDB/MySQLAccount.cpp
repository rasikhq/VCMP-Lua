#include "MySQLAccount.h"

void MySQLAccount::Init(sol::state* Lua) {
	sol::usertype<MySQLAccount> userdata = Lua->new_usertype<MySQLAccount>("__INTERNAL__MYSQLACCOUNT");
}

MySQLAccount::MySQLAccount(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port, const std::string& unix_socket) {
	m_AccountRef = mariadb::account::create(host, user, password, database, port, unix_socket);
}