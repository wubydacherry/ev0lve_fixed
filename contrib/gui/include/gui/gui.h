#ifndef GUI_GUI_H
#define GUI_GUI_H

#include <memory>
#include <stack>

#include <ren/adapter.h>
#include <ren/misc.h>
#include <ren/renderer.h>
#include <ren/types/texture.h>

#include <gui/anim.h>
#include <gui/container.h>
#include <gui/controls/control_container.h>
#include <gui/controls/popup.h>
#include <gui/input.h>
#include <gui/misc.h>
#include <gui/notify_system.h>
#include <gui/values.h>

#define GUI_NAMESPACE                                                                                                                                                    \
	using namespace evo;                                                                                                                                                 \
	using namespace gui;                                                                                                                                                 \
	using namespace ren
#define MAKE_COLOR(n, r, g, b) ren::color n = ren::color(r, g, b)

#ifndef EVO_GUI_XOR
#define XOR(x) (x)
#define XOR_STR(x) (x)
#else
#include EVO_GUI_XOR
#endif

namespace evo::ren
{
	void add_with_blur(std::shared_ptr<layer> &d, const rect &area, const std::function<void(std::shared_ptr<layer> &)> &fn);
}

namespace evo::gui
{
	enum gui_layers : int
	{
		l_back = 32,
		l_mid,
		l_popup_back,
		l_popup_mid,
		l_popup_top,
		l_top,
		l_last,
	};

	struct color_list
	{
		MAKE_COLOR(accent, 0, 172, 245);
		MAKE_COLOR(text, 220, 220, 220);
		MAKE_COLOR(text_light, 180, 180, 180);
		MAKE_COLOR(text_mid, 120, 120, 120);
		MAKE_COLOR(text_dark, 74, 74, 74);
		MAKE_COLOR(bg_bottom, 18, 18, 18);
		MAKE_COLOR(bg_block, 29, 29, 29);
		MAKE_COLOR(bg_block_overlay, 40, 40, 40);
		MAKE_COLOR(bg_block_light, 35, 35, 35);
		MAKE_COLOR(bg_odd, 20, 20, 20);
		MAKE_COLOR(bg_even, 24, 24, 24);
		MAKE_COLOR(outline, 56, 56, 56);
		MAKE_COLOR(outline_light, 96, 96, 96);
		MAKE_COLOR(warning, 245, 236, 66);
		MAKE_COLOR(danger, 245, 66, 66);

		struct
		{
			MAKE_COLOR(gray_darkest, 22, 22, 22);
			MAKE_COLOR(gray_darker, 24, 24, 24);
			MAKE_COLOR(gray_dark, 26, 26, 26);
			MAKE_COLOR(gray, 29, 29, 29);
			MAKE_COLOR(gray_light, 32, 32, 32);
		} base;

		struct
		{
			MAKE_COLOR(disabled, 130, 130, 130);
			MAKE_COLOR(hover, 170, 170, 170);
			MAKE_COLOR(enabled, 241, 241, 241);
		} texts;

		struct
		{
			MAKE_COLOR(outline_dark, 19, 19, 19);
			MAKE_COLOR(outline, 22, 22, 22);
			MAKE_COLOR(outline_light, 34, 34, 34);
		} outlines;

		struct
		{
			MAKE_COLOR(accent, 0, 172, 245);
			MAKE_COLOR(accent_dark, 0, 116, 166);
			MAKE_COLOR(accent_darker, 0, 110, 156);
			MAKE_COLOR(positive, 119, 246, 74);
			MAKE_COLOR(negative, 246, 74, 74);
			MAKE_COLOR(warning, 246, 189, 74);
		} accents;
	};

	enum blur_quality
	{
		blur_full,
		blur_half,
		blur_none,
		blur_max,
	};

	class context : public container
	{
	public:
		explicit context(float text_size = 16.f);

		// refreshes the input
		bool refresh(UINT msg, WPARAM w, LPARAM l);

		// renders controls
		void render();

		// resets control's state
		void reset();

		// tracks animation to auto-update when accent color is changed
		void track_accent_anim(const std::weak_ptr<ren::anim_color> &a);
		void track_accent_anim(const std::weak_ptr<anim_float_color> &a);

		void remove(const std::shared_ptr<control> &c) override;
		void remove(uint64_t id) override;

		void do_tick_sound()
		{
			if (tick_sound_callback)
				tick_sound_callback();
		}

		void add_chord(const chord &ch);

		char content_layer{l_back}; // current layer for controls
		char scrollbar_layer{l_mid}; // current layer for scroll bars

		std::shared_ptr<control> active{}; // current active element
		std::vector<std::shared_ptr<popup>> active_popups{}; // current active popups
		int top_sort{}; // current top most sort

		std::function<void()> tick_sound_callback{};
		std::unordered_map<uint64_t, std::weak_ptr<control>> hotkey_list{}; // used to process hotkeys separately

		[[nodiscard]] uint64_t get_last_hovered() const { return hover_stack.empty() ? 0ull : hover_stack.top(); }

		std::stack<uint64_t> hover_stack{};
		value_param<bool> show_ids{};
		value_param<bits> blur_quality{1 << blur_none};
		bool had_scroll_this_tick{};

		struct resources_t
		{
			struct
			{
				std::shared_ptr<ren::texture> pattern{};
				std::shared_ptr<ren::texture> logo_head{};
				std::shared_ptr<ren::texture> logo_stripes{};
				std::shared_ptr<ren::texture> pattern_group{};
				std::shared_ptr<ren::texture> loader{};
				std::shared_ptr<ren::texture> cursor{};
			} general;

			struct
			{
				std::shared_ptr<ren::texture> notify_clear{};
				std::shared_ptr<ren::texture> remove{};
				std::shared_ptr<ren::texture> add{};
				std::shared_ptr<ren::texture> up{};
				std::shared_ptr<ren::texture> down{};
				std::shared_ptr<ren::texture> copy{};
				std::shared_ptr<ren::texture> paste{};
				std::shared_ptr<ren::texture> alert{};
				std::shared_ptr<ren::texture> search{};
				std::shared_ptr<ren::texture> bug{};
				std::shared_ptr<ren::texture> pfp_hover{};
			} icons;

			struct
			{
				std::shared_ptr<ren::font_base> debug{};
				std::shared_ptr<ren::font_base> main{};
				std::shared_ptr<ren::font_base> bold{};
			} fonts;

			struct
			{
				std::shared_ptr<ren::shader> blur{};
			} shaders;
		} res;

		struct user_t
		{
			std::shared_ptr<ren::texture> avatar{};
			std::string username{};
			std::string active_until{};
		} user;

		bool should_render_cursor{true};
		bool should_process_hotkeys{true};

	private:
		std::mutex accents_mutex{};

		std::vector<std::weak_ptr<ren::anim_color>> accents{};
		std::vector<std::weak_ptr<anim_float_color>> accents_float{};
		ren::color accent_prev{};

		std::vector<chord_info> chords;

		void process_hotkeys();
		void process_chords();
	};

	class context_debug
	{
	public:
		// renders basic debug info
		void render();

		// renders control's info
		void render(const std::shared_ptr<control> &c);

		ren::color fill = ren::color(239, 9, 95);
		bool enabled = false;
	};

	inline context_input input{}; // input processor
	inline context_debug debug{}; // debug overlay
	inline color_list colors{}; // color list
	inline notification_system notify{}; // notifications

	inline std::unique_ptr<context> ctx{}; // main context
} // namespace evo::gui

#endif // GUI_GUI_H
