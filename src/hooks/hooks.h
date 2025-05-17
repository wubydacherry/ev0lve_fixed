
#ifndef HOOKS_HOOKS_H
#define HOOKS_HOOKS_H

#include <d3d9.h>

// clang-format off
#include <sdk/hl_client.h>
#include <sdk/engine_client.h>
#include <sdk/prediction.h>
#include <sdk/surface.h>
#include <sdk/input.h>
#include <sdk/model_render.h>
#include <sdk/engine_sound.h>
#include <sdk/glow_manager.h>
#include <sdk/client_state.h>
// clang-format on

namespace hooks
{
namespace hl_client
{
void __fastcall frame_stage_notify(sdk::hl_client_t *client, uint32_t edx, sdk::framestage stage);
void __fastcall create_move(sdk::hl_client_t *client, uint32_t edx, int32_t sequence_number,
							float input_sample_frametime, bool active);
void __fastcall level_init_pre_entity(sdk::hl_client_t *client, uint32_t edx, const char *level_name);
void __fastcall level_shutdown(sdk::hl_client_t *client, uint32_t edx);
} // namespace hl_client

namespace engine_client
{
float __fastcall get_aspect_ratio(sdk::engine_client_t *client, uint32_t edx, int32_t width, int32_t height);
void cl_move();
bool __fastcall is_connected(sdk::engine_client_t *client, uint32_t edx);
} // namespace engine_client

namespace prediction
{
void __fastcall run_command(sdk::prediction_t *prediction, uint32_t edx, sdk::cs_player_t *player, sdk::user_cmd *cmd,
							uintptr_t helper);
bool __fastcall perform_prediction(sdk::prediction_t *prediction, uint32_t edx, int32_t slot, sdk::cs_player_t *player,
								   bool recv_update, int32_t ack, int32_t out);
} // namespace prediction

namespace game_movement
{
void __fastcall process_movement(uintptr_t movement, uint32_t edx, sdk::cs_player_t *player, sdk::move_data *data);
}

namespace client_state
{
void __fastcall packet_start(sdk::client_state_t *state, uint32_t edx, int32_t incoming_sequence,
							 int32_t outgoing_acknowledged);
bool __fastcall send_netmsg(sdk::net_channel *channel, uint32_t edx, sdk::net_message *msg, bool reliable, bool voice);
int32_t __fastcall send_datagram(sdk::net_channel *channel, uint32_t edx, uintptr_t datagram);
void __fastcall send_server_cmd_key_values(sdk::client_state_t *state, uint32_t edx, sdk::keyvalues *kv);
bool __fastcall svc_msg_voicedata(sdk::client_state_t *state, uint32_t edx, sdk::svc_msg_voicedata_t *msg);
} // namespace client_state

namespace client_mode
{
void __fastcall override_view(sdk::client_mode_t *client_mode, uint32_t edx, sdk::view_setup *setup);
void __fastcall do_post_screen_space_effects(sdk::client_mode_t *client_mode, uint32_t edx, sdk::view_setup *setup);
} // namespace client_mode

namespace leaf_system
{
void __stdcall calc_renderable_world_space_aabb_bloated(sdk::renderable_info *info, sdk::vec3 *mins, sdk::vec3 *maxs);
void __stdcall add_renderables_to_render_list(uintptr_t info, int32_t count, sdk::renderable_info **renderables,
											  uintptr_t rl_info, int32_t dcount, uintptr_t dinfo);
int32_t __stdcall extract_occluded_renderables(int32_t view_id, int32_t count, sdk::renderable_info **renderables,
											   uintptr_t rl_info);
} // namespace leaf_system

namespace model_render
{
void __fastcall draw_model_execute(uintptr_t render, uint32_t edx, sdk::mat_render_context *context,
								   sdk::draw_model_state *state, sdk::model_render_info_t *info, sdk::mat3x4 *bone);
}

namespace entity
{
void __fastcall on_latch_interpolated_variables(sdk::entity_t *ent, uint32_t ecx, int32_t flags);
void __fastcall do_animation_events(sdk::entity_t *ent, uint32_t edx, sdk::cstudiohdr *hdr);
void __fastcall maintain_sequence_transitions(sdk::entity_t *ent, uintptr_t edx, uintptr_t bone_setup, sdk::vec3 *pos,
											  sdk::quaternion *q);
void __fastcall set_abs_angles(sdk::entity_t *ent, uint32_t edx, sdk::angle *ang);
void __fastcall estimate_abs_velocity(sdk::entity_t *ent, uint32_t edx, sdk::vec3 *out);
bool __fastcall setup_bones(sdk::entity_t *ent, uint32_t edx, sdk::mat3x4 *out, int max_bones, int mask, float time);
void __fastcall invalidate_physics_recursive(sdk::entity_t *ent, uint32_t edx, int32_t change_flags);
int __fastcall lookup_sequence(sdk::entity_t *ent, uint32_t edx, const char *sequence);
void __fastcall notify_should_transmit(sdk::entity_t *ent, uint32_t edx, uint32_t state);
void __fastcall update_relevant_interpolated_vars(sdk::entity_t *ent, uint32_t edx);
void __fastcall invalidate_mdl_cache(sdk::entity_t *ent, uint32_t edx);
} // namespace entity

namespace cs_player
{
void __fastcall physics_simulate(sdk::cs_player_t *player, uint32_t edx);
void __fastcall post_data_update(sdk::cs_player_t *player, uint32_t edx, uint32_t type);
void __fastcall do_animation_events(sdk::cs_player_t *player, uint32_t edx, sdk::cstudiohdr *hdr);
void __fastcall calc_view(sdk::cs_player_t *player, uint32_t edx, sdk::vec3 *eye, sdk::angle *ang, float *znear,
						  float *zfar, float *fov);
void __fastcall fire_event(sdk::cs_player_t *player, uint32_t edx, sdk::vec3 *origin, sdk::angle *angles, int event,
						   const char *options);
void __fastcall obb_change_callback(sdk::cs_player_t *player, uint32_t edx, sdk::vec3 *old_mins, sdk::vec3 *mins,
									sdk::vec3 *old_maxs, sdk::vec3 *maxs);
void __fastcall reevauluate_anim_lod(sdk::cs_player_t *player, uint32_t edx, int32_t bone_mask);
double __fastcall get_fov(sdk::cs_player_t *player, uintptr_t edx);
int32_t __fastcall get_default_fov(sdk::cs_player_t *player, uintptr_t edx);
void __fastcall update_clientside_animation(sdk::cs_player_t *player, uint32_t edx);

#ifdef CSGO_DEBUG_MODIFY_EYE_POSITION
void __fastcall modify_eye_position_server(uintptr_t state, uint32_t edx, sdk::vec3 *eye_position);
#endif

#ifndef NDEBUG
void __fastcall run_command_server(uintptr_t move, uint32_t edx, uintptr_t player, sdk::user_cmd *cmd,
								   uintptr_t move_helper);
#endif
} // namespace cs_player

namespace weapon
{
void __fastcall send_weapon_anim(sdk::weapon_t *weapon, uint32_t edx, uint32_t act);
bool __fastcall deploy(sdk::weapon_t *weapon, uint32_t edx);
int32_t __fastcall get_weapon_type(sdk::weapon_t *wpn, uint32_t edx);
} // namespace weapon

namespace cs_grenade_projectile
{
void __fastcall client_think(sdk::weapon_t *wpn, uint32_t edx);
void __fastcall post_data_update(sdk::weapon_t *wpn, uint32_t edx, uint32_t type);
} // namespace cs_grenade_projectile

namespace trace_filter_for_player_head_collision
{
bool __fastcall should_hit_entity(uintptr_t filter, uint32_t edx, sdk::entity_t *entity, int32_t contents_mask);
}

namespace recv_proxies
{
void __cdecl layer_sequence(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl layer_cycle(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl layer_playback_rate(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl layer_weight(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl layer_weight_delta_rate(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl layer_order(sdk::recv_proxy_data *data, sdk::animation_layer *str, void *out);
void __cdecl general(sdk::recv_proxy_data *data, void *ent, void *out);
void __cdecl general_int(sdk::recv_proxy_data *data, void *ent, void *out);
void __cdecl general_vec(sdk::recv_proxy_data *data, void *ent, void *out);
void __cdecl int_to_ehandle(sdk::recv_proxy_data *data, sdk::entity_t *ent, void *out);
void __cdecl viewmodel_sequence(sdk::recv_proxy_data *data, sdk::entity_t *str, void *out);
void __cdecl weapon_handle(sdk::recv_proxy_data *data, sdk::entity_t *str, void *out);
void __cdecl simulation_time(sdk::recv_proxy_data *data, sdk::entity_t *str, void *out);
void __cdecl effects(sdk::recv_proxy_data *data, sdk::weapon_t *str, void *out);
} // namespace recv_proxies

namespace mat_system_surface
{
void __fastcall lock_cursor(sdk::mat_surface_t *surf, uint32_t edx);
}

namespace steam
{
HRESULT __stdcall present(IDirect3DDevice9 *dev, RECT *a, RECT *b, HWND c, RGNDATA *d);
HRESULT __stdcall reset(IDirect3DDevice9 *dev, D3DPRESENT_PARAMETERS *pp);
} // namespace steam

namespace inputsystem
{
LRESULT __stdcall wnd_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);
}

namespace miscellaneous
{
int32_t __stdcall get_module_handle_ex_a(uint32_t flags, const char *str, uint32_t *module);
void __cdecl host_shutdown();
void __fastcall hud_draw_scope(uintptr_t hud, uint32_t edx);
int32_t __fastcall particle_draw_model(uintptr_t particle, uint32_t edx, uint32_t flags, uint32_t instance);
void __fastcall ent_info_list_link_before(sdk::ent_info_list_t *list, uint32_t edx, sdk::ent_info_t *after,
										  sdk::ent_info_t *element);
bool __fastcall process_input(uintptr_t overhead, uint32_t edx);
int32_t __fastcall start_sound_immediate(sdk::start_sound_params *params, uint32_t edx);
const char *__fastcall check_param(uintptr_t cmd, uint32_t edx, const char *sz, const char **values);
void __fastcall move_helper_start_sound(uintptr_t move_helper, uint32_t edx, sdk::vec3 *origin, const char *name);
void __cdecl get_tonemap_settings_from_env_tonemap_controller();
void __fastcall draw_mesh(uintptr_t dx8_api, uint32_t edx, uint32_t mesh, uint32_t count,
						  sdk::mesh_instance_data *instances, uint32_t compression, uint32_t compiled_state,
						  uint32_t info);
void __fastcall draw_3d_debug_overlays(uintptr_t view, uint32_t edx);
void __fastcall render_glow_boxes(sdk::glow_manager_t *glow, uintptr_t edx, int32_t pass,
								  sdk::mat_render_context **ctx);
void __fastcall spawn_smoke_effect(uintptr_t smoke, uint32_t edx);
void __fastcall render_iron_sight_scope_effect(uintptr_t iron_sight, uint32_t edx, int32_t x, int32_t y, int32_t w,
											   int32_t h, uintptr_t view_setup);
void __cdecl convar_network_change_callback(sdk::convar *var, const char *old1, float old2);
void __fastcall tefirebullets_post_data_update(uintptr_t ent, uint32_t edx, uint32_t type);
void __cdecl enforce_competitive_cvar(const char *name, float min_value, float max_value, int32_t args, ...);
NTSTATUS NTAPI nt_query_virtual_memory(HANDLE process_handle, PVOID base_address, int memory_information_class,
									   PMEMORY_BASIC_INFORMATION mem_information, ULONG length, PULONG result_length);
} // namespace miscellaneous
} // namespace hooks

#endif // HOOKS_HOOKS_H
