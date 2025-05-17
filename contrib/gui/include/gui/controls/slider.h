#ifndef GUI_TESTER_SLIDER_H
#define GUI_TESTER_SLIDER_H

#include <gui/control.h>
#include <gui/gui.h>
#include <gui/values.h>

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

namespace evo::gui
{
enum slider_type : char
{
	st_float,
	st_int
};

template <typename T> class slider : public control
{
public:
	struct fmt_step
	{
		fmt_step() = default;
		fmt_step(std::string f) // NOLINT(google-explicit-constructor)
			: min(std::numeric_limits<T>::max()), fmt(std::move(f))
		{
		}
		fmt_step(T v, std::string f) : min(v), fmt(std::move(f)) {}

		T min;
		std::string fmt;
	};

	struct fmt_step_array
	{
		fmt_step_array(const char *s) : steps({fmt_step(std::string(s))}) {} // NOLINT(google-explicit-constructor)
		fmt_step_array(const fmt_step &s) : steps({s}) {}					 // NOLINT(google-explicit-constructor)
		fmt_step_array(const std::initializer_list<fmt_step> init) : steps(init) {}

		std::vector<fmt_step> steps;
	};

	slider(const control_id &id, T low, T high, value_param<T> &v, const fmt_step_array &f = "%d", float step = 1.f)
		: control(id, {}, {100.f, 8.f}), low(low), high(high), value(v), step(step), format(f)
	{
		using namespace ren;

		margin = {2.f, 2.f, 2.f, 2.f};
		is_taking_keys = true;

		anim = std::make_shared<anim_float>(0.f, 0.15f, ease_in_out);

		determine_type();
		type = ctrl_slider;
	}

	void on_mouse_down(char key) override;
	void on_mouse_move(const ren::vec2 &p, const ren::vec2 &d) override;
	void on_key_down(uint16_t key) override;
	void on_mouse_up(char key) override;
	void on_mouse_enter() override;
	void on_mouse_leave() override;
	void on_mouse_wheel(float factor) override;

	void reset() override;
	void render() override;

	T low{};
	T high{};
	value_param<T> &value;
	float step{};
	fmt_step_array format{};

	// old, new
	std::function<void(T, T)> callback{};

	// required for hotkeys
	char type{};

private:
	bool show_val{};
	std::shared_ptr<ren::anim_float> anim;

	void determine_type();

	void update_value();
	void show_value_tip();
	void hide_value_tip();

	ren::vec2 get_fill()
	{
		return ren::vec2(size.x * (((float)*value - (float)low) / ((float)high - (float)low)), size.y);
	}

	std::string get_formatted(T v)
	{
		std::string s;
		for (const auto &[m, f] : format.steps)
		{
			// account for default value.
			if (m == std::numeric_limits<T>::max())
			{
				s = f;
				break;
			}

			// stepper.
			if (m <= v)
			{
				v = v - m;
				if (m != decltype(m)())
					v += 1;
				s = f;
				break;
			}
		}

		char buffer[2048]{};
		sprintf_s(buffer, sizeof(buffer), s.c_str(), v);

		return buffer;
	}
};

template <typename T> using ssa = typename slider<T>::fmt_step_array;
template <typename T> using ssv = typename slider<T>::fmt_step;
} // namespace evo::gui

#pragma pop_macro("min")
#pragma pop_macro("max")
#endif // GUI_TESTER_SLIDER_H
