#ifndef GUI_TESTER_TAB_H
#define GUI_TESTER_TAB_H

#include <gui/control.h>
#include <gui/controls/layout.h>
#include <gui/gui.h>
#include <ren/types/texture.h>

namespace evo::gui
{
	class tab : public control
	{
	public:
		tab(const control_id &id, const std::string &t, uint64_t l, const std::shared_ptr<ren::texture> &i = {}) :
			control(id, {0.f, 0.f}, {16.f, 28.f}), text(t), icon(i), container_id(l)
		{
			using namespace ren;

			margin = {0.f, 0.f, 8.f, 0.f};

			anim = std::make_shared<anim_color>(colors.texts.disabled, .15f);
			bg_anim = std::make_shared<anim_float>(0.f, .15f);
			icon_anim = std::make_shared<anim_color>(colors.texts.disabled, .15f);
			ctx->track_accent_anim(icon_anim);

			type = ctrl_tab;
		}

		void render() override;

		void on_mouse_down(char key) override;
		void on_mouse_up(char key) override;
		void on_mouse_enter() override;
		void on_mouse_leave() override;

		virtual void select();
		virtual void unselect();

		void on_first_render_call() override;

		std::shared_ptr<control> make_selected()
		{
			is_selected = true;
			anim->direct(colors.texts.enabled);
			bg_anim->direct(1.f);
			icon_anim->direct(colors.accents.accent);

			return shared_from_this();
		}

		bool is_selected{};
		bool is_highlighted{};

		std::string text{};
		std::shared_ptr<ren::texture> icon{};
		std::shared_ptr<control> container{};

	protected:
		std::shared_ptr<ren::anim_color> anim;
		std::shared_ptr<ren::anim_float> bg_anim;
		std::shared_ptr<ren::anim_color> icon_anim;

	private:
		uint64_t container_id{};
	};
} // namespace evo::gui

#endif // GUI_TESTER_TAB_H
