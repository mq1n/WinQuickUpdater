#include "../include/pch.hpp"
#include "../include/widget.hpp"
#include "../include/window.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/log_helper.hpp"

namespace WinQuickUpdater
{
	namespace gui
	{
		window::window() :
			widget(0, 0, true)
		{
		}	
		window::~window()
		{
			this->destroy();
		}

		void window::assemble(std::string const& class_name, std::string const& window_name, rectangle rect, const char* icon_name)
		{
			m_class_name = class_name;

			if (!this->create_class(class_name, icon_name))
			{
				CLog::Instance().LogError("create_class fail!", true);
				return;
			}

			if (!this->create_window(class_name, window_name, rect))
			{
				CLog::Instance().LogError("create_window fail!", true);
				return;
			}
			SetWindowLongPtr(this->get_handle(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

			this->set_message_handlers();
		}

		void window::destroy()
		{
			if (!m_class_name.empty())
			{
				UnregisterClass(m_class_name.c_str(), this->get_instance());
				m_class_name.clear();
			}
			// DestroyWindow will call than 'widget' destructor
		}

		bool window::has_message() const
		{
			MSG msg{};
			if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				return false;

			return true;
		}

		bool window::peek_single_message(MSG& msg) const
		{
			if (!PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				return false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
			return true;
		}
		void window::peek_message_loop(std::function<void()> cb)
		{
			MSG msg{};
			while (true)
			{
				while (peek_single_message(msg));
				if (msg.message == WM_QUIT)
					break;

				if (cb)
					cb();
			}
		}

		bool window::get_single_message(MSG& msg) const
		{
			if (GetMessage(&msg, NULL, 0, 0) <= 0)
				return false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
			return true;
		}
		void window::get_message_loop(std::function<void()> cb)
		{
			MSG msg{};
			while (true)
			{
				while (get_single_message(msg));
				if (msg.message == WM_QUIT)
					break;

				if (cb)
					cb();
			};
		}

		bool window::create_class(std::string const& class_name, const char* icon_name)
		{
			WNDCLASSEXA wcEx{};
			ZeroMemory(&wcEx, sizeof(wcEx));

			wcEx.cbSize = sizeof(wcEx);
			wcEx.style = CS_HREDRAW | CS_VREDRAW;
			wcEx.lpfnWndProc = window::WndProc;
			wcEx.hInstance = this->get_instance();
			wcEx.hCursor = LoadCursorA(NULL, IDC_ARROW);
			wcEx.lpszClassName = class_name.c_str();
			wcEx.hIcon = LoadIconA(this->get_instance(), icon_name);
			wcEx.hIconSm = LoadIconA(this->get_instance(), icon_name);

			return (RegisterClassExA(&wcEx) != NULL);
		}

		bool window::create_window(std::string const& class_name, std::string const& window_name, rectangle& rect)
		{
			if (!rect.get_x() && !rect.get_y())
			{
				RECT rc;
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

				//center screen:
				rect.set_x_y((rc.right / 2) - (rect.get_width() / 2), (rc.bottom / 2) - (rect.get_height() / 2));
			}

			return this->create(class_name, window_name, rect, WS_POPUP, 0, NULL);
		}

		void window::set_message_handlers()
		{
			this->add_message_handlers({
				TMessagePair(WM_CLOSE, [](HWND hWnd, WPARAM, LPARAM) -> LRESULT
				{
					// DestroyWindow(hWnd);
					PostQuitMessage(0);
					return 0;
				}),	
				TMessagePair(WM_DESTROY, [](HWND hWnd, WPARAM, LPARAM) -> LRESULT
				{
					PostQuitMessage(0);
					return 0;
				}),
				TMessagePair(WM_LBUTTONDOWN, [](HWND hWnd, WPARAM, LPARAM) -> LRESULT
				{
					auto pos = GetMessagePos();
					POINTS ps = MAKEPOINTS(pos);
					auto p = POINT{ ps.x,ps.y };

					ScreenToClient(hWnd, &p);

					if (p.y < 25 && p.x < 460) {
						ReleaseCapture();
						SendMessage(hWnd, WM_SYSCOMMAND, 0xf012, 0);
					}
					return 0;
				}),
				TMessagePair(WM_SIZE, [](HWND, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (wParam != SIZE_MINIMIZED)
					{
						CD3DManager::Instance().SetPosition(LOWORD(lParam), HIWORD(lParam));
					}
					return 0;
				}),
				TMessagePair(WM_DPICHANGED, [&](HWND, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
					{
						const RECT* suggested_rect = (RECT*)lParam;
						::SetWindowPos(this->get_handle(), NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
					}
					return 0;
				})
			});
		}
	}
};
