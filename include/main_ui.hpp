#pragma once
#include "abstract_singleton.hpp"

namespace WinQuickUpdater::gui
{
	struct SUpdateContext
	{
		int64_t idx;
		std::wstring id;
		uint8_t type;
		std::wstring name;
		std::wstring desc;
		std::string date;
		bool checked{false};
	};
	enum EUpdaterStatus : uint8_t
	{
		UPDATER_STATUS_UNKNOWN,
		UPDATER_STATUS_INITIALIZED,
		UPDATER_STATUS_SEARCHING,
		UPDATER_STATUS_SEARCH_COMPLETED,
		UPDATER_STATUS_DOWNLOADING,
		UPDATER_STATUS_INSTALLING,
		UPDATER_STATUS_COMPLETED
	};

	class main_ui : public CSingleton <main_ui>
	{
	public:
		main_ui(const std::string& title);
		virtual ~main_ui();

		void DrawMainUI();
		void OnDraw();

		auto IsOpenPtr() { return &m_open; };
		auto IsOpen() const { return m_open; };

		std::string get_current_status();
		auto can_start_update() const { return m_can_start_update; };
		auto get_selected_updates() const { return m_selected_updates; };

		void set_completed(const std::string& type);
		void set_percentage(const std::string& type, int64_t idx, int64_t perc, int64_t total);
		void set_status(uint8_t status);
		void show_progress_bar() { m_show_progressbar = true; };
		void show_update_list(uint32_t item_count) { m_show_listbox = true; m_listbox_item_count = item_count; };
		void add_update(const SUpdateContext& ctx) { m_updates.emplace_back(ctx); };
		void add_target_update(const SUpdateContext& ctx) { m_target_updates.emplace_back(ctx); };

	private:
		uint32_t m_start_time;
		std::string m_title;
		bool m_open;
		bool m_updates_open;
		bool m_focus;
		std::string m_current_object;
		std::vector <SUpdateContext> m_updates;
		std::vector <SUpdateContext> m_target_updates;
		std::vector <uint32_t> m_selected_updates;

		bool m_selected;
		uint8_t m_status;
		bool m_can_start_update;

		bool m_show_listbox;
		uint32_t m_listbox_item_count;

		bool m_show_progressbar;
		uint8_t m_progressbar_step;
	};
};
