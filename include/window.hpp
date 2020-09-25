#pragma once
#include "abstract_singleton.hpp"
#include "ui_types.hpp"
#include "widget.hpp"

namespace WinQuickUpdater
{
	namespace gui
	{
		class window : public CSingleton <window>, public widget
		{
		public:
			window();
			~window();

			void assemble(std::string const& class_name, std::string const& window_name, rectangle rect, const char* icon_name);
			void destroy();

			bool has_message() const;
			bool peek_single_message(MSG& msg) const;
			bool get_single_message(MSG& msg) const;
			void peek_message_loop(std::function<void()> cb);
			void get_message_loop(std::function<void()> cb);

		protected:
			bool create_class(std::string const& class_name, const char* icon_name);
			bool create_window(std::string const& class_name, std::string const& window_name, rectangle& rect);

			void set_message_handlers();

		private:
			std::string m_class_name;
		};
	}
};
