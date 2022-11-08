#include "SqLite.h"

extern sol::state Lua;

/*** DATABASE ***/
void SqLiteDatabase::Init(sol::state* L)
{
	sol::usertype<SqLiteDatabase> userdata = L->new_usertype<SqLiteDatabase>("SQLiteDatabase", sol::constructors<SqLiteDatabase(const std::string&)>());
	userdata["exec"] = sol::overload(&SqLiteDatabase::exec, &SqLiteDatabase::execPrepare);
	userdata["query"] = sol::overload(&SqLiteDatabase::query, &SqLiteDatabase::queryPrepare);

	userdata["createBackup"] = &SqLiteDatabase::createBackup;
	userdata["loadBackup"] = &SqLiteDatabase::loadBackup;

	userdata["getLastInsertID"] = &SqLiteDatabase::getLastInsertRowID;
	userdata["tableExists"] = &SqLiteDatabase::tableExists;

	// PERSONAL NOTE: Maybe add support for database key?
}

SqLiteDatabase::SqLiteDatabase(const std::string& db)
{
	m_Database = new SQLite::Database(db, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
}

SqLiteDatabase::~SqLiteDatabase()
{
	destroy();
}

void SqLiteDatabase::destroy()
{
	delete m_Database;
}

void SqLiteDatabase::createBackup()
{
	auto dbName = m_Database->getFilename();
	std::string backupName = dbName + "_backup";
	m_Database->backup(backupName.c_str(), SQLite::Database::BackupType::Save);
}

void SqLiteDatabase::loadBackup(const std::string backupFile)
{
	try {
		m_Database->backup(backupFile.c_str(), SQLite::Database::BackupType::Load);
		spdlog::info("SQLiteDatabase::loadBackup >> Backup database {} imported successfully!", backupFile);
	}
	catch (std::exception& e)
	{
		spdlog::error("SQLiteDatabase::loadBackup >> {}", e.what());
	}
}

int SqLiteDatabase::exec(const std::string& query)
{
	return m_Database->exec(query);
}

int SqLiteDatabase::execPrepare(const std::string& query, sol::table args)
{
	SQLite::Statement statement(*m_Database, query);
	try {
		for (const auto& pair : args) {
			sol::object value = pair.second;
			switch (value.get_type()) {
			case sol::type::string:
				if (pair.first.is<int>())
					statement.bind(pair.first.as<int>(), value.as<std::string>());
				else
					statement.bind(pair.first.as<std::string>(), value.as<std::string>());
				break;
			case sol::type::number:
				if (pair.first.is<float>())
					statement.bind(pair.first.as<float>(), value.as<float>());
				else
					statement.bind(pair.first.as<std::string>(), value.as<float>());
				break;
			case sol::type::nil:
			default:
				if (pair.first.is<int>())
					statement.bind(pair.first.as<int>());
				else
					statement.bind(pair.first.as<std::string>());
				break;
			}
		}
		return statement.exec();
	}
	catch (std::exception e) {
		spdlog::error("SQLite::executePrepare: {}", e.what());
		return false;
	}
}

std::variant<bool, sol::table> SqLiteDatabase::query(const std::string& query)
{
	SQLite::Statement statement(*m_Database, query);
	if (query.empty()) return false;
	sol::table result = Lua.create_table_with();
	int row_id = 0;
	while (statement.executeStep())
	{
		row_id++;
		auto column_count = statement.getColumnCount();
		auto row = Lua.create_table();
		for (int i = 0; i < column_count; i++)
		{
			auto column_name = statement.getColumnName(i);
			auto column = statement.getColumn(i);
			if (column.isInteger())
				row[column_name] = column.getInt();
			else if (column.isFloat())
				row[column_name] = column.getDouble();
			else if (column.isText())
				row[column_name] = column.getText();
			else if (column.isBlob())
				row[column_name] = column.getBlob();
			else if (column.isNull())
				row[column_name] = sol::nil;
		}
		result[row_id] = row;
	}
	return result;
}

std::variant<bool, sol::table> SqLiteDatabase::queryPrepare(const std::string& query, sol::table args)
{
	if (query.empty()) return false;
	SQLite::Statement statement(*m_Database, query);
	try {
		for (const auto& pair : args) {
			sol::object value = pair.second;
			switch (value.get_type()) {
			case sol::type::string:
				if (pair.first.is<int>())
					statement.bind(pair.first.as<int>(), value.as<std::string>());
				else
					statement.bind(pair.first.as<std::string>(), value.as<std::string>());
				break;
			case sol::type::number:
				if (pair.first.is<float>())
					statement.bind(pair.first.as<float>(), value.as<float>());
				else
					statement.bind(pair.first.as<std::string>(), value.as<float>());
				break;
			case sol::type::nil:
			default:
				if (pair.first.is<int>())
					statement.bind(pair.first.as<int>());
				else
					statement.bind(pair.first.as<std::string>());
				break;
			}
		}

		sol::table result = Lua.create_table_with();
		int row_id = 0;
		while (statement.executeStep())
		{
			row_id++;
			auto column_count = statement.getColumnCount();
			auto row = Lua.create_table();
			for (int i = 0; i < column_count; i++)
			{
				auto column = statement.getColumn(i);
				auto column_name = column.getName();
				if (column.isInteger())
					row[column_name] = column.getInt();
				else if (column.isFloat())
					row[column_name] = column.getDouble();
				else if (column.isText())
					row[column_name] = column.getText();
				else if (column.isBlob())
					row[column_name] = column.getBlob();
				else if (column.isNull())
					row[column_name] = sol::nil;
			}
			result[row_id] = row;
		}
		return result;
	}
	catch (std::exception e) {
		spdlog::error("SQLite::queryPrepare: {}", e.what());
		return false;
	}
}

long long SqLiteDatabase::getLastInsertRowID()
{
	return m_Database->getLastInsertRowid();
}

bool SqLiteDatabase::tableExists(const std::string& tableName)
{
	return m_Database->tableExists(tableName);
}

/*** USERTYPE ***/
SqLiteDatabase* SqLite::openDatabase(const std::string& db)
{
	if (db.empty())
	{
		spdlog::error("SQLite::openDatabase >> Provide a valid database name!");
		return nullptr;
	}
	return (new SqLiteDatabase(db));
}

void SqLite::closeDatabase(SqLiteDatabase* sqliteDB)
{
	delete sqliteDB;
	sqliteDB = nullptr;
}

void SqLite::Init(sol::state* L)
{
	SqLiteDatabase::Init(L);

	sol::usertype<SqLite> userdata = L->new_usertype<SqLite>("SQLite");

	userdata["openDatabase"] = &SqLite::openDatabase;
	userdata["closeDatabase"] = &SqLite::closeDatabase;
}