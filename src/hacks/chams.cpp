
#include <base/debug_overlay.h>
#include <detail/custom_prediction.h>
#include <detail/player_list.h>
#include <hacks/chams.h>
#include <hacks/misc.h>
#include <hacks/visuals.h>
#include <sdk/client_entity_list.h>
#include <sdk/engine_client.h>
#include <sdk/render_view.h>

using namespace sdk;
using namespace detail;
using namespace hacks::misc;

namespace hacks
{
chams cham;

chams::chams()
{
	for (auto i = 0; i < max_material; i++)
	{
		materials[i] = nullptr;
		modes[i] = -1;
		clr1[i] = 0xFFFFFFFF;
		clr2[i] = 0xFFFFFFFF;
	}
}

chams::~chams()
{
	for (auto i = 0; i < max_material; i++)
		if (materials[i] != nullptr)
			materials[i]->decrement_reference_count();
}

void chams::on_draw_model(model_render_info_t &info, const std::function<void(mat3x4 *)> draw)
{
	auto player = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(info.entity_index));

	if (player && player->get_class_id() == class_id::csragdoll && cfg.player_esp.apply_on_death.get())
	{
		player = reinterpret_cast<cs_player_t *>(
			game->client_entity_list->get_client_entity_from_handle(player->get_player()));

		if (!player || !player->is_player())
			return;

		return perform_player(info, player, draw);
	}

	if (player && player->is_player() && player->is_valid(true))
	{
		auto &entry = GET_PLAYER_ENTRY(player);
		lag_record *record = nullptr;

		if (player->index() == game->engine_client->get_local_player())
			record = &local_record;
		else
		{
			for (auto &rec : entry.records)
				if (!rec.extrapolated)
				{
					record = &rec;
					break;
				}

			entry.visuals.has_matrix = true;
			memcpy(entry.visuals.matrix, player->get_bone_accessor().out,
				   sizeof(mat3x4) * player->get_studio_hdr()->numbones());
			entry.visuals.abs_org = player->get_abs_origin();
		}

		if (record && !record->has_visual_matrix)
		{
			memcpy(record->vis_mat, player->get_bone_accessor().out,
				   sizeof(mat3x4) * player->get_studio_hdr()->numbones());
			record->abs_origin = player->get_abs_origin();
			record->abs_angle = player->get_abs_angle();
			record->has_visual_matrix = true;
		}

		return perform_player(info, player, draw);
	}
	else
	{
		// try to swap for owner.
		if (info.renderable)
		{
			const auto parent = info.renderable->get_shadow_parent();
			const auto unknown = parent != nullptr ? parent->get_client_unknown() : nullptr;
			const auto owner =
				unknown != nullptr ? reinterpret_cast<cs_player_t *>(unknown->get_client_entity()) : nullptr;

			if (owner && owner->is_player() && owner->is_valid(true))
				return perform_player(info, owner, draw, true);
		}
	}

	XOR_STR_STACK(a, arms);
	XOR_STR_STACK(w, wpns);

	const auto rendering_arms = strstr(info.model->name, a) != nullptr;
	if (rendering_arms || strstr(info.model->name, w))
		perform_viewmodel(rendering_arms);
}

void chams::extend_chams()
{
	// TODO: this
	return;

	if (!game->engine_client->is_ingame() || !game->local_player)
		return;

	game->client_entity_list->for_each_player(
		[](cs_player_t *player)
		{
			if (!player->is_alive() || !player->is_dormant() || !player->is_enemy())
				return;

			if (cfg.player_esp.enemy_chams.mode->test(cfg_t::chams_mode::chams_disabled))
				return;

			auto &entry = GET_PLAYER_ENTRY(player);
			if (entry.visuals.has_matrix)
			{
				const auto inst = game->model_render->create_instance(player);
				const auto ang = player->get_abs_angle();
				game->model_render->draw_model(studio_render, player, inst, player->index(), player->get_model(),
											   &entry.visuals.pos.current, &ang, player->get_skin(), player->get_body(),
											   player->get_hitbox_set());
				game->model_render->destroy_instance(inst);
			}
		});
}

