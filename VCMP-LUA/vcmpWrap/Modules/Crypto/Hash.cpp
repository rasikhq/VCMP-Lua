#include "Hash.h"

void Hash::Init(sol::state* L)
{
	sol::table hashClass = L->create_named_table("Hash");
	sol::table metaTable = L->create_table_with();

	metaTable[sol::meta_function::new_index] = [](lua_State* luaState) -> int {
		return luaL_error(luaState, "Cannot modify internal class");
	};
	metaTable[sol::meta_function::index] = metaTable;

	metaTable["MD5"] = [](const std::string& input) -> std::string { return digestpp::md5().absorb(input).hexdigest(); };
	metaTable["SHA1"] = [](const std::string& input) -> std::string { return digestpp::sha1().absorb(input).hexdigest(); };
	metaTable["SHA256"] = [](const std::string& input) -> std::string { return digestpp::sha256().absorb(input).hexdigest(); };
	metaTable["SHA512"] = [](const std::string& input) -> std::string { return digestpp::sha512().absorb(input).hexdigest(); };

	metaTable["Whirlpool"] = [](const std::string& input) -> std::string { return digestpp::whirlpool().absorb(input).hexdigest(); };

	metaTable["KMAC256"] = [](const std::string& key, const std::string& input) -> std::string { return digestpp::kmac256_xof().set_key(key).absorb(input).hexsqueeze(64); };

	metaTable["SKEIN256"] = [](const std::string& key, const std::string& input) -> std::string { return digestpp::skein256_xof().set_key(key).absorb(input).hexsqueeze(64); };
	metaTable["SKEIN512"] = [](const std::string& key, const std::string& input) -> std::string { return digestpp::skein512_xof().set_key(key).absorb(input).hexsqueeze(64); };
	
	hashClass[sol::metatable_key] = metaTable;
}