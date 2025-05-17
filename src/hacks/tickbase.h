
#ifndef HACKS_TICKBASE_H
#define HACKS_TICKBASE_H

#include <sdk/game_rules.h>
#include <sdk/misc.h>

#define max_new_cmds (sv_maxusrcmdprocessticks - 2)

namespace hacks
{
class tickbase_t
{
public:
	void reset();
	void adjust_limit_dynamic(sdk::user_cmd *cmd);
	bool attempt_shift_back(bool &send_packet);
	void prepare_cycle(int32_t command_number = game->client_state->last_command + game->client_state->choked_commands +
												1);
	void on_finalize_cycle();
	void on_run_command(sdk::user_cmd *cmd, int32_t &tickbase);
	void on_recharge(int32_t command_number);
	void on_finish_command(bool send_packet);
	bool apply_static_configuration();
	int32_t determine_optimal_shift() const;
	int32_t determine_optimal_limit() const;
	int32_t compute_current_limit(int32_t command_number = game->client_state->last_command +
														   game->client_state->choked_commands + 1) const;

	int32_t to_recharge{}, to_shift{}, to_adjust{}, delay_shift = -1;

	std::pair<int32_t, int32_t> lag, recent;

	bool force_choke{}, force_unchoke{}, skip_next_adjust{}, fast_fire{}, hide_shot{}, post_shift{},
		keep_config_changed{};
};

extern tickbase_t tickbase;
} // namespace hacks

#endif // HACKS_TICKBASE_H
