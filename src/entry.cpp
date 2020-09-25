#include "../include/pch.hpp"
#include "../include/application.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/log_helper.hpp"
#include <clocale>

volatile bool interrupted{ false };

static void interrupt_handler(int signal)
{
	if (interrupted)
		return;
	interrupted = true;

	if (WinQuickUpdater::CApplication::InstancePtr())
		WinQuickUpdater::CApplication::Instance().destroy();
}

int main(int, char**)
{
	// Check ImGui version
	IMGUI_CHECKVERSION();

	// Hide console window
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	// Setup signal handlers
	std::signal(SIGINT, &interrupt_handler);
	std::signal(SIGABRT, &interrupt_handler);
	std::signal(SIGTERM, &interrupt_handler);

	// Initialize instances
	static WinQuickUpdater::CLog kLogManager("WinQuickUpdater", WinQuickUpdater::CUSTOM_LOG_FILENAME);
	static WinQuickUpdater::CD3DManager kD3DManager;
	static WinQuickUpdater::CApplication kApplication(__argc, __argv);

	// Start to main routine
	return kApplication.main_routine();
}
