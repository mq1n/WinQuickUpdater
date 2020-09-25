#pragma once
#include "abstract_singleton.hpp"
#include "window.hpp"
#include "main_ui.hpp"
#include "win_updater.hpp"

namespace WinQuickUpdater
{
	class CApplication : public CSingleton <CApplication>
	{
	public:
		CApplication(int argc, char* argv[]);
		virtual ~CApplication();

		void destroy();
		int main_routine();

		auto get_window() const { return m_wnd.get(); };
		auto get_ui() const { return m_ui.get(); };
		auto get_updater() const { return m_updater.get(); };
		auto is_silent() const { return m_silent; };

	protected:
		void parse_command_line(int argc, char* argv[]);
		void parse_config_file();

	private:
		std::string m_config_file;
		bool m_silent;

		std::shared_ptr <gui::window>	m_wnd;
		std::shared_ptr <gui::main_ui>	m_ui;
		std::shared_ptr <win_updater>	m_updater;
	};
};
