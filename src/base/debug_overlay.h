
#ifndef BASE_DEBUG_OVERLAY_H
#define BASE_DEBUG_OVERLAY_H

#include <base/game.h>
#include <gui_legacy/gui.h>
#include <sdk/cs_player.h>
#include <sdk/global_vars_base.h>
#include <unordered_map>
#include <variant>

template <class... Ts> struct overloaded : Ts...
{
	using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

using debug_variant = std::variant<float, uint32_t, int32_t, sdk::vec3>;

class debug_interface : public gui_legacy::drawable
{
public:
	virtual ~debug_interface() = default;
	virtual void draw(const gui_legacy::draw_adapter &draw) override{};
	virtual void draw_player(sdk::cs_player_t *const player, const sdk::mat3x4 *mat,
							 sdk::color clr = sdk::color(0, 255, 0, 130),
							 float duration = game->globals->interval_per_tick,
							 std::optional<int32_t> only_hitbox = std::nullopt) const;
	virtual void add_value_to_observation(const std::string description, const debug_variant &value){};
};

class debug_overlay : public debug_interface
{
public:
	void draw(const gui_legacy::draw_adapter &draw) final;
	void add_value_to_observation(const std::string description, const debug_variant &value) final;

private:
	void draw_observation_list(const gui_legacy::draw_adapter &draw) const;

	std::unordered_map<std::string, debug_variant> values_under_observation;
};

template <typename... A> inline void print_to_console(const char *format, A... args)
{
	char out[0x1000];
	sprintf_s(out, sizeof(out), format, args...);
	auto cl = sdk::color::white();
	game->cvar->print(cl, out);
}

template <typename... A> inline void print_to_console(sdk::color clr, const char *format, A... args)
{
	char out[0x1000];
	sprintf_s(out, sizeof(out), format, args...);
	auto cl = clr;
	game->cvar->print(cl, out);
}

extern std::shared_ptr<debug_interface> debug;

#define OBSERVE_VALUE(desc, val) debug->add_value_to_observation(XOR_STR(desc), val)

#endif // BASE_DEBUG_OVERLAY_H
