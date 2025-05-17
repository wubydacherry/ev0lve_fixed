
#ifndef BASE_HOOK_MANAGER_H
#define BASE_HOOK_MANAGER_H

#include <hooks/hooks.h>
#include <util/hook.h>

struct hook_manager_t
{
	hook_manager_t() = default;
	NO_COPY_OR_MOVE(hook_manager_t);

	void init();
	void attach();
	void detach();

	/* steam */
	std::shared_ptr<util::hook<decltype(hooks::steam::present)>> present;
	std::shared_ptr<util::hook<decltype(hooks::steam::reset)>> reset;

	/* mat_system_surface */
	std::shared_ptr<util::hook<decltype(hooks::mat_system_surface::lock_cursor)>> lock_cursor;

	/* inputsystem */
	std::shared_ptr<util::hook<decltype(hooks::inputsystem::wnd_proc)>> wnd_proc;

	/* hl_client */
	std::shared_ptr<util::hook<decltype(hooks::hl_client::frame_stage_notify)>> frame_stage_notify;
	std::shared_ptr<util::hook<decltype(hooks::hl_client::create_move)>> create_move;
	std::shared_ptr<util::hook<decltype(hooks::hl_client::level_init_pre_entity)>> level_init_pre_entity;
	std::shared_ptr<util::hook<decltype(hooks::hl_client::level_shutdown)>> level_shutdown;

	/* engine_client */
	std::shared_ptr<util::hook<decltype(hooks::engine_client::get_aspect_ratio)>> get_aspect_ratio;
	std::shared_ptr<util::hook<decltype(hooks::engine_client::cl_move)>> cl_move;
	std::shared_ptr<util::hook<decltype(hooks::engine_client::is_connected)>> is_connected;

	/* prediction */
	std::shared_ptr<util::hook<decltype(hooks::prediction::run_command)>> run_command;
	std::shared_ptr<util::hook<decltype(hooks::prediction::perform_prediction)>> perform_prediction;

	/* game_movement */
	std::shared_ptr<util::hook<decltype(hooks::game_movement::process_movement)>> process_movement;

	/* client_state */
	std::shared_ptr<util::hook<decltype(hooks::client_state::packet_start)>> packet_start;
	std::shared_ptr<util::hook<decltype(hooks::client_state::send_netmsg)>> send_netmsg;
	std::shared_ptr<util::hook<decltype(hooks::client_state::send_datagram)>> send_datagram;
	std::shared_ptr<util::hook<decltype(hooks::client_state::send_server_cmd_key_values)>> send_server_cmd_key_values;
	std::shared_ptr<util::hook<decltype(hooks::client_state::svc_msg_voicedata)>> svc_msg_voicedata;

	/* client_mode */
	std::shared_ptr<util::hook<decltype(hooks::client_mode::override_view)>> override_view;
	std::shared_ptr<util::hook<decltype(hooks::client_mode::do_post_screen_space_effects)>>
		do_post_screen_space_effects;

	/* leaf_system */
	std::shared_ptr<util::hook<decltype(hooks::leaf_system::calc_renderable_world_space_aabb_bloated)>>
		calc_renderable_world_space_aabb_bloated;
	std::shared_ptr<util::hook<decltype(hooks::leaf_system::add_renderables_to_render_list)>>
		add_renderables_to_render_list;
	std::shared_ptr<util::hook<decltype(hooks::leaf_system::extract_occluded_renderables)>>
		extract_occluded_renderables;

	/* model_render */
	std::shared_ptr<util::hook<decltype(hooks::model_render::draw_model_execute)>> draw_model_execute;

	/* entity */
	std::shared_ptr<util::hook<decltype(hooks::entity::on_latch_interpolated_variables)>>
		on_latch_interpolated_variables;
	std::shared_ptr<util::hook<decltype(hooks::entity::do_animation_events)>> do_animation_events;
	std::shared_ptr<util::hook<decltype(hooks::entity::maintain_sequence_transitions)>> maintain_sequence_transitions;
	std::shared_ptr<util::hook<decltype(hooks::entity::set_abs_angles)>> set_abs_angles;
	std::shared_ptr<util::hook<decltype(hooks::entity::estimate_abs_velocity)>> estimate_abs_velocity;
	std::shared_ptr<util::hook<decltype(hooks::entity::setup_bones)>> setup_bones;
	std::shared_ptr<util::hook<decltype(hooks::entity::invalidate_physics_recursive)>> invalidate_physics_recursive;
	std::shared_ptr<util::hook<decltype(hooks::entity::lookup_sequence)>> lookup_sequence;
	std::shared_ptr<util::hook<decltype(hooks::entity::notify_should_transmit)>> notify_should_transmit;
	std::shared_ptr<util::hook<decltype(hooks::entity::update_relevant_interpolated_vars)>>
		update_relevant_interpolated_vars;
	std::shared_ptr<util::hook<decltype(hooks::entity::invalidate_mdl_cache)>> invalidate_mdl_cache;

