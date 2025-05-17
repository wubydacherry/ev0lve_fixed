
#include <base/hook_manager.h>
#include <hacks/chams.h>
#include <hacks/visuals.h>
#include <sdk/render_view.h>

#ifdef CSGO_LUA
#include <lua/engine.h>
#endif

using namespace sdk;
using namespace hacks;

namespace hooks::model_render
{
#ifdef CSGO_LUA
// meme to avoid capture since can't pass those to lua without static rtti
struct
{
	uintptr_t render;
	draw_model_state *state;
	uint32_t edx;
	mat_render_context *context;
	model_render_info_t *info;
	mat3x4 *bone;
} cur_data;

void lua_callback()
{
	hook_manager.draw_model_execute->call(cur_data.render, cur_data.edx, cur_data.context, cur_data.state,
										  cur_data.info, cur_data.bone);
}
#endif

void __fastcall draw_model_execute(uintptr_t render, uint32_t edx, mat_render_context *context, draw_model_state *state,
								   model_render_info_t *info, mat3x4 *bone)
{
	if (!game->engine_client->is_ingame())
		return hook_manager.draw_model_execute->call(render, edx, context, state, info, bone);

	// potentially skip shadows.
	if (info->flags & 0x40000000 || !(info->flags & 1))
	{
		if (cfg.local_visuals.disable_post_processing.get())
			return;

		return hook_manager.draw_model_execute->call(render, edx, context, state, info, bone);
	}

	// skip calls from the glow manager.
	const auto texture = context->get_render_target();
	if (texture)
	{
		const char *texture_name = texture->get_name();
		if (texture_name[4] == 'f' && texture_name[10] == 'a')
			return hook_manager.draw_model_execute->call(render, edx, context, state, info, bone);
	}

#ifdef CSGO_LUA
	cur_data.render = render;
	cur_data.edx = edx;
	cur_data.context = context;
	cur_data.state = state;
	cur_data.info = info;
	cur_data.bone = bone;

	const auto draw_cbk = [](lua_State *l) -> int
	{
		lua_callback();
		return 0;
	};

	lua::api.callback(FNV1A("on_draw_model_execute"),
					  [&](lua::state &s)
					  {
						  s.push(draw_cbk);
						  s.push(info->entity_index);
						  s.push(info->model->name);

						  return 3;
					  });
#endif

	const auto backup_blend = game->render_view->get_blend();
	vis->set_alpha_for_third_person(*info);

	vec3 def(1.f, 1.f, 1.f);
	auto org = context->get_tone_mapping_scale_linear();
	context->set_tone_mapping_scale_linear(&def);
	game->render_view->set_color_modulation(color::white());

	cham.on_draw_model(*info,
					   [&](mat3x4 *override_mat)
					   {
						   const auto body = info->body;
						   info->body = 0;
						   hook_manager.draw_model_execute->call(render, edx, context, state, info,
																 override_mat ? override_mat : bone);
						   info->body = body;
					   });
	hook_manager.draw_model_execute->call(render, edx, context, state, info,
										  cham.has_override_mat ? cham.override_mat : bone);

	context->set_tone_mapping_scale_linear(&org);

	cham.reset();
	cham.has_override_mat = false;
	game->render_view->set_blend(backup_blend);
}
} // namespace hooks::model_render
