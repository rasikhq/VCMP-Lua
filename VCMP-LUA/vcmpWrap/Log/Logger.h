#pragma once
#include "pch.h"

class Logger {
public:
	static unsigned short m_Level;
	static std::shared_ptr<spdlog::logger> m_fileLogger;

	static void Init(sol::state* L, spdlog::level::level_enum level = spdlog::level::info) {
		spdlog::set_level(level);

		sol::usertype<Logger> userdata = L->new_usertype<Logger>("Logger",
			"setLevel", &Logger::setLevel,
			"debug", &Logger::debug,
			"info", &Logger::info,
			"warn", &Logger::warn,
			"error", &Logger::error,
			"critical", &Logger::critical);
	}

	inline static void setLevel(unsigned short level) { 
		m_Level = level; 
		spdlog::info("Logging level set to: {}", level);
	}

	inline static void setFile(const char* fileName, unsigned short hour, unsigned short minute) {
		if (m_fileLogger == nullptr) {
			m_fileLogger = spdlog::daily_logger_mt("LOGS", fileName, hour, minute);
			m_fileLogger->flush_on(spdlog::level::trace);
		}
		else
			spdlog::critical("Trying to set log file more than once!");
	}
private:
	inline static void debug(const std::string& s) {
		if (m_fileLogger != nullptr) m_fileLogger->debug(s);
		if (m_Level >= 0) spdlog::debug(s);
	}

	inline static void info(const std::string& s) { 
		if (m_fileLogger != nullptr) m_fileLogger->info(s); 
		if (m_Level >= 1) spdlog::info(s); 
	}

	inline static void warn(const std::string& s) {
		if (m_fileLogger != nullptr) m_fileLogger->warn(s);
		if (m_Level >= 2) spdlog::warn(s);
	}

	inline static void error(const std::string& s) { 
		if (m_fileLogger != nullptr) m_fileLogger->error(s);
		if (m_Level >= 3) spdlog::error(s);
	}

	inline static void critical(const std::string& s) {
		if (m_fileLogger != nullptr) m_fileLogger->critical(s);
		if (m_Level >= 4) spdlog::critical(s);
	}
};

unsigned short Logger::m_Level = 1;
std::shared_ptr<spdlog::logger> Logger::m_fileLogger = nullptr;