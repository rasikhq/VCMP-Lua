#pragma once
#include "pch.h"
#include "MySQLAccount.h"
#include <async++.h>

class MySQLConnection {
public:
	static void Init(sol::state* Lua);

	static inline void LogError() {
		mariadb::last_error error;
		if (error.error_no() == 0) return;
		spdlog::error("MySQLConnection Error: [{}] {}", error.error_no(), error.error());
	}

	MySQLConnection(MySQLAccount* account) : m_AccountHandle(account)
	{
		m_Connection = mariadb::connection::create(m_AccountHandle->getRef());
	}

private:
	MySQLAccount* m_AccountHandle = nullptr;
	mariadb::connection_ref m_Connection = nullptr;
	bool m_ReadyForTask = true;

public:
	/*** METHODS ***/
	std::variant<bool, mariadb::u64> execute(const std::string& query);
	void executeCallback(const sol::function& callback, const std::string& query);
	std::variant<bool, mariadb::u64> executePrepare(const std::string& query, sol::table args);
	void executePrepareCallback(const sol::function& callback, const std::string& query, sol::table args);

	sol::table query(const std::string& query);
	void queryCallback(const sol::function& callback, const std::string& query);
	sol::table queryPrepare(const std::string& query, sol::table args);
	void queryPrepareCallback(const sol::function& callback, const std::string& query, sol::table args);

	std::variant<bool, mariadb::u64> insert(const std::string& query);
	void insertCallback(const sol::function& callback, const std::string& query);
	std::variant<bool, mariadb::u64> insertPrepare(const std::string& query, sol::table args);
	void insertPrepareCallback(const sol::function& callback, const std::string& query, sol::table args);
private:
	std::variant<int, float, std::string, sol::lua_nil_t> GetLuaData(mariadb::result_set_ref result_set, mariadb::u32 index) {
		using namespace mariadb;
		std::variant<int, float, std::string, sol::lua_nil_t> data;
		switch (result_set->column_type(index))
		{
		case value::signed8:
			data = int(result_set->get_signed8(index));
			return std::get<int>(data);
		case value::unsigned8:
			data = int(result_set->get_unsigned8(index));
			return std::get<int>(data);
		case value::signed16:
			data = int(result_set->get_signed16(index));
			return std::get<int>(data);
		case value::unsigned16:
			data = int(result_set->get_unsigned16(index));
			return std::get<int>(data);
		case value::signed32:
			data = int(result_set->get_signed32(index));
			return std::get<int>(data);
		case value::unsigned32:
			data = int(result_set->get_unsigned32(index));
			return std::get<int>(data);
		case value::signed64:
			data = int(result_set->get_signed64(index));
			return std::get<int>(data);
		case value::unsigned64:
			data = int(result_set->get_unsigned64(index));
			return std::get<int>(data);
		case value::float32:
			data = float(result_set->get_float(index));
			return std::get<float>(data);
		case value::blob:
		case value::string:
		case value::data:
			data = result_set->get_string(index);
			return std::get<std::string>(data);
		default:
			data = sol::nil;
			return std::get<sol::lua_nil_t>(data);
		}
	}
};