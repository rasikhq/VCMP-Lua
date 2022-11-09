#pragma once
#include <pch.h>

#include <SQLiteCpp/SQLiteCpp.h>

/*** DATABASE WRAP ***/
class SqLiteDatabase
{
public:
	static void Init(sol::state*);
	SqLiteDatabase(const std::string&);
	~SqLiteDatabase();
private:
	void destroy();

	void createBackup();
	void loadBackup(const std::string);

	int exec(const std::string&);
	int execPrepare(const std::string&, sol::table);

	std::variant<bool, sol::table> query(const std::string&);
	std::variant<bool, sol::table> queryPrepare(const std::string&, sol::table);

	long long getLastInsertRowID();
	bool tableExists(const std::string&);
private:
	SQLite::Database* m_Database = nullptr;
};


/*** SQLITE USERTYPE ***/
class SqLite
{
public:
	static void Init(sol::state*);
private:
	static SqLiteDatabase* openDatabase(const std::string&);
	static void closeDatabase(SqLiteDatabase*);
};