	/* cs_player */
	std::shared_ptr<util::hook<decltype(hooks::cs_player::physics_simulate)>> physics_simulate;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::post_data_update)>> post_data_update;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::do_animation_events)>> do_animation_events_overlay;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::calc_view)>> calc_view;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::fire_event)>> fire_event;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::obb_change_callback)>> obb_change_callback;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::reevauluate_anim_lod)>> reevauluate_anim_lod;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::get_fov)>> get_fov;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::get_default_fov)>> get_default_fov;
	std::shared_ptr<util::hook<decltype(hooks::cs_player::update_clientside_animation)>> update_clientside_animation;
#ifdef CSGO_DEBUG_MODIFY_EYE_POSITION
	std::shared_ptr<util::hook<decltype(hooks::cs_player::modify_eye_position_server)>> modify_eye_position_server;
#endif
#ifndef NDEBUG
	std::shared_ptr<util::hook<decltype(hooks::cs_player::run_command_server)>> run_command_server;
#endif

	/* weapon */
	std::shared_ptr<util::hook<decltype(hooks::weapon::send_weapon_anim)>> send_weapon_anim;
	std::shared_ptr<util::hook<decltype(hooks::weapon::deploy)>> deploy;
	std::shared_ptr<util::hook<decltype(hooks::weapon::get_weapon_type)>> get_weapon_type;

	/* cs_grenade_projectile */
	std::shared_ptr<util::hook<decltype(hooks::cs_grenade_projectile::client_think)>> client_think;
	std::shared_ptr<util::hook<decltype(hooks::cs_grenade_projectile::post_data_update)>> projectile_post_data_update;

	/* trace_filter_for_player_head_collision */
	std::shared_ptr<util::hook<decltype(hooks::trace_filter_for_player_head_collision::should_hit_entity)>>
		should_hit_entity;

	/* recv_proxies */
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_sequence)>> layer_sequence;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_cycle)>> layer_cycle;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_playback_rate)>> layer_playback_rate;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_weight)>> layer_weight;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_weight_delta_rate)>> layer_weight_delta_rate;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::layer_order)>> layer_order;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::general)>> general;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::general_int)>> general_int;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::general_vec)>> general_vec;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::int_to_ehandle)>> int_to_ehandle;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::weapon_handle)>> weapon_handle;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::viewmodel_sequence)>> viewmodel_sequence;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::simulation_time)>> simulation_time;
	std::shared_ptr<util::hook<decltype(hooks::recv_proxies::effects)>> effects;

	/* miscellaneous */
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::get_module_handle_ex_a)>> get_module_handle_ex_a;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::host_shutdown)>> host_shutdown;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::hud_draw_scope)>> hud_draw_scope;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::particle_draw_model)>> particle_draw_model;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::ent_info_list_link_before)>> ent_info_list_link_before;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::process_input)>> process_input;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::start_sound_immediate)>> start_sound_immediate;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::check_param)>> check_param;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::move_helper_start_sound)>> move_helper_start_sound;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::get_tonemap_settings_from_env_tonemap_controller)>>
		get_tonemap_settings_from_env_tonemap_controller;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::draw_mesh)>> draw_mesh;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::draw_3d_debug_overlays)>> draw_3d_debug_overlays;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::render_glow_boxes)>> render_glow_boxes;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::spawn_smoke_effect)>> spawn_smoke_effect;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::render_iron_sight_scope_effect)>>
		render_iron_sight_scope_effect;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::convar_network_change_callback)>>
		convar_network_change_callback;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::tefirebullets_post_data_update)>>
		tefirebullets_post_data_update;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::enforce_competitive_cvar)>> enforce_competitive_cvar;
	std::shared_ptr<util::hook<decltype(hooks::miscellaneous::nt_query_virtual_memory)>> nt_query_virtual_memory;

private:
	/* keeping track of all hooks here */
	std::list<std::shared_ptr<util::hook_interface>> hooks;

	/* create the entry for a new type of hook */
	template <typename T> void create_hook(std::shared_ptr<util::hook<T>> &entry, uintptr_t original, T *fn)
	{
		// Log the original address and endpoint function
		printf("Creating hook for target: 0x%p, endpoint: 0x%p\n", (void*)original, (void*)fn);

		try {
			auto ptr = std::make_shared<util::hook<T>>(util::encrypted_ptr<T>((T*)original), util::encrypted_ptr(fn));
			entry = ptr;
			hooks.push_back(ptr);

			printf("Hooked %s at 0x%p\n", typeid(T).name(), (void*)original);
		}
		catch (const std::exception& e) {
			printf("Failed to hook %s: %s\n", typeid(T).name(), e.what());
		}
	}
};

extern hook_manager_t hook_manager;

#endif // BASE_HOOK_MANAGER_H