void chams::reset()
{
	if (!should_reset)
		return;

	game->model_render->forced_material_override(nullptr);
	should_reset = false;
}

void chams::reload(bool exit)
{
	for (auto i = 0; i < max_material; i++)
	{
		if (exit)
		{
			if (materials[i] != nullptr)
			{
				materials[i]->decrement_reference_count();
				materials[i] = nullptr;
			}
		}

		clr1[i] = 0xFFFFFFFF;
		clr2[i] = 0xFFFFFFFF;
	}
}

void chams::draw_fake_chams(const std::function<void(mat3x4 *)> draw)
{
	if (game->local_player && game->local_player->is_alive() && local_fake_record.has_state &&
		!cfg.local_visuals.fake_chams.mode.get().test(cfg_t::chams_disabled) && game->input->camera_in_third_person &&
		local_fake_record.has_matrix)
	{
		alignas(16) mat3x4 bones[max_bones]{};
		move_matrix(local_fake_record.unprocessed_mat, local_fake_record.abs_origin,
					game->local_player->get_abs_origin());
		memcpy(bones, local_fake_record.unprocessed_mat, sizeof(bones));

		const auto eye_angle = *game->local_player->eye_angles();
		const auto view_height = game->local_player->get_view_height();
		const auto bounds_change_time = game->local_player->get_bounds_change_time();
		const auto obb_maxs = game->local_player->get_collideable()->obb_maxs();

		pred.restore_animation(hacks::tickbase.recent.second, true);
		game->prediction->set_local_view_angles(eye_angle);
		game->local_player->post_build_transformations(bones, XOR_32(bone_used_by_anything));

		game->local_player->get_view_height() = view_height;
		game->local_player->get_bounds_change_time() = bounds_change_time;
		game->local_player->get_collideable()->obb_maxs() = obb_maxs;

		const auto draw_fake = [&draw, &bones](mat3x4 *) { draw(bones); };
		const auto mat = maintain_material(cfg.local_visuals.fake_chams, index_material(cfg.local_visuals.fake_chams));
		if (mat)
		{
			game->model_render->forced_material_override(mat);
			draw_fake(nullptr);
			game->model_render->forced_material_override(nullptr);
		}
	}
}

