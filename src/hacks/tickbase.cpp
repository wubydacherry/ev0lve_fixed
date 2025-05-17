
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <hacks/antiaim.h>
#include <hacks/peek_assistant.h>
#include <hacks/rage.h>
#include <hacks/tickbase.h>
#include <sdk/client_state.h>

using namespace sdk;
using namespace detail;
using namespace detail::aim_helper;
using namespace hooks;

namespace hacks
{
tickbase_t tickbase{};

void tickbase_t::reset()
{
	to_recharge = to_shift = to_adjust = 0;
	delay_shift = -1;
	force_choke = skip_next_adjust = post_shift = keep_config_changed = fast_fire = hide_shot = false;
	lag = recent = {0, 0};
}

void tickbase_t::adjust_limit_dynamic(user_cmd *cmd)
{
	const auto changed = apply_static_configuration();
	const auto ready = !to_shift && !post_shift && !force_choke;

	if (changed)
		keep_config_changed = force_unchoke = true;

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn || !ready || recent.second != game->client_state->last_command)
		return;
	const auto data = wpn->get_cs_weapon_data();

	auto dont_recharge = wpn->is_grenade() && ((wpn->get_pin_pulled() || wpn->get_throw_time() != 0.f) ||
											   rag.has_target() || pred.had_attack || cmd->weapon_select);
	if (dont_recharge)
		keep_config_changed = false;

	const auto diff_wpn = wpn->get_next_primary_attack() - game->globals->curtime;
	if (!dont_recharge && !changed && fast_fire && (wpn->is_shootable_weapon() || wpn->is_knife()) &&
		((data->cycle_time < .55f && diff_wpn > -.2f) || diff_wpn > .7f) && (wpn->is_knife() || !wpn->get_in_reload()))
		dont_recharge = true;

	const auto diff_player = game->local_player->get_next_attack() - game->globals->curtime;
	if (!dont_recharge && diff_player > .7f)
		dont_recharge = true;

	if (keep_config_changed)
		dont_recharge = false;

	if (dont_recharge)
		to_recharge = 0;

	const auto diff = determine_optimal_limit() - compute_current_limit();
	const auto standing = pred.info[(cmd->command_number - 1) % input_max].velocity.length() < 1.1f &&
						  pred.original_cmd.forwardmove == 0.f && pred.original_cmd.sidemove == 0.f;
	if (!dont_recharge && diff > 0 && (diff > 2 || standing))
	{
		to_recharge = diff;
		to_shift = 0;
	}
	else if (diff < 0)
	{
		to_recharge = 0;
		to_shift = -diff;
	}

	if (!diff)
		keep_config_changed = false;
}

bool tickbase_t::attempt_shift_back(bool &send_packet)
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if ((!holds_tick_base_weapon() && (!wpn->is_grenade() || !peek_assistant.pos.has_value())) ||
		(!fast_fire && !hide_shot) || aa.is_fakeducking())
		return true;

	const auto optimal = determine_optimal_shift();

	if (to_shift <= 0 &&
		(aa.get_shot_cmd() > game->client_state->last_command || (pred.can_shoot && pred.had_attack)) &&
		compute_current_limit() > 2 && game->local_player->get_tick_base() >= lag.first &&
		wpn->get_item_definition_index() != item_definition_index::weapon_revolver)
	{
		skip_next_adjust = send_packet = true;
		return false;
	}

	if (fast_fire)
	{
		const auto limit = compute_current_limit();
		to_shift = optimal;
		if (limit - to_shift <= 3)
			to_shift = limit;

		to_recharge = 0;

		if (to_shift > 0)
			send_packet = false;
	}

	return true;
}

void tickbase_t::prepare_cycle(int32_t command_number)
{
	to_adjust = 0;

	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	auto &p1 = pred.info[command_number % input_max];
	if (p1.sequence != command_number)
		return;

	p1.tickbase.sent_commands = game->client_state->choked_commands + 1;

	if (((fast_fire && cfg.rage.fast_fire.get().test(cfg_t::fast_fire_peek)) || hide_shot) &&
		wpn->get_item_definition_index() != item_definition_index::weapon_revolver && aa.is_peeking() && !to_shift)
		skip_next_adjust = true;

	if (skip_next_adjust)
		game->prediction->get_predicted_commands() =
			std::clamp(game->client_state->last_command - game->client_state->last_command_ack, 0,
					   game->prediction->get_predicted_commands());
	else
		to_adjust = p1.tickbase.limit;

	for (auto i = game->client_state->last_command + 1; i <= command_number; i++)
	{
		auto &p2 = pred.info[i % input_max];
		if (p2.sequence != i)
			continue;
		p2.tickbase.skip_fake_commands = skip_next_adjust;
	}

	compute_current_limit(command_number);
}

