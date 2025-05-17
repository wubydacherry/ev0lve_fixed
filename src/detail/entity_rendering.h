#ifndef ENTITY_RENDERING_895B0563C8114EF980C064C0764D0256_H
#define ENTITY_RENDERING_895B0563C8114EF980C064C0764D0256_H

#include <detail/player_list.h>
#include <sdk/cs_player.h>
#include <sdk/model_render.h>

namespace detail
{
class entity_rendering
{
	static constexpr auto player_vtable_size = 397;
	static constexpr auto renderable_vtable_size = 397;
	static constexpr auto networkable_vtable_size = 397;
	static constexpr auto animstate_tickrate = 1.f / 64.f;

public:
	void init();

	void draw();
	void on_exit();

	std::atomic_bool is_updating_animstate{};

	sdk::texture *render_target{};
	sdk::model *render_model{};

	uint64_t current_preview_id{};
	float yaw{-89.f};

private:
	lag_record fake_player_record{};
	sdk::user_cmd fake_cmd{};

	sdk::view_setup view_setup{};
	sdk::texture *cubemap{};
	sdk::material *glow{};
	sdk::vmatrix world_to_projection{};

	sdk::cs_player_t *fake_player{};
	sdk::animation_layer *fake_layers{};
	sdk::anim_state *fake_anim_state{};
	sdk::cstudiohdr *fake_player_hdr{};
	sdk::merged_mdl_t *fake_player_model{};

	uintptr_t *fake_player_vtable{};
	uintptr_t *fake_renderable_vtable{};
	uintptr_t *fake_networkable_vtable{};

	std::pair<std::string, char> player_mdl = XOR_STR_STORE("models/player/custom_player/legacy/tm_phoenix.mdl");
	std::pair<std::string, char> weapon_mdl = XOR_STR_STORE("models/weapons/w_snip_ssg08.mdl");

	void handle_render_target();
	void handle_fake_player();

	float last_animstate_update{};
};

extern entity_rendering ent_ren;
} // namespace detail

#endif // ENTITY_RENDERING_895B0563C8114EF980C064C0764D0256_H
