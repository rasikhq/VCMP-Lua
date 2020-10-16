#include "MySQLConnection.h"

extern sol::state Lua;

void MySQLConnection::Init(sol::state* Lua) {
	sol::usertype<MySQLConnection> userdata = Lua->new_usertype<MySQLConnection>("__INTERNAL__MYSQLCONNECTION");

	userdata["execute"]	= sol::overload(&MySQLConnection::execute, &MySQLConnection::executeCallback, &MySQLConnection::executePrepare, &MySQLConnection::executePrepareCallback);
	userdata["query"]	= sol::overload(&MySQLConnection::query, &MySQLConnection::queryCallback, &MySQLConnection::queryPrepare, &MySQLConnection::queryPrepareCallback);
	userdata["insert"]	= sol::overload(&MySQLConnection::insert, &MySQLConnection::insertCallback, &MySQLConnection::insertPrepare, &MySQLConnection::insertPrepareCallback);
}

std::variant<bool, mariadb::u64> MySQLConnection::execute(const std::string& query) {
	try {
		auto rows_affected = m_Connection->execute(query);
		if (!rows_affected) {
			LogError();
			return false;
		}
		return rows_affected;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::execute: {}", e.what());
		return false;
	}
}

void MySQLConnection::executeCallback(const sol::function& callback, const std::string& query) {
	try {
		if (callback.valid()) {
			auto result = this->execute(query);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::executeCallback: {}", e.what());
	}
}

std::variant<bool, mariadb::u64> MySQLConnection::executePrepare(const std::string& query, sol::table args) {
	try {
		mariadb::statement_ref statement = m_Connection->create_statement(query);
		for (const auto& pair : args) {
			int key = pair.first.as<int>();
			sol::object value = pair.second;
			switch (value.get_type()) {
				case sol::type::string:
					statement->set_string(key-1, value.as<std::string>());
					break;
				case sol::type::number:
					statement->set_float(key-1, value.as<float>());
					break;
				case sol::type::nil:
				default:
					statement->set_null(key-1);
					break;
			}
		}
		auto rows_affected = statement->execute();
		if (!rows_affected) {
			LogError();
			return false;
		}
		return rows_affected;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::executePrepare: {}", e.what());
		return false;
	}
}

void MySQLConnection::executePrepareCallback(const sol::function& callback, const std::string& query, sol::table args) {
	try {
		if (callback.valid()) {
			auto result = this->executePrepare(query, args);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::executePrepareCallback: {}", e.what());
	}
}

sol::table MySQLConnection::query(const std::string& query) {
	try {
		mariadb::result_set_ref result_set = m_Connection->query(query);
		sol::table result = Lua.create_table();
		int row_id = 0;
		while (result_set->next()) {
			row_id++;
			auto row_index = result_set->row_index();
			auto column_count = result_set->column_count();
			auto row = Lua.create_table();
			for (mariadb::u32 i = 0; i < column_count; i++) {
				auto column_name = result_set->column_name(i);
				row[column_name] = GetLuaData(result_set, i);
			}
			result[row_id] = row;
		}
		return result;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::query: {}", e.what());
		return sol::nil;
	}
}

void MySQLConnection::queryCallback(const sol::function& callback, const std::string& query) {
	try {
		if (callback.valid()) {
			auto result = this->query(query);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::queryCallback: {}", e.what());
	}
}

sol::table MySQLConnection::queryPrepare(const std::string& query, sol::table args) {
	try {
		// Prepare the statement
		mariadb::statement_ref statement = m_Connection->create_statement(query);
		for (const auto& pair : args) {
			int key = pair.first.as<int>();
			sol::object value = pair.second;
			switch (value.get_type()) {
			case sol::type::string:
				statement->set_string(key - 1, value.as<std::string>());
				break;
			case sol::type::number:
				statement->set_float(key - 1, value.as<float>());
				break;
			case sol::type::nil:
			default:
				statement->set_null(key - 1);
				break;
			}
		}
		
		// Query it
		mariadb::result_set_ref result_set = statement->query();
		sol::table result = Lua.create_table();
		int row_id = 0;
		while (result_set->next()) {
			row_id++;
			auto row_index = result_set->row_index();
			auto column_count = result_set->column_count();
			auto row = Lua.create_table();
			for (mariadb::u32 i = 0; i < column_count; i++) {
				auto column_name = result_set->column_name(i);
				row[column_name] = GetLuaData(result_set, i);
			}
			result[row_id] = row;
		}
		return result;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::queryPrepare: {}", e.what());
		return sol::nil;
	}
}

void MySQLConnection::queryPrepareCallback(const sol::function& callback, const std::string& query, sol::table args) {
	try {
		if (callback.valid()) {
			auto result = this->queryPrepare(query, args);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		spdlog::error("MySQL::queryPrepareCallback: {}", e.what());
	}
}

std::variant<bool, mariadb::u64> MySQLConnection::insert(const std::string& query) {
	try {
		auto last_row_id = m_Connection->insert(query);
		if (!last_row_id) {
			LogError();
			return false;
		}
		return last_row_id;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::insert: {}", e.what());
		return false;
	}
}

void MySQLConnection::insertCallback(const sol::function& callback, const std::string& query) {
	try {
		if (callback.valid()) {
			auto result = this->insert(query);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::insertCallback: {}", e.what());
	}
}

std::variant<bool, mariadb::u64> MySQLConnection::insertPrepare(const std::string& query, sol::table args) {
	try {
		mariadb::statement_ref statement = m_Connection->create_statement(query);
		for (const auto& pair : args) {
			int key = pair.first.as<int>();
			sol::object value = pair.second;
			switch (value.get_type()) {
			case sol::type::string:
				statement->set_string(key - 1, value.as<std::string>());
				break;
			case sol::type::number:
				statement->set_float(key - 1, value.as<float>());
				break;
			case sol::type::nil:
			default:
				statement->set_null(key - 1);
				break;
			}
		}
		auto last_row_id = statement->insert();
		if (!last_row_id) {
			LogError();
			return false;
		}
		return last_row_id;
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::insertPrepare: {}", e.what());
		return false;
	}
}

void MySQLConnection::insertPrepareCallback(const sol::function& callback, const std::string& query, sol::table args) {
	try {
		if (callback.valid()) {
			auto result = this->insertPrepare(query, args);
			callback.call(result);
		}
	}
	catch (std::exception e) {
		LogError();
		spdlog::error("MySQL::insertPrepareCallback: {}", e.what());
	}
}