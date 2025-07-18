#ifndef GUI_TESTER_CONTROL_H
#define GUI_TESTER_CONTROL_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <ren/types/animator.h>
#include <ren/types/pos.h>

namespace evo::gui
{
// draw parameters for the on_draw_param_changed callback
enum draw_parameter : char
{
	dp_visibility,
	dp_position,
	dp_size,
	dp_margin,
	dp_text
};

// input priority for the refresh
enum input_priority : char
{
	ip_least,
	ip_window,
	ip_control,
	ip_popup,
	ip_most
};

enum hotkey_behavior : char
{
	hkb_toggle,
	hkb_hold
};

enum hotkey_type : char
{
	hkt_none,
	hkt_checkbox,
	hkt_dropdown, // TODO: implement this
	hkt_slider	  // TODO: implement this
};

enum hotkey_update : char
{
	hku_click,
	hku_release
};

enum control_type : char
{
	ctrl_default,
	ctrl_button,
	ctrl_checkbox,
	ctrl_child_tab,
	ctrl_color_picker,
	ctrl_combo_box,
	ctrl_control_container,
	ctrl_group,
	ctrl_hotkey,
	ctrl_hsv_slider,
	ctrl_label,
	ctrl_layout,
	ctrl_list,
	ctrl_loading,
	ctrl_notification_control,
	ctrl_popup,
	ctrl_selectable,
	ctrl_slider,
	ctrl_spacer,
	ctrl_tab,
	ctrl_tabs_layout,
	ctrl_text_input,
	ctrl_toggle_button,
	ctrl_window,
	ctrl_widget,
};

struct control_id
{
	uint64_t id;
	std::string id_string;
};

// control base class
class control : public std::enable_shared_from_this<control>
{
public:
	control(control_id _id) : id(_id.id), id_string(_id.id_string)
	{
		highlight_alpha = std::make_shared<ren::anim_float>(0.f, 2.f, ren::ease_out);
	}

	/**
	 * control constructor
	 * @param _id ID
	 * @param _pos Position
	 * @param _size Size
	 */
	control(control_id _id, const ren::vec2 &_pos, const ren::vec2 &_size)
		: id(_id.id), id_string(_id.id_string), pos(_pos), size(_size)
	{
		highlight_alpha = std::make_shared<ren::anim_float>(0.f, 2.f, ren::ease_out);
	}

	// called on input refresh
	virtual void refresh();

	// called on hotkey refresh
	virtual void refresh_hotkeys();

	// called on render
	virtual void render();

	// called by end user (ex. reset state after config update)
	virtual void reset() {}

	// called before first time render has occured
	virtual void on_first_render_call() {}

	// mouse has been moved
	virtual void on_mouse_move(const ren::vec2 &p, const ren::vec2 &d) {}

	// mouse entered the control
	virtual void on_mouse_enter() {}

	// mouse left the control
	virtual void on_mouse_leave() {}

	// mouse button state changed to down
	virtual void on_mouse_down(char key) {}

	// mouse button state changed to up
	virtual void on_mouse_up(char key) {}

	// mouse wheel has been scrolled (factor is -1 or 1, depending on the scroll direction)
	virtual void on_mouse_wheel(float factor) {}

	// keyboard key state changed to down
	virtual void on_key_down(uint16_t key) {}

	// keyboard key state changed to up
	virtual void on_key_up(uint16_t key) {}

	// input text buffer update
	virtual void on_text_input(const std::vector<uint32_t> &text) {}

	// draw parameter changed
	virtual void on_draw_param_changed(char w) {}

	// when hotkey table updates, control should update their setting
	virtual void update_hotkey_table();

	// if hotkey has been toggled, ask control to update its value
	virtual void update_hotkey_value(hotkey_update upd, uint32_t v) {}

	// raises first render call callback
	void do_first_render_call();

	// forces width to stick to parent
	std::shared_ptr<control> make_width_automatic();

	// forces height to stick to parent
	std::shared_ptr<control> make_height_automatic();

	// hides control
	std::shared_ptr<control> make_invisible();

	// changes control margin
	std::shared_ptr<control> adjust_margin(const ren::rect &m);

	// sets default tooltip
	std::shared_ptr<control> make_tooltip(const std::string &s);

	// returns absolute position on screen
	ren::vec2 pos_abs() const;

	// returns relative rect (m decides whenether to account boittom right margin)
	ren::rect area(bool m = false) const;

