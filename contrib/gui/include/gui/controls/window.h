#ifndef GUI_TESTER_WINDOW_H
#define GUI_TESTER_WINDOW_H

#include <gui/controls/control_container.h>
#include <gui/gui.h>
#include <gui/input.h>

namespace evo::gui
{
	class window : public control_container
	{
	public:
		window(const control_id &id, const ren::vec2 &p, const ren::vec2 &s) : control_container(id, p, s)
		{
			using namespace ren;

			is_window = true;
			priority = ip_window;
			sort = ++ctx->top_sort;

			pfp_anim = std::make_shared<anim_float>(.0f, .15f);
			src_anim = std::make_shared<anim_color>(colors.texts.disabled, .15f);
			type = ctrl_window;
		}

		void add(const std::shared_ptr<control> &c) override;

		void render() override;

		void on_mouse_down(char key) override;
		void on_mouse_up(char key) override;
		void on_mouse_move(const ren::vec2 &p, const ren::vec2 &d) override;

		bool allow_move{true};

	private:
		ren::vec2 get_avatar_pos();
		bool is_mouse_on_avatar{};

		ren::vec2 get_search_pos();
		bool is_mouse_on_search{};

		std::shared_ptr<ren::anim_float> pfp_anim;
		std::shared_ptr<ren::anim_color> src_anim;
	};
} // namespace evo::gui

#endif // GUI_TESTER_WINDOW_H
