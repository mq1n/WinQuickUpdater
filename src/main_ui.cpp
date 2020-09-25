#include "../include/pch.hpp"
#include "../include/main_ui.hpp"
#include "../include/application.hpp"

namespace WinQuickUpdater
{
	inline std::string readable_elapsed_time(uint32_t sec)
	{
		int ms = static_cast<int>(sec * 1000.);

		int h = ms / (1000 * 60 * 60);
		ms -= h * (1000 * 60 * 60);

		int m = ms / (1000 * 60);
		ms -= m * (1000 * 60);

		int s = ms / 1000;
		ms -= s * 1000;

		return fmt::format("{0}s {1}m {2}h", s, m, h);
	}
	inline std::string readable_timestamp(const time_t t)
	{
		const auto dt = localtime(&t);

		char buffer[30]{ 0 };
		strftime(buffer, sizeof(buffer), "%H:%M:%S - %d:%m:%y", dt);

		return buffer;
	}

	namespace gui
	{
		main_ui::main_ui(const std::string& title) :
			m_open(true), m_title(title), m_focus(true), m_status(EUpdaterStatus::UPDATER_STATUS_UNKNOWN),
			m_selected(false), m_can_start_update(false), m_show_listbox(false), m_listbox_item_count(0),
			m_show_progressbar(false), m_progressbar_step(0), m_current_object(""), m_updates_open(false)
		{
			m_start_time = static_cast<std::chrono::seconds>(std::time(nullptr)).count();
		}
		main_ui::~main_ui()
		{
		}

		void main_ui::set_completed(const std::string& type)
		{
			m_progressbar_step = 100;
		}
		void main_ui::set_percentage(const std::string& type, int64_t idx, int64_t perc, int64_t total)
		{
			for (auto& update : m_target_updates)
			{
				if (idx == update.idx)
				{
					m_current_object = std::string(update.id.begin(), update.id.end());
				}
			}
			m_progressbar_step = perc;
		}

		std::string main_ui::get_current_status()
		{
			switch (m_status)
			{
				case EUpdaterStatus::UPDATER_STATUS_UNKNOWN:
					return "Waiting...";
				case EUpdaterStatus::UPDATER_STATUS_INITIALIZED:
					return "Initialized.";
				case EUpdaterStatus::UPDATER_STATUS_SEARCHING:
					return "Searching...";
				case EUpdaterStatus::UPDATER_STATUS_SEARCH_COMPLETED:
					return "Search completed.";
				case EUpdaterStatus::UPDATER_STATUS_DOWNLOADING:
					return "Downloading...";
				case EUpdaterStatus::UPDATER_STATUS_INSTALLING:
					return "Installing...";
				case EUpdaterStatus::UPDATER_STATUS_COMPLETED:
					return "Completed.";
				default:
					return "Undefined";
			}
		}
		void main_ui::set_status(uint8_t status)
		{
			m_start_time = static_cast<std::chrono::seconds>(std::time(nullptr)).count();
			m_status = status;
			if (status != EUpdaterStatus::UPDATER_STATUS_COMPLETED)
				m_progressbar_step = 0;
		}

		void main_ui::OnDraw()
		{
			ImGui::Begin("dummy_main_element");
			ImGui::Button("");
			ImGui::End();

			this->DrawMainUI();
		}

		void main_ui::DrawMainUI()
		{
			// Set window size
			if (m_status == UPDATER_STATUS_UNKNOWN || m_status == UPDATER_STATUS_INITIALIZED || m_status == UPDATER_STATUS_SEARCHING)
				ImGui::SetNextWindowSize(ImVec2(600, 250));
			else
				ImGui::SetNextWindowSize(ImVec2(800, 500));

			if (ImGui::Begin(m_title.c_str(), this->IsOpenPtr(), ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGuiViewport* vp = ImGui::GetWindowViewport();
				if (this->m_focus) {
					SetActiveWindow((HWND)vp->PlatformHandle);
					this->m_focus = false;
				}

				if (!m_selected)
				{
					ImGui::SetCursorPosY(50);
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - 80);
					if (ImGui::Button("Start Windows update"))
					{
						CApplication::Instance().get_updater()->start_update();
						m_selected = true;
					}

					ImGui::SetCursorPosY(100);
					ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - 105);
					if (ImGui::Button("Install redisturable packages"))
					{
						CApplication::Instance().get_updater()->start_redist_installer();
						m_selected = true;
					}
				}

				auto text = fmt::format("Status: {}", get_current_status());
				if (m_show_listbox)
					ImGui::SetCursorPosY(460);
				else
					ImGui::SetCursorPosY(210);
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(text.c_str()).x / 2);
				ImGui::TextColored(ImVec4(1, 0, 0, 1), text.c_str());

