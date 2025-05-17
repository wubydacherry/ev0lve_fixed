#include <base/game.h>
#include <detail/entity_rendering.h>
#include <gui/misc.h>
#include <hacks/chams.h>
#include <sdk/cutlvector.h>
#include <sdk/mdl_cache.h>
#include <sdk/misc.h>
#include <sdk/render_view.h>

namespace detail
{
using namespace hacks;
entity_rendering ent_ren;

namespace cs_player
{
int __fastcall index(sdk::cs_player_t *player, void *edx) { return 1337; }

bool __fastcall is_alive(sdk::cs_player_t *player, void *edx) { return true; }

sdk::model *__fastcall get_model(sdk::cs_player_t *player, void *edx) { return ent_ren.render_model; }

void *__fastcall get_cs_active_weapon(sdk::cs_player_t *player, void *edx) { return nullptr; }

sdk::vec3 pos{};
sdk::vec3 *__fastcall get_abs_origin(sdk::cs_player_t *player, void *edx) { return &pos; }

sdk::vec3 *__fastcall eye_angles(sdk::cs_player_t *player, void *edx) { return &pos; }

void __fastcall set_sequence(sdk::cs_player_t *player, void *edx, int sequence)
{
	// abort if sequence was already set.
	if (player->get_sequence() == sequence) // 39 9E [ ? ? ? ? ] 74 7F
		return;

	player->get_sequence() = sequence;
	player->get_sequence_loops() = false;
	player->get_ground_speed() = 0.f;
}

void __fastcall estimate_abs_velocity(sdk::cs_player_t *player, void *edx, void *a1)
{
	// stub.
}

float __fastcall get_layer_sequence_cycle_rate(sdk::cs_player_t *player, void *edx, sdk::animation_layer *layer,
											   int sequence)
{
	return 1.f;
}
} // namespace cs_player

void entity_rendering::draw()
{
	// abort if something did not create for whatever reason.
	if (!render_target || !fake_player || !fake_anim_state)
		return;

	// clamp yaw.
	while (yaw > 180.f)
		yaw -= 360.f;
	while (yaw < -180.f)
		yaw += 360.f;

	// update animstate like 64 tickrate server.
	if (game->globals->realtime - last_animstate_update >= animstate_tickrate)
	{
		const auto old_curtime = game->globals->curtime;
		game->globals->curtime = last_animstate_update;

		is_updating_animstate = true;
		fake_cmd.command_number = game->globals->framecount;
		fake_player_record.update_animations(&fake_cmd);

		// check if alive loop is set up.
		if (fake_player->get_sequence_activity((int)fake_player->get_animation_layers()->at(11).sequence) !=
			XOR_32(981))
		{
			std::array<uint16_t, 3> mods;
			player_list.build_activity_modifiers(fake_player, mods);

			fake_player->try_initiate_animation(11, XOR_32(981), mods, game->globals->framecount);
		}

		is_updating_animstate = false;
		last_animstate_update = game->globals->realtime;
		game->globals->curtime = old_curtime;
	}

	// initialize view setup.
	view_setup.x = 0.f;
	view_setup.y = 0.f;
	view_setup.width = render_target->get_actual_width();
	view_setup.height = render_target->get_actual_height();
	view_setup.ortho = false;
	view_setup.fov = 12.f;
	view_setup.origin = {-329.f, 2.5f, 42.f};
	view_setup.angles = {2.f, 0.f, 0.f};
	view_setup.z_near = 20.f;
	view_setup.z_far = 1000.f;
	view_setup.do_bloom_and_tone_mapping = true;

	// begin render.
	sdk::vmatrix v1{}, v2{}, v3{};
	game->render_view->get_matrices_for_view(view_setup, &v1, &v2, &world_to_projection, &v3);

	const auto ctx = game->material_system()->get_render_context();

	// off we go.
	ctx->begin_render();
	ctx->push_render_target_and_viewport();
	ctx->set_render_target(render_target);
	ctx->bind_local_cubemap(cubemap);
	ctx->set_lighting_origin({0.f, 0.f, 100.f});
	ctx->set_int_rendering_parameter(10, 0);

	sdk::frustum_t frustum;
	game->render_view->push_3d_view(ctx, view_setup,
									sdk::view_clear_color | sdk::view_clear_depth | sdk::view_clear_stencil,
									render_target, frustum);

	ctx->clear_color_4_ub(0, 0, 0, 0);
	ctx->clear_buffers(true, true, true);

	sdk::vec4 col[6] = {sdk::vec4(1.f, 1.f, 1.f, 1.f), sdk::vec4(1.f, 1.f, 1.f, 1.f), sdk::vec4(1.f, 1.f, 1.f, 1.f),
						sdk::vec4(1.f, 1.f, 1.f, 1.f), sdk::vec4(1.f, 1.f, 1.f, 1.f), sdk::vec4(1.f, 1.f, 1.f, 1.f)};

	ctx->set_ambient_light_cube(col);

	game->studio_render->set_ambient_light_colors(col);
	game->studio_render->set_local_lights(0, nullptr);
	game->model_render->suppress_engine_lighting(true);

	// select winner sequence.
	sdk::animation_layer winner{};
	for (const auto &layer : *fake_player->get_animation_layers())
	{
		if (layer.weight >= winner.weight)
			winner = layer;
	}

	// troll the model.
	auto &root_mdl = fake_player_model->root.mdl;
	root_mdl.body = fake_player->get_animating_body();
	root_mdl.sequence = (int)winner.sequence > 0 ? (int)winner.sequence : 113;
	root_mdl.playback_rate = winner.playback_rate;
	root_mdl.time = game->globals->realtime;
	memcpy(fake_player_model->pose_parameters, fake_player->get_pose_parameter().data(), sizeof(float) * 24);

	auto &att_mdl = fake_player_model->merge[0].mdl;

	// TODO: render glow.

	// figure out stuff.
	cfg_t::chams *ch_inv{}, *ch_vis{}, *ch_att_inv{}, *ch_att_vis{};
	switch (current_preview_id)
	{
	case GUI_HASH("visuals.enemy.preview.preview"):
		ch_inv = &cfg.player_esp.enemy_chams;
		ch_vis = &cfg.player_esp.enemy_chams_visible;
		ch_att_inv = &cfg.player_esp.enemy_attachment_chams;
		ch_att_vis = &cfg.player_esp.enemy_attachment_chams_visible;
		break;
	case GUI_HASH("visuals.team.preview.preview"):
		ch_inv = &cfg.player_esp.team_chams;
		ch_vis = &cfg.player_esp.team_chams_visible;
		ch_att_inv = &cfg.player_esp.team_attachment_chams;
		ch_att_vis = &cfg.player_esp.team_attachment_chams_visible;
		break;
	case GUI_HASH("visuals.local.preview.preview"):
		ch_vis = &cfg.local_visuals.local_chams;
		ch_att_vis = &cfg.local_visuals.local_attachment_chams;
		break;
	}

	// draw the model.
	sdk::material *override{}, *override_att{};
	if (ch_inv && !ch_inv->mode->test(cfg_t::chams_disabled))
		override = cham.maintain_material(*ch_inv, cham.index_material(*ch_inv));
	if (ch_vis && !ch_vis->mode->test(cfg_t::chams_disabled))
		override = cham.maintain_material(*ch_vis, cham.index_material(*ch_vis));
	if (ch_att_inv && !ch_att_inv->mode->test(cfg_t::chams_disabled))
		override_att = cham.maintain_material(*ch_att_inv, cham.index_material(*ch_att_inv));
	if (ch_att_vis && !ch_att_vis->mode->test(cfg_t::chams_disabled))
		override_att = cham.maintain_material(*ch_att_vis, cham.index_material(*ch_att_vis));

	root_mdl.override = override;
	att_mdl.override = override_att;
	fake_player_model->draw(sdk::angle_matrix({0.f, yaw, 90.f}));

	// stop rendering.
	game->model_render->suppress_engine_lighting(false);
	game->render_view->pop_view(ctx, frustum);

	ctx->bind_local_cubemap(nullptr);
	ctx->pop_render_target_and_viewport();
	ctx->end_render();
}

void entity_rendering::on_exit()
{
	// DO NOT REMOVE THIS RETURN BEFORE USING YOUR BRAIN
	return;
	game->mdl_cache->begin_lock();

	// free render target.
	if (render_target)
	{
		render_target->decrement_reference_count();
		render_target = nullptr;
	}

	const auto model_handle = game->model_info_client->get_cache_handle(render_model);
	game->mdl_cache->unlock_studio_hdr(model_handle);

	game->mem_alloc->free(fake_player_model);
	game->mem_alloc->free(fake_player);
	game->mem_alloc->free(fake_player_hdr);
	game->mem_alloc->free(fake_anim_state);
	game->mem_alloc->free(fake_renderable_vtable);
	game->mem_alloc->free(fake_networkable_vtable);
	game->mem_alloc->free(fake_player_vtable);
	game->mem_alloc->free(fake_layers);
	game->mdl_cache->end_lock();
}

void entity_rendering::init()
{
	// DO NOT REMOVE THIS RETURN BEFORE USING YOUR BRAIN
	return;
	game->mdl_cache->begin_lock();
	handle_render_target();
	handle_fake_player();
	game->mdl_cache->end_lock();
}

void entity_rendering::handle_render_target()
{
	if (render_target)
		return;

	// clear viewsetup.
	memset(&view_setup, 0, sizeof(view_setup));

	// enter allocation block.
	auto old = game->material_system->get_disable_render_target_allocation_forever();
	game->material_system->get_disable_render_target_allocation_forever() = false;
	game->material_system->begin_render_target_allocation();
	game->material_system->get_disable_render_target_allocation_forever() = old;

	// create render target.
	render_target = game->material_system->create_named_render_target_texture_ex(
		std::to_string(sdk::random_int(INT_MIN, INT_MAX)).c_str(), 235, 370, sdk::rt_size_no_change,
		game->material_system->get_back_buffer_format(), sdk::mat_rt_depth_shared,
		sdk::textureflags_clamps | sdk::textureflags_clampt, 1);
	render_target->increment_reference_count();

	// exit allocation block.
	old = game->material_system->get_disable_render_target_allocation_forever();
	game->material_system->get_disable_render_target_allocation_forever() = false;
	game->material_system->end_render_target_allocation();
	game->material_system->get_disable_render_target_allocation_forever() = old;
	game->material_system->finish_render_target_allocation();

	// get cubemap.
	cubemap = game->material_system->find_texture(XOR_STR("editor/cubemap.hdr"), XOR_STR("CubeMap textures"));
}

void entity_rendering::handle_fake_player()
{
	if (fake_player && fake_anim_state)
		return;

	// mock player.
	fake_player = (sdk::cs_player_t *)game->mem_alloc->alloc(0x11D23); // new char[0x11D23];
	memset((void *)fake_player, 0, 0x11D23);

	// create player model.
	fake_player_hdr = (sdk::cstudiohdr *)game->mem_alloc->alloc(0x8C); // new char[0x8C];
	memset((void *)fake_player_hdr, 0, 0x8C);

	XOR_STR_STACK(player_mdl_str, player_mdl);
	XOR_STR_STACK(weapon_mdl_str, weapon_mdl);

	// init hdr.
	render_model = game->model_info_client->find_or_load_model(player_mdl_str);
	const auto model_handle = game->model_info_client->get_cache_handle(render_model);
	fake_player_hdr->init((sdk::studiohdr *)game->mdl_cache->lock_studio_hdr(model_handle), game->mdl_cache());
	fake_player_hdr->init_softbody(game->client.at(sdk::globals::softbody_environment));

	// init layers.
	fake_layers = (sdk::animation_layer *)game->mem_alloc->alloc(sizeof(sdk::animation_layer) * 14);
	memset((void *)fake_layers, 0, sizeof(sdk::animation_layer) * 14);

	// init merged model.
	fake_player_model = (sdk::merged_mdl_t *)game->mem_alloc->alloc(0x75C); // new char[0x75C];
	memset((void *)fake_player_model, 0, 0x75C);
	fake_player_model->init();
	fake_player_model->set_mdl(player_mdl_str, nullptr, nullptr);
	fake_player_model->set_merge_mdl(weapon_mdl_str);
	fake_player_model->setup_bones_for_attachment_queries();

	// set some garbage.
	fake_player->get_is_player_ghost() = true;
	fake_player->get_use_new_animstate() = true;
	*(sdk::model **)(uintptr_t(fake_player) + 0x68) = render_model;
	fake_player->get_studio_hdr() = fake_player_hdr;
	*(sdk::animation_layer **)(uintptr_t(fake_player) + sdk::offsets::csplayer::animation_layers) = fake_layers;
	*(int *)(uintptr_t(fake_player) + sdk::offsets::csplayer::animation_layers + 4) = 14;
	*(int *)(uintptr_t(fake_player) + sdk::offsets::csplayer::animation_layers + 8) = 14;
	*(int *)(uintptr_t(fake_player) + sdk::offsets::csplayer::animation_layers + 12) = 14;
	*(sdk::animation_layer **)(uintptr_t(fake_player) + sdk::offsets::csplayer::animation_layers + 16) = fake_layers;
	fake_player->get_model_index() = 1337;

	// create fake vtable.
	fake_player_vtable = (uintptr_t *)game->mem_alloc->alloc(sizeof(uintptr_t) *
															 player_vtable_size); // new uintptr_t[player_vtable_size];
	memset((void *)fake_player_vtable, 0, player_vtable_size * sizeof(uintptr_t));

	// fill in vtable.
	fake_player_vtable[2] = (uintptr_t)cs_player::index;
	fake_player_vtable[10] = (uintptr_t)cs_player::get_abs_origin;
	fake_player_vtable[145] = (uintptr_t)cs_player::estimate_abs_velocity;
	fake_player_vtable[156] = (uintptr_t)cs_player::is_alive;
	fake_player_vtable[170] = (uintptr_t)cs_player::eye_angles;
	fake_player_vtable[219] = (uintptr_t)cs_player::set_sequence; // correct 19/4/2024 (FF 90 ?? ?? ?? ?? 8B 47 60 C7 80 18 0A 00 00 00 00 00 00) 
	fake_player_vtable[223] = (uintptr_t)cs_player::get_layer_sequence_cycle_rate;
	fake_player_vtable[268] = (uintptr_t)cs_player::get_cs_active_weapon;

	// create fake renderable vtable.
	fake_renderable_vtable = (uintptr_t *)game->mem_alloc->alloc(
		sizeof(uintptr_t) * renderable_vtable_size); // new uintptr_t[renderable_vtable_size];
	memset((void *)fake_renderable_vtable, 0, renderable_vtable_size * sizeof(uintptr_t));

	// fill this one too.
	fake_renderable_vtable[8] = (uintptr_t)cs_player::get_model;

	// create fake networkable vtable.
	fake_networkable_vtable = (uintptr_t *)game->mem_alloc->alloc(
		sizeof(uintptr_t) * networkable_vtable_size); // new uintptr_t[networkable_vtable_size];
	memset((void *)fake_networkable_vtable, 0, networkable_vtable_size * sizeof(uintptr_t));

	// fill this one too.
	fake_networkable_vtable[10] = (uintptr_t)cs_player::index;

	// assign vtable.
	*(uintptr_t **)fake_player = fake_player_vtable;
	*(uintptr_t **)(uintptr_t(fake_player) + 4) = fake_renderable_vtable;
	*(uintptr_t **)(uintptr_t(fake_player) + 8) = fake_networkable_vtable;

	// mock anim state.
	fake_anim_state = (sdk::anim_state *)game->mem_alloc->alloc(sizeof(sdk::anim_state)); // new sdk::anim_state;
	memset((void *)fake_anim_state, 0, sizeof(sdk::anim_state));
	fake_anim_state->create(fake_player);

	// set more garbage.
	fake_player_record.player = fake_player;
	*(sdk::anim_state **)(uintptr_t(fake_player) + sdk::offsets::csplayer::anim_state) = fake_anim_state;
}
} // namespace detail
