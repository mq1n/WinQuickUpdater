#include "../include/pch.hpp"
#include "../include/application.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/window.hpp"
#include "../include/log_helper.hpp"

namespace WinQuickUpdater
{
	// cotr & dotr
	CApplication::CApplication(int argc, char* argv[]) :
		m_config_file("WinQuickUpdate.json"), m_silent(true)
	{
		CLog::Instance().Log(LL_SYS, fmt::format("WinQuickUpdater started! Build: {} {}", __DATE__, __TIME__));

		parse_command_line(argc, argv);
		parse_config_file();
	}
	CApplication::~CApplication()
	{
	}

	// self funcs
	void CApplication::parse_command_line(int argc, char* argv[])
	{
		cxxopts::Options options(argv[0], "");

		options.add_options()
			("s,silent", "Silent mode", cxxopts::value<std::uint8_t>())
		;

		try
		{
			auto result = options.parse(argc, argv);
			if (argc < 2)
				return;

			if (result.count("silent"))
			{
				m_silent = !!result["silent"].as<std::uint8_t>();
				CLog::Instance().Log(LL_SYS, fmt::format("Silent mode: %u", m_silent ? 1 : 0));
			}
		}
		catch (const cxxopts::OptionException& ex)
		{
			CLog::Instance().LogError(fmt::format("IO Console parse exception: {}", ex.what()), true);
		}
		catch (...)
		{
			CLog::Instance().LogError("IO Unhandled exception", true);
		}
	}
	void CApplication::parse_config_file()
	{
		auto read_file = [](const std::string& szFileName) {
			std::ifstream in(szFileName.c_str(), std::ios_base::binary);
			in.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
			return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
		};

		if (!std::filesystem::exists(m_config_file))
		{
			CLog::Instance().Log(LL_WARN, "Config file does not exist!");
			return;
		}

		const auto data = read_file(m_config_file);
		if (data.empty())
		{
			CLog::Instance().Log(LL_ERR, "Config file could not read!");
			return;
		}
		
		auto document = Document{};
		document.Parse(data.c_str());
		if (document.HasParseError())
		{
			CLog::Instance().Log(LL_ERR, fmt::format("Config file could NOT parsed! Error: %s offset: %u", GetParseError_En(document.GetParseError()), document.GetErrorOffset()));
			return;
		}
		if (!document.IsObject())
		{
			CLog::Instance().Log(LL_ERR, fmt::format("Confile file base is not an object! Type: %u", document.GetType()));
			return;
		}

		size_t idx = 0;
		for (auto node = document.MemberBegin(); node != document.MemberEnd(); ++node)
		{
			idx++;

			if (!node->name.IsString())
			{
				CLog::Instance().Log(LL_ERR, fmt::format("Config file node: %u key is not an string! Type: %u", idx, node->name.GetType()));
				continue;
			}

			if (!strcmp(node->name.GetString(), "silent_mode"))
			{
				if (!node->value.IsBool())
				{
					CLog::Instance().Log(LL_ERR, fmt::format("Config file node: %u key: %s value is not an boolean! Type: %u", idx, node->name.GetString(), node->value.GetType()));
					continue;
				}
				m_silent = node->value.GetBool();
				CLog::Instance().Log(LL_SYS, fmt::format("Silent mode: %u", m_silent ? 1 : 0));
			}
		}
	}

	// main routines
	void CApplication::destroy()
	{
		if (CD3DManager::InstancePtr())
		{
			CD3DManager::Instance().CleanupImGui();
			CD3DManager::Instance().CleanupDevice();
		}

		if (m_updater)
			m_updater->destroy();

		if (m_wnd)
			m_wnd->destroy();
	}

	int CApplication::main_routine()
	{
		// Initialize
		m_wnd = std::make_shared<gui::window>();
		assert(m_wnd || m_wnd.get());

		m_ui = std::make_shared<gui::main_ui>("WinQuickUpdater");
		assert(m_ui || m_ui.get());

		m_updater = std::make_shared<win_updater>();
		assert(m_updater || m_updater.get());
		
		// Setup UI
		m_wnd->set_instance(GetModuleHandle(0));
		m_wnd->assemble("WinQuickUpdater", "container_window_name", gui::rectangle(100, 100, 620, 500), nullptr);

		CD3DManager::Instance().SetWindow(m_wnd->get_handle());
		CD3DManager::Instance().CreateDevice();
		CD3DManager::Instance().SetupImGui();
		
		// Hide main windows container window
		::ShowWindow(m_wnd->get_handle(), SW_HIDE);
		::UpdateWindow(m_wnd->get_handle());

		// Setup updater
		m_updater->initialize();

		// Main loop
		m_wnd->peek_message_loop([]() {
			CD3DManager::Instance().OnRenderFrame();
		});

		// End-of-loop cleanup
		this->destroy();
		return EXIT_SUCCESS;
	}
};