void chams::perform_player(model_render_info_t &info, cs_player_t *const player,
						   const std::function<void(mat3x4 *)> draw, const bool is_attachment)
{
	auto &entry = GET_PLAYER_ENTRY(player);
	const auto is_enemy = player->is_enemy();
	auto &cf = player == game->local_player
				   ? (is_attachment ? cfg.local_visuals.local_attachment_chams : cfg.local_visuals.local_chams)
				   : (is_enemy ? (is_attachment ? cfg.player_esp.enemy_attachment_chams : cfg.player_esp.enemy_chams)
							   : (is_attachment ? cfg.player_esp.team_attachment_chams : cfg.player_esp.team_chams));
	auto &cvf =
		is_enemy ? (is_attachment ? cfg.player_esp.enemy_attachment_chams_visible : cfg.player_esp.enemy_chams_visible)
				 : (is_attachment ? cfg.player_esp.team_attachment_chams_visible : cfg.player_esp.team_chams_visible);
	const auto cfi = index_material(cf);
	const auto cfhi = index_material(cfg.player_esp.enemy_chams_history);
	const auto cvfi = index_material(cvf);
	const auto cvhi = index_material(cfg.player_esp.enemy_chams_history);

	material *cfm = nullptr, *cfhm = nullptr, *cvfm = nullptr, *cvhm = nullptr;
	if (player != game->local_player)
	{
		cfm = maintain_material(cf, cfi);
		cfhm = maintain_material(cfg.player_esp.enemy_chams_history, cfhi);
		cvhm = maintain_material(cfg.player_esp.enemy_chams_history, cvhi);

		if (!cvf.mode.get().test(cfg_t::chams_mode::chams_disabled))
			cvfm = maintain_material(cvf, cvfi);
		else
			cvfm = maintain_material(cf, cvfi);
	}
	else
		cvfm = maintain_material(cf, cfi);

	// skip decals.
	info.flags |= 0x10000000;

	if (player == game->local_player && !is_attachment)
		draw_fake_chams(draw);

	if (cfhm && game->local_player && is_enemy && player->is_valid() && !is_attachment && game->cl_lagcompensation &&
		game->cl_predict)
	{
		auto first = 0.f, second = 0.f;
		entry.get_boundary_times(first, second, true);

		if (second > 0.f)
		{
			const auto record = entry.get_record(second, true);

			if (record && record->has_visual_matrix)
			{
				const auto dist = std::clamp(((record->abs_origin - info.origin).length() - 8.f) * .06f, 0.f, 1.f);

				if (dist > 0.f)
				{
					const auto backup_blend = game->render_view->get_blend();
					game->render_view->set_blend(dist);

					game->model_render->forced_material_override(cfhm);
					draw(record->vis_mat);

					if (cfg.player_esp.enemy_chams_history.primary->value.a >= .99f && dist >= .99f)
					{
						game->model_render->forced_material_override(cvhm);
						draw(record->vis_mat);
					}

					game->model_render->forced_material_override(nullptr);
					game->render_view->set_blend(backup_blend);
				}
			}

			if (record)
				entry.scan_records.count--;
		}
	}

	if (player->is_dormant())
	{
		has_override_mat = true;
		memcpy(override_mat, entry.visuals.matrix, sizeof(mat3x4) * max_bones);
		auto start = entry.visuals.abs_org;
		move_matrix(override_mat, start, entry.visuals.pos.current);
	}

	// TODO: this as well
	if (is_enemy && false)
		game->render_view->set_blend(entry.visuals.dormant_alpha);

	if (cfm && player != game->local_player)
	{
		game->model_render->forced_material_override(cfm);
		should_reset = true;
		draw(has_override_mat ? override_mat : nullptr);
	}

	if (cvfm)
	{
		game->model_render->forced_material_override(cvfm);
		should_reset = true;
	}

	// restore decals.
	if (cf.mode.get().test(cfg_t::chams_mode::chams_disabled) && cvf.mode.get().test(cfg_t::chams_mode::chams_disabled))
		info.flags &= ~0x10000000;
}

void chams::perform_viewmodel(const bool rendering_arms)
{
	auto &cf = rendering_arms ? cfg.local_visuals.arms_chams : cfg.local_visuals.weapon_chams;
	const auto mat = maintain_material(cf, index_material(cf));

	if (mat)
	{
		game->model_render->forced_material_override(mat);
		should_reset = true;
	}
}

