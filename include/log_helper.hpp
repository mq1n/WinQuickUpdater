#pragma once
#include "abstract_singleton.hpp"
#include "basic_log.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace WinQuickUpdater
{
	static constexpr auto CUSTOM_LOG_FILENAME = "WinQuickUpdater.log";
	static constexpr auto CUSTOM_LOG_ERROR_FILENAME = "WinQuickUpdaterError.log";

	enum ELogLevels
	{
		LL_SYS,
		LL_ERR,
		LL_CRI,
		LL_WARN,
		LL_DEV,
		LL_TRACE
	};

	class CLog : public CSingleton <CLog>
	{
	public:
		CLog() = default;
		CLog(const std::string& stLoggerName, const std::string& stFileName);

		void Log(int32_t nLevel, const std::string& stBuffer);
		void LogError(const std::string& stMessage, bool bFatal = true, bool bShowLastError = true);

	private:
		mutable std::recursive_mutex		m_pkMtMutex;

		std::shared_ptr <spdlog::logger>	m_pkLoggerImpl;
		std::string							m_stLoggerName;
		std::string							m_stFileName;
	};
}
