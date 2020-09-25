#include "../include/pch.hpp"
#include "../include/widget.hpp"
#include "../include/log_helper.hpp"
#include <imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace WinQuickUpdater
{
	namespace gui
	{
		std::unordered_map <HWND, widget*> widget::widget_list;

		LRESULT CALLBACK widget::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
				return true;

			const auto current_widget = widget::widget_list[hWnd];
			if (current_widget)
			{
				if (current_widget->custom_messages[message])
				{
					return current_widget->custom_messages[message](hWnd, wParam, lParam);
				}
				else
				{
					if (current_widget->is_window)
					{
						return DefWindowProc(hWnd, message, wParam, lParam);
					}
					else
					{
						return CallWindowProc(current_widget->original_wndproc, hWnd, message, wParam, lParam);
					}
				}
			}

			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		widget::widget(HWND hwnd_parent, HINSTANCE instance, bool is_window)
			: font(0), hwnd(nullptr), hwnd_parent(hwnd_parent), original_wndproc(nullptr), is_window(is_window), m_visible(true)
		{
			this->instance = (instance != NULL ? instance : reinterpret_cast<HINSTANCE>(GetWindowLong(this->hwnd_parent, GWLP_HINSTANCE)));
		}
		widget::~widget()
		{
			if (this->hwnd)
			{
				this->widget_list.erase(this->hwnd);
				if (IsWindow(this->hwnd))
					DestroyWindow(this->hwnd);

				this->hwnd = nullptr;
			}

			if (this->font)
			{
				DeleteObject(this->font);
				this->font = nullptr;
			}

			this->m_visible = false;
		}

		bool widget::create(const std::string& class_name, const std::string& widget_name, rectangle& rect, unsigned int style, unsigned int ex_style, unsigned int menu_handle)
		{
			this->hwnd = CreateWindowExA(
				ex_style, class_name.c_str(), widget_name.c_str(), style, 
				rect.get_x(), rect.get_y(), rect.get_width(), rect.get_height(),
				this->hwnd_parent, reinterpret_cast<HMENU>(menu_handle), this->instance, NULL
			);
			if (!this->hwnd)
			{
				CLog::Instance().LogError("CreateWindow fail!", true);
				return false;
			}

			this->font = CreateFontA(13, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 5, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
			if (!this->font)
			{
				CLog::Instance().LogError("CreateFontA fail!", true);
				return false;
			}

			SendMessage(this->hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(this->font), TRUE);

			if (!this->is_window)
			{
				this->original_wndproc = reinterpret_cast<WNDPROC>(SetWindowLong(this->hwnd, GWLP_WNDPROC, reinterpret_cast<long>(widget::WndProc)));
			}

			this->widget_list[this->hwnd] = this;
			return true;
		}

		void widget::add_message_handlers(const std::vector <TMessagePair>& vec)
		{
			for (const auto& msg : vec)
			{
				this->custom_messages[msg.first] = msg.second;
			}
		}
		void widget::remove_message_handler(unsigned int message)
		{
			this->custom_messages.erase(message);
		}

		bool widget::set_text(std::string const& text) const
		{
			return (SetWindowTextA(this->hwnd, text.c_str()) != FALSE);
		}

		HWND widget::get_handle() const
		{
			return this->hwnd;
		}

		void widget::set_instance(HINSTANCE instance)
		{
			this->instance = instance;
		}
		HINSTANCE widget::get_instance() const
		{
			return this->instance;
		}

		void widget::set_visible(bool isVisible)
		{
			this->m_visible = isVisible;

			if (this->m_visible)
			{
				ShowWindow(this->hwnd, SW_SHOW);
			}
			else
			{
				ShowWindow(this->hwnd, SW_HIDE);
			}
		}
		bool widget::is_visible() const
		{
			return this->m_visible;
		}

		void widget::show()
		{
			this->m_visible = true;
			ShowWindow(this->hwnd, SW_SHOW);
		}
		void widget::hide()
		{
			this->m_visible = false;
			ShowWindow(this->hwnd, SW_HIDE);
		}

		int	widget::get_screen_width() const
		{
			return GetSystemMetrics(SM_CXSCREEN);
		}
		int	widget::get_screen_height() const
		{
			return GetSystemMetrics(SM_CYSCREEN);
		}

		void widget::get_window_rect(RECT* prc)
		{
			::GetWindowRect(this->hwnd, prc);
		}
		void widget::get_client_rect(RECT* prc)
		{
			::GetClientRect(this->hwnd, prc);
		}

		void widget::get_mouse_position(POINT* ppt)
		{
			GetCursorPos(ppt);
			ScreenToClient(this->hwnd, ppt);
		}

		void widget::set_position(int x, int y)
		{
			SetWindowPos(this->hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		void widget::set_center_position()
		{
			RECT rc;
			get_client_rect(&rc);

			int windowWidth = rc.right - rc.left;
			int windowHeight = rc.bottom - rc.top;

			set_position((get_screen_width() - windowWidth) / 2, (get_screen_height() - windowHeight) / 2);
		}
	}
};