material *chams::maintain_material(cfg_t::chams &chams, int32_t index)
{
	if (index < 0 || index >= max_material)
		return nullptr;

	auto mat = materials[index];
	const auto &mode = chams.mode.get();
	const auto setup = modes[index] != int8_t((uint64_t)mode);
	if (setup)
	{
		modes[index] = int8_t((uint64_t)mode);

		const char *type = nullptr;
		const char *material = nullptr;
		if (mode.test(cfg_t::chams_default))
		{
			XOR_STR_STACK(vl, vertexlit);
			type = vl;
			XOR_STR_STACK(defaultm, default_mat);
			material = defaultm;
		}
		else if (mode.test(cfg_t::chams_flat))
		{
			XOR_STR_STACK(ul, unlit);
			type = ul;
			XOR_STR_STACK(flatm, flat_mat);
			material = flatm;
		}
		else if (mode.test(cfg_t::chams_glow))
		{
			XOR_STR_STACK(vl, vertexlit);
			type = vl;
			XOR_STR_STACK(glowm, glow_mat);
			material = glowm;
		}
		else if (mode.test(cfg_t::chams_pulse))
		{
			XOR_STR_STACK(vl, vertexlit);
			type = vl;
			XOR_STR_STACK(pulsem, pulse_mat);
			material = pulsem;
		}
		else if (mode.test(cfg_t::chams_acryl))
		{
			XOR_STR_STACK(vl, vertexlit);
			type = vl;
			XOR_STR_STACK(acrylm, acryl_mat);
			material = acrylm;
		}

		if (mat != nullptr)
			mat->decrement_reference_count();

		if (material != nullptr)
		{
			const auto name = std::to_string(util::fnv1a(material) + index);
			const auto kv = keyvalues::produce(name.c_str(), type, material);
			mat = game->material_system->create_material(name.c_str(), kv);
			mat->increment_reference_count();
		}
		else
			mat = nullptr;
	}

	if (mat == nullptr)
		return materials[index] = mat;

	const auto &base_color = chams.primary.get();
	if (setup || base_color.rgba() != clr1[index])
	{
		clr1[index] = base_color.rgba();
		const auto v = vec3(base_color.value.r, base_color.value.g, base_color.value.b);
		static uint32_t clr_id = 0u;
		XOR_STR_STACK(clr, $color);
		mat->find_var_fast(clr, &clr_id)->set_vector(v);
		static uint32_t alpha_id = 0u;
		XOR_STR_STACK(al, $alpha);
		mat->find_var_fast(al, &alpha_id)->set_float(base_color.value.a);

		if (index == enemy_material || index == enemy_attachment_material || index == enemy_his_material ||
			index == friend_material || index == friend_attachment_material)
			mat->set_material_var_flag(material_var_ignorez, true);
	}

	const auto &secondary_color = chams.secondary.get();
	if ((mode.test(cfg_t::chams_glow) || mode.test(cfg_t::chams_pulse)) &&
		(setup || secondary_color.rgba() != clr2[index]))
	{
		clr2[index] = secondary_color.rgba();
		const auto v =
			vec3(secondary_color.value.a * secondary_color.value.r, secondary_color.value.a * secondary_color.value.g,
				 secondary_color.value.a * secondary_color.value.b);
		static uint32_t tint_id = 0u;
		XOR_STR_STACK(tint, $envmaptint);
		mat->find_var_fast(tint, &tint_id)->set_vector(v);
	}

	return materials[index] = mat;
}

int32_t chams::index_material(cfg_t::chams &chams)
{
	auto index = -1;
	if (&chams == &cfg.player_esp.enemy_chams)
		index = enemy_material;
	else if (&chams == &cfg.player_esp.enemy_attachment_chams)
		index = enemy_attachment_material;
	else if (&chams == &cfg.player_esp.enemy_chams_visible)
		index = enemy_vis_material;
	else if (&chams == &cfg.player_esp.enemy_attachment_chams_visible)
		index = enemy_vis_attachment_material;
	else if (&chams == &cfg.player_esp.enemy_chams_history)
		index = enemy_his_material;
	else if (&chams == &cfg.player_esp.team_chams)
		index = friend_material;
	else if (&chams == &cfg.player_esp.team_attachment_chams)
		index = friend_attachment_material;
	else if (&chams == &cfg.player_esp.team_chams_visible)
		index = friend_vis_material;
	else if (&chams == &cfg.player_esp.team_attachment_chams_visible)
		index = friend_vis_attachment_material;
	else if (&chams == &cfg.local_visuals.local_chams)
		index = local_material;
	else if (&chams == &cfg.local_visuals.local_attachment_chams)
		index = local_attachment_material;
	else if (&chams == &cfg.local_visuals.fake_chams)
		index = fake_material;
	else if (&chams == &cfg.local_visuals.arms_chams)
		index = arm_material;
	else if (&chams == &cfg.local_visuals.weapon_chams)
		index = weapon_material;
	return index;
}
} // namespace hacks
