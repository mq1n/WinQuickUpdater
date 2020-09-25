#pragma once
#include <Windows.h>
#include "ui_types.hpp"

namespace WinQuickUpdater
{
	namespace gui
	{
		using TMessageFunction	= std::function <LRESULT(HWND, WPARAM, LPARAM)>;
		using TMessagePair		= std::pair <unsigned int, TMessageFunction>;

		class widget
		{
		public:
			widget(HWND hwnd_parent = nullptr, HINSTANCE instance = nullptr, bool is_window = false);
			~widget();

			bool create(
				const std::string& class_name, const std::string& widget_name, rectangle& rect,
				unsigned int style, unsigned int ex_style = 0, unsigned int menu_handle = 0
			);

			void add_message_handlers(const std::vector <TMessagePair>& vec);
			void remove_message_handler(unsigned int message);

			bool set_text(const std::string& text) const;
			void set_instance(HINSTANCE instance);
			void set_visible(bool isVisible);
			void show();
			void hide();
			void set_position(int x, int y);
			void set_center_position();

			HWND get_handle() const;
			HINSTANCE get_instance() const;
			bool is_visible() const;
			int get_screen_width() const;
			int get_screen_height() const;
			void get_window_rect(RECT* prc);
			void get_client_rect(RECT* prc);
			void get_mouse_position(POINT* ppt);

		public:
			static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static std::unordered_map <HWND, widget*> widget_list;

		private:
			std::unordered_map <unsigned int, TMessageFunction> custom_messages;

			HFONT font;

			HWND hwnd;
			HWND hwnd_parent;

			HINSTANCE instance;

			WNDPROC original_wndproc;

			bool is_window;
			bool m_visible;
		};
	}
};