	// returns absolute rect (m decides whenether to account boittom right margin)
	ren::rect area_abs(bool m = false) const;

	// checks if control is out of provided rect
	bool is_out_of_rect(const ren::rect &r) const;

	// find a window this control is placed in
	std::shared_ptr<control> find_associated_window();

	template <typename T> std::shared_ptr<T> as() { return std::static_pointer_cast<T>(shared_from_this()); }

	// setters
	void set_visible(bool v)
	{
		is_visible = v;
		process_draw_param_changes(dp_visibility);
	}

	void set_pos(const ren::vec2 &p)
	{
		pos = p;
		process_draw_param_changes(dp_position);
	}

	void set_size(const ren::vec2 &s)
	{
		size = s;
		process_draw_param_changes(dp_size);
	}

	void set_margin(const ren::rect &m)
	{
		margin = m;
		process_draw_param_changes(dp_margin);
	}

	void set_breadcrumb(const std::string &bc)
	{
		is_breadcrumb = true;
		breadcrumb_name = bc;
	}

	void highlight();
	std::weak_ptr<control> get_top_breadcrumb() const;
	std::vector<std::string> get_breadcrumbs() const;

	uint64_t id{};					 // control id
	std::string id_string{};		 // string representation
	std::weak_ptr<control> parent{}; // parent control
	std::weak_ptr<control> window{}; // parent window
	char priority{ip_control};		 // input priority
	int sort{};						 // sort id (mostly for windows and popups)

	bool size_to_parent_w{}; // should size to parent's width
	bool size_to_parent_h{}; // should size to parent's height
	bool is_container{};	 // is this control a container
	bool is_window{};		 // is this control a window
	bool
		is_action_popup{}; // is this control an action popup (will hide automatically if user clicked outside the rect)
	bool is_visible{true}; // should be drawn
	bool is_taking_input{true}; // should process input
	bool is_taking_keys{};		// should process keyboard input
	bool is_taking_text{};		// should process input text
	bool disable_id_display{};	// whether should disable ID display

	control_type type{ctrl_default}; // control type

	ren::vec2 pos{};	// relative position
	ren::vec2 size{};	// bounding box size
	ren::rect margin{}; // margin

	std::string tooltip{};				// tooltip, will be drawn on hover
	std::vector<std::string> aliases{}; // name aliases

	bool is_mouse_on_me{};			   // util member to see if mouse is currently on this control
	bool is_caring_about_parent{true}; // if set to false, will not check if mouse is on parent
	bool is_caring_about_hover{true};  // if set to false, will not check if currenly hovered and process input anyway

	char hotkey_mode{hkb_toggle};				 // hotkey behavior
	char hotkey_type{hkt_none};					 // hotkey (control) type (TODO: USELESS ATM)
	bool allow_multiple_keys{};					 // should allow more than 1 hotkey (TODO: USELESS ATM)
	std::unordered_map<int, uint32_t> hotkeys{}; // hotkey list (key -> value)

	std::unordered_map<uint32_t, std::vector<std::function<void()>>> universal_callbacks{};

protected:
	bool is_highlighted{};
	std::shared_ptr<ren::anim_float> highlight_alpha{};

	bool is_breadcrumb{};
	std::string breadcrumb_name{};

	// disable parent and hover checks
	void stop_caring() { is_caring_about_hover = is_caring_about_parent = false; }

	// enable parent and hover checks
	void start_caring() { is_caring_about_hover = is_caring_about_parent = true; }

	std::shared_ptr<control> get_parent() const { return parent.lock(); }

	void run_universal_callbacks();

	// locks input
	void lock_input();

	// unlocks input
	void unlock_input();

	// checks if input is locked by this control
	bool is_locked_by_me();

	// checks if input is locked by someone else
	bool is_locked_by_someone_else();

	// checks if input is locked at all
	bool is_input_locked();

	// removes invalid hotkeys from the list
	void clean_up_hotkeys();

	// checks if the point is outside global clip area relative to this control
	bool is_out_of_clip(const ren::vec2 &p, float max_offset = 25.f) const;

	// calculates perfect position for this control (used for windows & popups)
	ren::vec2 calc_perfect_position(const ren::vec2 &base) const;

	// updates on_draw_param_changed
	void process_draw_param_changes(char w);

	bool can_show_debug_info() const;

private:
	bool first_render_call{};
};
} // namespace evo::gui

#endif // GUI_TESTER_CONTROL_H
