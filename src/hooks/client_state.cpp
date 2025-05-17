
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/hook_manager.h>
#include <detail/player_list.h>
#include <sdk/keyvalues.h>

using namespace sdk;
using namespace detail;

namespace hooks::client_state
{
void __fastcall packet_start(client_state_t *state, uint32_t edx, int32_t incoming_sequence,
							 int32_t outgoing_acknowledged)
{
	// erase commands that are out of range by a huge margin.
	game->cmds.erase(std::remove_if(game->cmds.begin(), game->cmds.end(),
									[&](const uint32_t &cmd)
									{ return abs(int32_t(outgoing_acknowledged - cmd)) >= input_max; }),
					 game->cmds.end());

	// rollback the ack count to what we aimed for.
	auto target_acknowledged = outgoing_acknowledged;
	for (auto c : game->cmds)
		if (outgoing_acknowledged >= c)
			target_acknowledged = c;

	hook_manager.packet_start->call(state, edx, incoming_sequence, target_acknowledged);
}

bool __fastcall send_netmsg(net_channel *channel, uint32_t edx, net_message *msg, bool reliable, bool voice)
{
	if (game->skip_net_msg || msg->get_type() == 14)
		return true;

	if (msg->get_group() == 9)
		voice = false;

	return hook_manager.send_netmsg->call(channel, edx, msg, reliable, voice);
}

int32_t __fastcall send_datagram(net_channel *channel, uint32_t edx, uintptr_t datagram)
{
	if (!game->engine_client->is_ingame() || channel != game->client_state->net_channel)
		return hook_manager.send_datagram->call(channel, edx, datagram);

	const auto org_seqnr = channel->in_sequence_nr;
	const auto reliable_changed = channel->in_reliable_state != hacks::misc::last_reliable_state;
	hacks::misc::last_reliable_state = channel->in_reliable_state;

	const auto spike = TIME_TO_TICKS(cfg.rage.fake_ping_amount.get() * .001f - channel->get_latency(flow_outgoing));
	if (cfg.rage.fake_ping.get() && spike > 0 && !reliable_changed && channel->in_sequence_nr > spike)
		channel->in_sequence_nr -= spike;

	const auto ret = hook_manager.send_datagram->call(channel, edx, datagram);
	channel->in_sequence_nr = org_seqnr;
	return ret;
}

void __fastcall send_server_cmd_key_values(client_state_t *state, uint32_t edx, keyvalues *kv)
{
	game->skip_net_msg = kv && FNV1A_CMP(kv->get_name(), "UserExtraData");
	hook_manager.send_server_cmd_key_values->call(state, edx, kv);
	game->skip_net_msg = false;
}

bool __fastcall svc_msg_voicedata(client_state_t *state, uint32_t edx, svc_msg_voicedata_t *msg)
{
	if (!cfg.player_esp.shared_esp.get() || msg->format != 0 || !game->local_player)
		return hook_manager.svc_msg_voicedata->call(state, edx, msg);

	// check bhearsplayer
	if (!msg->sequence_bytes && !msg->section_number && !msg->uncompressed_sample_offset)
		return true;
	if (msg->client + 1 == game->engine_client->get_local_player())
		return true;

	uint8_t buf[shared_esp_size];
	msg->get(buf);

	player_list.recv_esp_data(buf, msg->client + 1);
	return true;
}
} // namespace hooks::client_state
