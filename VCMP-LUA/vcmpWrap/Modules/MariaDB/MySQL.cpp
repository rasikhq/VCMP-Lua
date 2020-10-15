#include "MySQL.h"

void MySQL::Init(sol::state* Lua) {
	MySQLAccount::Init(Lua);
	MySQLConnection::Init(Lua);

	sol::usertype<MySQL> userdata = Lua->new_usertype<MySQL>("MySQL");
	userdata["createAccount"] = &MySQL::createAccount;
	userdata["createConnection"] = &MySQL::createConnection;
}

MySQLAccount* MySQL::createAccount(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port, const std::string& unix_socket) {
	return new MySQLAccount(host, user, password, database, port, unix_socket);
}

MySQLConnection* MySQL::createConnection(MySQLAccount* account) {
	return new MySQLConnection(account);
}