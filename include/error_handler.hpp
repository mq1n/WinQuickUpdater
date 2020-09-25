#pragma once
#include <Windows.h>

namespace WinQuickUpdater
{
	class CError
	{
		public:
			CError(DWORD error_code = ::GetLastError()) :
				m_error_code(error_code), m_sys_err(std::system_error(std::error_code(m_error_code, std::system_category()))) {};
			CError(const std::string& message, DWORD error_code = GetLastError()) :
				m_error_code(error_code), m_message(message), m_sys_err(std::system_error(std::error_code(m_error_code, std::system_category()), m_message)) {};
			~CError() = default;

			CError(const CError&) = delete;
			CError(CError&&) noexcept = delete;
			CError& operator=(const CError&) = delete;
			CError& operator=(CError&&) noexcept = delete;
		
			auto code() const { return m_error_code; };
			auto msg() const { return m_message; };
			auto what() const { return m_sys_err.what(); }

		private:
			DWORD m_error_code;
			std::string m_message;
			std::system_error m_sys_err;
	};
};