void tickbase_t::on_finalize_cycle()
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));

	if (!wpn)
		return;

	const auto is_grenade = wpn->is_grenade();

	skip_next_adjust = false;
	for (auto i = 0; i < to_adjust; i++)
	{
		game->client_state->choked_commands++;
		const auto sequence = game->client_state->last_command + game->client_state->choked_commands + 1;
		auto cmd = &game->input->commands[sequence % input_max];
		*cmd = game->input->commands[(sequence - 1) % input_max];
		cmd->command_number = sequence;
		if (!is_grenade)
			cmd->buttons &= ~(user_cmd::attack | user_cmd::attack2);
		cmd->tick_count = game->globals->tickcount + 200 + i;
		auto &verified = game->input->verified_commands[cmd->command_number % input_max];
		verified.cmd = *cmd;
		verified.crc = cmd->get_checksum();
	}
}

void tickbase_t::on_run_command(user_cmd *cmd, int32_t &tickbase)
{
	const auto &p1 = pred.info[cmd->command_number % input_max];
	if (p1.sequence != cmd->command_number)
		return;

	auto to_adjust = 0;
	std::optional<bool> prev_skip_fake_commands;

	for (auto i = game->client_state->last_command_ack; i <= cmd->command_number; i++)
	{
		const auto &p2 = pred.info[i % input_max];
		if (p2.sequence != i)
			continue;

		if (p2.tickbase.invalid_commands > 0)
		{
			prev_skip_fake_commands = false;
			continue;
		}

		if (!prev_skip_fake_commands.has_value())
			prev_skip_fake_commands = p2.tickbase.skip_fake_commands;

		if (prev_skip_fake_commands != p2.tickbase.skip_fake_commands)
			to_adjust = (p2.tickbase.skip_fake_commands ? p2.tickbase.limit : -p2.tickbase.limit) + p2.tickbase.adjust;
		else
			to_adjust = 0;

		prev_skip_fake_commands = p2.tickbase.skip_fake_commands;
	}

	tickbase += to_adjust;
}

void tickbase_t::on_recharge(int32_t command_number)
{
	auto &p = pred.info[command_number % input_max];
	p = {};
	p.sequence = command_number;
	p.tickbase.invalid_commands++;
}

void tickbase_t::on_finish_command(bool send_packet)
{
	const auto cmd = game->client_state->last_command + game->client_state->choked_commands + 1;
	auto &p = pred.info[cmd % input_max];
	if (p.sequence != cmd)
		return;

	if (to_shift > 0)
		p.tickbase.extra_commands++;

	if (send_packet)
		on_finalize_cycle();
}

bool tickbase_t::apply_static_configuration()
{
	const auto previous = fast_fire || hide_shot;

	if (cfg.rage.fake_duck.get())
		fast_fire = hide_shot = false;
	else
	{
		fast_fire = cfg.rage.enable_fast_fire.get();
		hide_shot = !fast_fire && cfg.rage.hide_shot.get();
	}

	return previous != (fast_fire || hide_shot);
}

int32_t tickbase_t::determine_optimal_shift() const
{
	const auto wpn = reinterpret_cast<weapon_t *const>(
		game->client_entity_list->get_client_entity_from_handle(game->local_player->get_active_weapon()));
	if (!wpn)
		return 0;

	const auto limit = compute_current_limit();

	if (wpn->is_grenade() || wpn->get_item_definition_index() == item_definition_index::weapon_shield ||
		wpn->is_knife() || wpn->get_item_definition_index() == item_definition_index::weapon_revolver ||
		peek_assistant.pos.has_value())
		return limit;

	return std::clamp(TIME_TO_TICKS(wpn->get_cs_weapon_data()->cycle_time) - 1, 4, limit);
}

int32_t tickbase_t::determine_optimal_limit() const
{
	if (fast_fire || hide_shot)
		return max_new_cmds;

	return 0;
}

int32_t tickbase_t::compute_current_limit(int32_t command_number) const
{
	if (!command_number)
		return 0;

	const auto &p = pred.info[game->client_state->last_command_ack % input_max];
	auto limit = p.sequence == game->client_state->last_command_ack ? p.tickbase.limit : 0;

	for (auto i = game->client_state->last_command_ack + 1; i <= command_number; i++)
	{
		auto &p2 = pred.info[i % input_max];
		if (p2.sequence != i)
			continue;

		p2.tickbase.limit = std::clamp(limit + p2.tickbase.invalid_commands, 0, sv_maxusrcmdprocessticks);
		p2.tickbase.limit = limit = max(p2.tickbase.limit - p2.tickbase.extra_commands, 0);
	}

	return limit;
}
} // namespace hacks
