#include "MySQL.h"

void MySQL::Init(sol::state* Lua) {
	/*MySQLAccount::Init(Lua);
	MySQLConnection::Init(Lua);

	sol::usertype<MySQL> userdata = Lua->new_usertype<MySQL>("MySQL");
	userdata["createAccount"] = sol::overload(&MySQL::createAccount, &MySQL::createAccountEx, &MySQL::createAccountEx2);
	userdata["createConnection"] = &MySQL::createConnection;*/
}

M/*ySQLAccount* MySQL::createAccount(const std::string& host, const std::string& user, const std::string& password, const std::string& database) {
	return new MySQLAccount(host, user, password, database);
}

MySQLAccount* MySQL::createAccountEx2(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port, const std::string& unix_socket) {
	return new MySQLAccount(host, user, password, database, port, unix_socket);
}

MySQLAccount* MySQL::createAccountEx(const std::string& host, const std::string& user, const std::string& password, const std::string& database, mariadb::u32 port) {
	return new MySQLAccount(host, user, password, database, port);
}

MySQLConnection* MySQL::createConnection(MySQLAccount* account) {
	return new MySQLConnection(account);
}*/