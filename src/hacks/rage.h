
#ifndef HACKS_RAGE_H
#define HACKS_RAGE_H

#include <detail/aim_helper.h>
#include <detail/player_list.h>
#include <sdk/cs_player.h>
#include <sdk/input.h>
#include <sdk/weapon.h>

namespace hacks
{
class rage
{
public:
	bool on_create_move(sdk::user_cmd *cmd, bool send_packet);
	void on_finalize_cycle(sdk::user_cmd *cmd, bool &send_packet);
	void on_manual(sdk::user_cmd *cmd, detail::shot &&s);

	bool did_stop() const;
	bool should_recalculate_eye_position() const;
	void autostop(sdk::user_cmd *cmd, bool should_stop = false, bool force = false, bool full = false);

	bool has_target() const;
	bool did_shoot() const;
	void drop_shot();
	void reset();
	bool has_priority_targets() const;
	bool is_player_target(sdk::cs_player_t *player) const;
	bool is_active() const;
	void set_attempt_jump(bool jump);

private:
	bool full_scan(sdk::user_cmd *cmd, bool send_packet);
	std::optional<detail::aim_helper::rage_target> scan_targets(sdk::user_cmd *cmd, bool send_packet);
	std::optional<detail::aim_helper::rage_target> scan_dormants(sdk::user_cmd *cmd);
	std::vector<detail::aim_helper::rage_target> scan_target(sdk::user_cmd *cmd, sdk::cs_player_t *player);
	std::optional<detail::aim_helper::rage_target> scan_record(detail::lag_record *record, const bool minimal = false);
	std::optional<detail::aim_helper::rage_target> scan_record_knife(detail::lag_record *record);
	std::optional<detail::aim_helper::rage_target> scan_record_shield(detail::lag_record *record);
	std::optional<detail::aim_helper::rage_target> scan_record_gun(detail::lag_record *record,
																   const bool minimal = false);
	void cock_revolver(sdk::user_cmd *cmd);
	void prioritize_player(sdk::cs_player_t *player, uint32_t amount = 1);
	bool is_player_in_range(sdk::cs_player_t *player);
	void update_scan_map();
	void cancel_autostop();

	bool has_stopped{}, should_stop{}, should_duck{}, is_engaged{}, recalculate_eye_position{}, had_target{},
		is_priority_scan{}, active{}, attempted_to_jump{};
	std::optional<sdk::user_cmd> intermediate_cmd = std::nullopt;
	std::optional<detail::shot> intermediate_shot = std::nullopt;
	std::deque<int32_t> scan_list_default{}, scan_list_priority{};
	std::array<bool, 65> scan_map;
};

extern rage rag;
} // namespace hacks

#endif // HACKS_RAGE_H
