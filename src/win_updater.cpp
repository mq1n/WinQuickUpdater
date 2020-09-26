#include "../include/pch.hpp"
#include "../include/win_updater.hpp"
#include "../include/log_helper.hpp"
#include "../include/application.hpp"
#include "../include/main_ui.hpp"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }
#endif

namespace WinQuickUpdater
{
	volatile bool g_update_search_completed = false;

	inline bool get_reboot_permission()
	{
		bool ret = false;
		HANDLE token = nullptr;

		if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
		{
			TOKEN_PRIVILEGES tp;
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid))
			{
				AdjustTokenPrivileges(token, FALSE, &tp, 0, NULL, NULL);
				ret = true;
			}

			CloseHandle(token);
		}

		return ret;
	}

	win_updater::win_updater() : 
		m_start_update(false)
	{
	}
	win_updater::~win_updater()
	{
	}

	void win_updater::start_redist_installer()
	{
		// TODO
	}
	void win_updater::start_update()
	{
		m_start_update = true;
	}
	void win_updater::update_worker()
	{
		while (!CApplication::Instance().get_ui()->IsOpen() || !m_start_update.load())
			std::this_thread::sleep_for(std::chrono::microseconds(100));

		CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_INITIALIZED);

		bool completed = false;
		bool update_downloaded = false;
		bool update_installed = false;
		bool reboot_required = false;

		HRESULT hr = 0;
		std::thread timeout_thread{};

		ISystemInformation* system_info = nullptr;
		IUpdateIdentity* update_identity = nullptr;
		IUpdateSession2* update_session2 = nullptr;
		IUpdateCollection* update_collection = nullptr;
		IUpdateSearcher* searcher = nullptr;
		IUpdateDownloader* downloader = nullptr;
		IUpdateInstaller* installer = nullptr;
		IUpdateInstaller2* installer2 = nullptr;
		IUpdateCollection* update_list = nullptr;
		IUpdate* update_item = nullptr;
		IDownloadResult* download_result = nullptr;
		IDownloadJob* download_job = nullptr;
		IInstallationResult* installer_result = nullptr;
		IInstallationJob* installer_job = nullptr;

		do
		{
			// Initialize COM
			hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("CoInitialize failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			hr = CoCreateInstance(CLSID_UpdateSession, NULL, CLSCTX_INPROC_SERVER, IID_IUpdateSession2, (LPVOID*)&update_session2);
			if (FAILED(hr) || !update_session2)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("CoCreateInstance(CLSID_UpdateSession) failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			hr = CoCreateInstance(CLSID_UpdateCollection, NULL, CLSCTX_INPROC_SERVER, IID_IUpdateCollection, (void**)&update_collection);
			if (FAILED(hr) || !update_collection)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("CoCreateInstance(CLSID_UpdateCollection) failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			hr = CoCreateInstance(CLSID_SystemInformation, NULL, CLSCTX_INPROC_SERVER, IID_ISystemInformation, (void**)&system_info);
			if (FAILED(hr) || !system_info)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("CoCreateInstance(CLSID_SystemInformation) failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			// Set default language
			const auto english_id = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
			hr = update_session2->put_UserLocale(english_id);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_session2(put_UserLocale) failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			// Check current status
			VARIANT_BOOL reboot_required;
			if (SUCCEEDED(system_info->get_RebootRequired(&reboot_required)) && reboot_required == VARIANT_TRUE)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("Reboot required for continue to Windows update."), true, false);
				break;
			}

			CApplication::Instance().get_ui()->show_progress_bar();

			// Searcher routine
			hr = update_session2->CreateUpdateSearcher(&searcher);
			if (FAILED(hr) || !searcher)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("CreateUpdateSearcher failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			hr = searcher->put_ServerSelection(ssWindowsUpdate);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("put_ServerSelection failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			hr = searcher->put_Online(VARIANT_TRUE);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("put_Online failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Checking for updates...");
			CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_SEARCHING);

			if (timeout_thread.joinable())
			{
				WinQuickUpdater::CLog::Instance().LogError("Timeout thread in joinable state", true, false);
				break;
			}

			auto timeout_func = [&] {
				std::this_thread::sleep_for(std::chrono::seconds(30));
				if (!g_update_search_completed)
					WinQuickUpdater::CLog::Instance().LogError("Update search timeout", true, false);
			};

			std::thread temp_thread(timeout_func);
			std::swap(timeout_thread, temp_thread);

			timeout_thread.detach();

			ISearchResult* results = nullptr;
			hr = searcher->Search(ComStr{ "IsInstalled=0 or IsHidden=1 or IsPresent=1" }, &results);
			if (FAILED(hr) || !results)
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("searcher->Search failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Update search completed.");

			g_update_search_completed = true;

			hr = results->get_Updates(&update_list);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("results->get_Updates failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			LONG updateSize = 0;
			hr = update_list->get_Count(&updateSize);
			if (FAILED(hr))
			{
				WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_list->get_Count failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
				break;
			}

			if (updateSize == 0)
			{
				WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "No updates found");
				completed = true;
				break;
			}

			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, fmt::format("{} updates found", updateSize));
			CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_SEARCH_COMPLETED);

			auto idx = 0u;
			auto update_found = false;
			for (LONG i = 0; i < updateSize; i++)
			{
				hr = update_list->get_Item(i, &update_item);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_list->get_Item failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				BSTR updateName;
				hr = update_item->get_Title(&updateName);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_Title failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				BSTR desc;
				hr = update_item->get_Description(&desc);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_Description failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				hr = update_item->get_Identity(&update_identity);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_Identity failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				BSTR update_id;
				hr = update_identity->get_UpdateID(&update_id);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_identity->get_UpdateID failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				UpdateType type;
				hr = update_item->get_Type(&type);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_Type failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				DATE retdate;
				hr = update_item->get_LastDeploymentChangeTime(&retdate);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_LastDeploymentChangeTime failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}
				const auto unix_time = (double)retdate * 86400 - 2209161600;
				const auto p = std::chrono::system_clock::time_point(std::chrono::seconds((int64_t)unix_time));
				const auto t = std::chrono::system_clock::to_time_t(p);
				const auto human_readable_time = std::ctime(&t);

				VARIANT_BOOL installed;
				hr = update_item->get_IsInstalled(&installed);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_item->get_IsInstalled failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				const auto w_update_name = ComStrToStdStr(updateName);
				const auto s_update_name = std::string(w_update_name.begin(), w_update_name.end());

				WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, fmt::format("Pending update({0}) Name: {1} Date: {2} Installed: {3}",
					i, s_update_name, retdate, installed == VARIANT_TRUE
				));

				if (installed != VARIANT_TRUE)
				{
					update_found = true;
					CApplication::Instance().get_ui()->add_update({ idx++, ComStrToStdStr(update_id), (uint8_t)type, w_update_name, ComStrToStdStr(desc), human_readable_time });
				}

				SAFE_RELEASE(update_item);
			}
			CApplication::Instance().get_ui()->show_update_list(updateSize);

			if (!update_found)
				WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Any downloadable update could not found!");

			while (!CApplication::Instance().get_ui()->can_start_update())
				std::this_thread::sleep_for(std::chrono::microseconds(100));

			const auto selected_updates = CApplication::Instance().get_ui()->get_selected_updates();
			for (const auto& idx : selected_updates)
			{
				hr = update_list->get_Item(idx, &update_item);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_list->get_Item failed with error: {:#x} {} index: {}", hr, ComErrorMessage(hr), idx), false, false);
					continue;
				}

				BSTR update_id;
				hr = update_identity->get_UpdateID(&update_id);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_identity->get_UpdateID failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				LONG idx;
				hr = update_collection->Add(update_item, &idx);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_collection->Add failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				CApplication::Instance().get_ui()->add_target_update({idx, ComStrToStdStr(update_id) });
			}

			// EULA routine
			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "EULA check processing...");

			if (update_found && WinQuickUpdater::CApplication::Instance().is_silent())
			{
				LONG count = 0;
				hr = update_collection->get_Count(&count);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_collection->get_Count failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
					continue;
				}

				for (LONG i = 0; i < count; ++i)
				{
					hr = update_collection->get_Item(i, &update_item);
					if (FAILED(hr))
					{
						WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_collection->get_Item failed with error: {:#x} {}", hr, ComErrorMessage(hr)), false, false);
						continue;
					}

					update_item->AcceptEula();

					SAFE_RELEASE(update_item)
				}
			}
			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "EULA check completed.");

			// Downloader routine
			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Downloader routine started...");
			CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_DOWNLOADING);

			if (update_found)
			{
				hr = update_session2->CreateUpdateDownloader(&downloader);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_session->CreateUpdateDownloader failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				hr = downloader->put_Updates(update_collection);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("downloader->put_Updates failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				IDPC progressCB{[&](LONG idx, LONG perc, LONG total) {
					CApplication::Instance().get_ui()->set_percentage("download", idx, perc, total);
				}};
				IDCC completeCB{[&]() {
					CApplication::Instance().get_ui()->set_completed("download");
				}};
				VARIANT download_var = { 0 };
				hr = downloader->BeginDownload((IUnknown*)&progressCB, (IUnknown*)&completeCB, download_var, &download_job);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("downloader->BeginDownload failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				/*
				DWORD dwWaitCounter = 0;
				DWORD dwWaitRet = 0;
				do {
					if (dwWaitCounter++ > 30)
					{
						WinQuickUpdater::CLog::Instance().LogError("Download timed out", false, false);
						break;
					}
					dwWaitRet = WaitForSingleObject(completeCB.GetEvent(), 1000);
				} while (dwWaitRet == WAIT_TIMEOUT)
				*/
				WaitForSingleObject(completeCB.GetEvent(), INFINITE);

				hr = downloader->EndDownload(download_job, &download_result);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("downloader->EndDownload failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				OperationResultCode code;
				hr = download_result->get_ResultCode(&code);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("download_result->get_ResultCode failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				if (code != orcSucceeded && code != orcSucceededWithErrors)
				{
					WinQuickUpdater::CLog::Instance().Log(LL_ERR, fmt::format("Update download could not complete. Error: {}", code));
					break;
				}

				WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Downloader routine completed.");
				update_downloaded = true;
			}

			// Installler routine
			WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, "Installer routine started...");
			CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_INSTALLING);

			if (update_downloaded)
			{
				hr = update_session2->CreateUpdateInstaller(&installer);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("update_session->CreateUpdateInstaller failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				hr = installer->put_Updates(update_collection);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("installer->put_Updates failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				if (WinQuickUpdater::CApplication::Instance().is_silent())
				{
					hr = installer->QueryInterface(__uuidof(IUpdateInstaller2), (LPVOID*)&installer2);
					if (SUCCEEDED(hr))
					{
						installer2->put_ForceQuiet(VARIANT_TRUE);
					}
				}

				IIPC progressCB{[&](LONG idx, LONG perc, LONG total) {
					CApplication::Instance().get_ui()->set_percentage("install", idx, perc, total);
				}};
				IICC completeCB{[&]() {
					CApplication::Instance().get_ui()->set_completed("install");
				}};
				VARIANT install_var = { 0 };
				hr = installer->BeginInstall((IUnknown*)&progressCB, (IUnknown*)&completeCB, install_var, &installer_job);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("installer->BeginInstall failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				/*
				DWORD dwWaitCounter = 0;
				DWORD dwWaitRet = 0;
				do {
					if (dwWaitCounter++ > 30)
					{
						WinQuickUpdater::CLog::Instance().LogError("Install timed out", false, false);
						break;
					}
					dwWaitRet = WaitForSingleObject(completeCB.GetEvent(), 1000);
				} while (dwWaitRet == WAIT_TIMEOUT)
				*/
				WaitForSingleObject(completeCB.GetEvent(), INFINITE);

				hr = installer->EndInstall(installer_job, &installer_result);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("installer->EndInstall failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				OperationResultCode code;
				hr = installer_result->get_ResultCode(&code);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("installer_result->get_ResultCode failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				if (code != orcSucceeded && code != orcSucceededWithErrors)
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("Update install could not complete. Error: {}", code), true, false);
					break;
				}

				update_installed = true;

				VARIANT_BOOL reboot_var;
				hr = installer_result->get_RebootRequired(&reboot_var);
				if (FAILED(hr))
				{
					WinQuickUpdater::CLog::Instance().LogError(fmt::format("installer_result->get_RebootRequired failed with error: {:#x} {}", hr, ComErrorMessage(hr)), true, false);
					break;
				}

				if (reboot_var == VARIANT_TRUE)
					reboot_required = true;

				WinQuickUpdater::CLog::Instance().Log(WinQuickUpdater::LL_SYS, fmt::format("Installer routine completed. Reboot required: {}", reboot_required));

				if (reboot_required && get_reboot_permission())
				{
					const auto reboot_res = MessageBox(NULL, "Reboot required for complete Windows update installer, Should you continue?", "WinQuickUpdater", MB_YESNO | MB_ICONINFORMATION);
					if (reboot_res == IDYES)
						if (!ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED))
							WinQuickUpdater::CLog::Instance().LogError("ExitWindowsEx failed", true, true);
				}
			}

			completed = true;
		} while (false);

		CApplication::Instance().get_ui()->set_status(gui::UPDATER_STATUS_COMPLETED);

		if (timeout_thread.joinable())
			timeout_thread.join();

		SAFE_RELEASE(system_info);
		SAFE_RELEASE(update_identity);
		SAFE_RELEASE(update_session2);
		SAFE_RELEASE(update_collection);
		SAFE_RELEASE(searcher);
		SAFE_RELEASE(downloader);
		SAFE_RELEASE(installer);
		SAFE_RELEASE(installer2);
		SAFE_RELEASE(update_list);
		SAFE_RELEASE(update_item);
		SAFE_RELEASE(download_result);
		SAFE_RELEASE(download_job);
		SAFE_RELEASE(installer_result);
		SAFE_RELEASE(installer_job);

		this->destroy();
	}

	bool win_updater::initialize()
	{
		if (m_worker.joinable())
		{
			WinQuickUpdater::CLog::Instance().LogError("Worker thread in joinable state", true, false);
			return false;
		}

		std::thread temp_thread(std::bind(&win_updater::update_worker, this));
		std::swap(m_worker, temp_thread);

		m_worker.detach();
		return true;
	}
	void win_updater::destroy()
	{
		if (m_worker.joinable())
			m_worker.join();

		CoUninitialize();
	}
};
