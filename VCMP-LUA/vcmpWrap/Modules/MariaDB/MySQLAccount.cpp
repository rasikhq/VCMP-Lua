#include "MySQLAccount.h"

void MySQLAccount::Init(sol::state* Lua) {
	sol::usertype<MySQLAccount> userdata = Lua->new_usertype<MySQLAccount>("__INTERNAL__MYSQLACCOUNT");
}