				text = fmt::format("Elapsed time: {}", readable_elapsed_time(static_cast<std::chrono::milliseconds>(std::time(nullptr)).count() - m_start_time));
				if (m_show_listbox)
					ImGui::SetCursorPosY(470);
				else
					ImGui::SetCursorPosY(225);
				ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(text.c_str()).x - 25);
				ImGui::Text(text.c_str());

				if (m_show_progressbar && m_status != UPDATER_STATUS_SEARCH_COMPLETED)
				{
					const auto curr_step = (float)m_progressbar_step / 100;
					if (m_show_listbox)
						ImGui::SetCursorPosY(430);
					else
						ImGui::SetCursorPosY(110);
					ImGui::ProgressBar(curr_step, ImVec2(ImGui::GetWindowWidth() - 30, 14), fmt::format("%{}", m_progressbar_step).c_str());
				}
				if (m_status == UPDATER_STATUS_DOWNLOADING || m_status == UPDATER_STATUS_INSTALLING)
				{
					const auto text = fmt::format("Current object: {}", m_current_object);
					if (m_show_listbox)
						ImGui::SetCursorPosY(477);
					else
						ImGui::SetCursorPosY(230);
					ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - ImGui::CalcTextSize(text.c_str()).x / 2);
					ImGui::Text(text.c_str());
				}

				if (m_show_listbox)
				{
					ImGui::SetCursorPosX(2);
					ImGui::SetCursorPosY(15);
					if (ImGui::BeginChild("Updates", ImVec2(800, 400), true))
					{
						ImGui::Columns(7, "update_columns", false, false);

						ImGui::Separator();
						ImGui::Text("#"); ImGui::NextColumn();
						ImGui::Text("ID"); ImGui::NextColumn();
						ImGui::Text("Type"); ImGui::NextColumn();
						ImGui::Text("Name"); ImGui::NextColumn();
						ImGui::Text("Desc"); ImGui::NextColumn();
						ImGui::Text("Date"); ImGui::NextColumn();
						ImGui::Text("Install"); ImGui::NextColumn();

						ImGui::Separator();
						ImGui::SetColumnWidth(0, 45);
						ImGui::SetColumnWidth(2, 80);
						ImGui::SetColumnWidth(3, 175);
						ImGui::SetColumnWidth(4, 160);
						ImGui::SetColumnWidth(5, 152);
						ImGui::SetColumnWidth(6, 62);

						for (auto& update : m_updates)
						{
							ImGui::Text("%lld", update.idx);
							ImGui::NextColumn();

							ImGui::Text(update.id.size() > 20 ? "%.20ls..." : "%ls", update.id.c_str());
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("%ls", update.id.c_str());
							ImGui::NextColumn();

							ImGui::Text("%s", update.type == 1 ? "Software" : "Hardware");
							ImGui::NextColumn();

							ImGui::Text(update.name.size() > 20 ? "%.20ls..." : "%ls", update.name.c_str());
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("%ls", update.name.c_str());
							ImGui::NextColumn();

							ImGui::Text(update.desc.size() > 20 ? "%.20ls..." : "%ls", update.desc.c_str());
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("%ls", update.desc.c_str());
							ImGui::NextColumn();

							ImGui::Text("%s", update.date.c_str());
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("%s", update.date.c_str());
							ImGui::NextColumn();

							if (ImGui::Checkbox(fmt::format("##Install_{}", update.idx).c_str(), &update.checked))
							{
								if (std::find(m_selected_updates.begin(), m_selected_updates.end(), update.idx) == m_selected_updates.end())
									m_selected_updates.emplace_back(update.idx);
								else
									m_selected_updates.erase(std::remove(m_selected_updates.begin(), m_selected_updates.end(), update.idx), m_selected_updates.end());
							}
							ImGui::NextColumn();
						}

						ImGui::Columns(1);
						ImGui::Separator();

						ImGui::EndChild();
					}

					if (m_status == UPDATER_STATUS_SEARCH_COMPLETED)
					{
						ImGui::SetCursorPosY(428);
						ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - 110);
						if (ImGui::Button("Select all"))
						{
							for (auto& update : m_updates)
							{
								update.checked = true;
								if (std::find(m_selected_updates.begin(), m_selected_updates.end(), update.idx) != m_selected_updates.end())
									m_selected_updates.emplace_back(update.idx);
							}
						}

						ImGui::SetCursorPosY(428);
						ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) - 20);
						if (ImGui::Button("Install"))
						{
							m_can_start_update = true;
						}

						ImGui::SetCursorPosY(428);
						ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2) + 50);
						if (ImGui::Button("Cancel"))
						{
							std::exit(EXIT_SUCCESS);
						}
					}
				}

				ImGui::End();
			}
		}
	};
};
