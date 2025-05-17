
#include <base/cfg.h>
#include <base/hook_manager.h>
#include <detail/aim_helper.h>
#include <detail/custom_prediction.h>
#include <hacks/antiaim.h>
#include <hacks/misc.h>
#include <hacks/tickbase.h>

using namespace detail;
using namespace detail::aim_helper;
using namespace hacks;
using namespace sdk;

namespace hooks::engine_client
{
float __fastcall get_aspect_ratio(engine_client_t *client, uint32_t edx, int32_t width, int32_t height)
{
	if (!cfg.local_visuals.aspect_changer.get())
		return hook_manager.get_aspect_ratio->call(client, edx, width, height);

	constexpr auto min = 1.f;
	constexpr auto max = 21.f / 9.f;
	return min + std::clamp(cfg.local_visuals.aspect.get() / 100.f, 0.f, 100.f) * (max - min);
}

bool __fastcall is_connected(sdk::engine_client_t *client, uint32_t edx)
{
	if (cfg.misc.unlock_inventory &&
		uintptr_t(_ReturnAddress()) == game->client.at(functions::return_to_enable_inventory))
		return false;

	return hook_manager.is_connected->call(client, edx);
}

void __declspec(noinline) org_cl_move(float accumulated_extra_samples, bool final_tick)
{
	auto original = uintptr_t(hook_manager.cl_move->get_original());

	__asm
	{
			movss xmm0, accumulated_extra_samples
			mov cl, final_tick
			push original
			call [esp]
			add esp, 0x4
	}
}

void __cdecl cl_move_hook(float accumulated_extra_samples, bool final_tick)
{
	const auto start = game->client_state->last_command;

	if (!game->engine_client->is_ingame() || !game->client_state->net_channel || game->client_state->paused ||
		!game->local_player || !game->local_player->is_alive())
	{
		tickbase.reset();
		org_cl_move(accumulated_extra_samples, final_tick);
		if (start != game->client_state->last_command)
			game->cmds.push_back(game->client_state->last_command);
		return;
	}

	game->client_state->process_sockets();

	// could be disconnected now.
	if (!game->client_state->net_channel)
	{
		tickbase.reset();
		org_cl_move(accumulated_extra_samples, final_tick);
		return;
	}

	game->prediction->update(game->client_state->delta_tick, true, game->client_state->last_command_ack,
							 game->client_state->last_command + game->client_state->choked_commands);

	tickbase.to_adjust = 0;
	tickbase.force_choke = tickbase.force_unchoke = false;
	org_cl_move(accumulated_extra_samples, final_tick);

	if (tickbase.to_shift > 0)
	{
		while (tickbase.to_shift > 0)
		{
			tickbase.force_choke = --tickbase.to_shift > 0;
			if (!tickbase.to_shift)
			{
				tickbase.post_shift = true;
				game->prediction->get_predicted_commands() =
					std::clamp(game->client_state->last_command - game->client_state->last_command_ack, 0,
							   game->prediction->get_predicted_commands());
				for (auto i = game->client_state->last_command + 1;
					 i <= game->client_state->last_command + game->client_state->choked_commands; i++)
					game->input->commands[i % input_max].predicted = false;
				game->prediction->update(game->client_state->delta_tick, true, game->client_state->last_command_ack,
										 game->client_state->last_command + game->client_state->choked_commands);
			}
			org_cl_move(game->globals->interval_per_tick, final_tick);
		}
	}

	if (start == game->client_state->last_command)
	{
		game->client_state->net_channel->out_sequence_nr--;
		game->client_state->net_channel->choked_packets = 0;

		const auto recharge = game->input->commands[(start + 1) % input_max].tick_count == INT_MAX;
		if (recharge)
			game->client_state->choked_commands--;

		game->client_state->send_netmsg_tick();
		const auto next_command = game->client_state->net_channel->send_datagram();

		if (recharge)
			game->client_state->last_command = next_command;
	}
	else
	{
		tickbase.post_shift = false;
		tickbase.compute_current_limit(game->client_state->last_command);
		game->cmds.push_back(game->client_state->last_command);
	}
}

__declspec(naked) void cl_move()
{
	__asm {
			mov eax, dword ptr ss:[ebp-0x4]
			push ebp
			mov ebp, esp

			movzx ebx, cl
			push ebx // final_tick
			push eax // accumulated_extra_samples
			call cl_move_hook
			add esp, 0x8
			mov eax, 0

			pop ebp
			ret
	}
}
} // namespace hooks::engine_client
