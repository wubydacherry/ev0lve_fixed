
#include <base/cfg.h>
#include <base/debug_overlay.h>
#include <base/event_log.h>
#include <sdk/engine_trace.h>

using namespace sdk;

void event_log::force_output()
{
	is_forced_output = true;
	output();
	is_forced_output = false;
}

void event_log::output()
{
	if (!is_forced_output && (cfg.misc.eventlog.get().none() || log.empty()))
	{
		log.clear();
		return;
	}

	// init output strings
	std::string chat_output;
	std::wstring top_left_output{};

	std::vector<event_element> log_output = prefix;
	for (auto &e : log)
		log_output.push_back(e);

	// loop through all parts
	for (auto part = log_output.begin(); part != log_output.end(); ++part)
	{
		const bool encrypted = part->dec_text.empty();

		// init output strings
		std::string current_text, console_output;

		if (encrypted)
		{
			// decrypt
			XOR_STR_STACK(dec, part->enc_text);
			current_text = dec;
		}
		else
			current_text = part->dec_text;

		// set string
		if (cfg.misc.eventlog.get().test(cfg_t::eventlog_console) || is_forced_output)
		{
			// set current text
			console_output = current_text;

			// output current part in color
			output_console(part->color, console_output.c_str());
		}

		if (cfg.misc.eventlog.get().test(cfg_t::eventlog_chat))
			chat_output.append(static_cast<char>(part->color) + current_text);

		top_left_output.append(util::utf8_decode(current_text));
	}

	// finish the line, console only
	if (cfg.misc.eventlog.get().test(cfg_t::eventlog_console))
		output_console("\n");
	if (cfg.misc.eventlog.get().test(cfg_t::eventlog_chat))
		output_chat(chat_output.c_str());

	log_text_only.emplace_back(std::make_tuple(top_left_output, game->globals->realtime, 1.f));

	// clear event log after usage
	log.clear();
}

void event_log::on_player_hurt(sdk::game_event *event)
{
	if (!game->local_player || !event)
		return;

	const auto attacker = event->get_int(XOR_STR("attacker"));
	const auto attacker_index = game->engine_client->get_player_for_user_id(attacker);

	const auto userid = event->get_int(XOR_STR("userid"));
	const auto index = game->engine_client->get_player_for_user_id(userid);

	const auto dmg = event->get_int(XOR_STR("dmg_health"));
	const auto health = event->get_int(XOR_STR("health"));
	const auto hitgroup = event->get_int(XOR_STR("hitgroup"));

	auto hitbox = XOR_STR_STORE("");

	switch (hitgroup)
	{
	case hitgroup_head:
		hitbox = XOR_STR_STORE("head");
		break;
	case hitgroup_chest:
		hitbox = XOR_STR_STORE("upper body");
		break;
	case hitgroup_stomach:
		hitbox = XOR_STR_STORE("lower body");
		break;
	case hitgroup_leftarm:
	case hitgroup_rightarm:
		hitbox = XOR_STR_STORE("arm");
		break;
	case hitgroup_leftleg:
	case hitgroup_rightleg:
		hitbox = XOR_STR_STORE("leg");
		break;
	case hitgroup_neck:
		hitbox = XOR_STR_STORE("neck");
		break;
	default:
		break;
	}

	std::string victim_name, attacker_name;

	if (attacker_index == game->engine_client->get_local_player())
	{
		if (!cfg.misc.event_triggers.get().test(cfg_t::event_shot_info))
			return;

		const auto victim_info = game->engine_client->get_player_info(index);

		victim_name = victim_info.name;
		attacker_name = XOR_STR("You");
	}
	else if (index == game->engine_client->get_local_player())
	{
		if (!cfg.misc.event_triggers.get().test(cfg_t::event_dmg_taken))
			return;

		if (attacker_index > 0)
		{
			const auto attacker_info = game->engine_client->get_player_info(attacker_index);
			attacker_name = attacker_info.name;
		}
		else
			attacker_name = XOR_STR("World");

		victim_name = XOR_STR("you");
	}
	else
		return;

	if (hitgroup > hitgroup_generic && hitgroup < hitgroup_gear)
	{
		add(0x09, attacker_name);
		add(0x06, XOR_STR_STORE(" hit "));
		add(0x09, victim_name);
		add(0x06, XOR_STR_STORE(" in "));
		add(0x09, hitbox);
		add(0x06, XOR_STR_STORE(" for "));
		add(0x09, std::to_string(dmg));
		add(0x06, XOR_STR_STORE(" dmg. "));
		add(0x09, std::to_string(health));
		add(0x06, XOR_STR_STORE(" HP remaining."));
	}
	else
	{
		add(0x09, attacker_name);
		add(0x06, XOR_STR_STORE(" hit "));
		add(0x09, victim_name);
		add(0x06, XOR_STR_STORE(" for "));
		add(0x09, std::to_string(dmg));
		add(0x06, XOR_STR_STORE(" dmg. "));
		add(0x09, std::to_string(health));
		add(0x06, XOR_STR_STORE(" HP remaining."));
	}

	output();
}

void event_log::on_purchase(sdk::game_event *event)
{
	if (!game->local_player || !game->local_player->is_alive() || !event)
		return;

	if (!cfg.misc.event_triggers.get().test(cfg_t::event_purchase))
		return;

	const auto userid = event->get_int(XOR_STR("userid"));
	const auto index = game->engine_client->get_player_for_user_id(userid);
	const auto info = game->engine_client->get_player_info(index);

	auto player = reinterpret_cast<cs_player_t *>(game->client_entity_list->get_client_entity(index));

	if (player && player->get_team_num() == game->local_player->get_team_num())
		return;

	add(0x09, info.name);
	add(0x06, XOR_STR(" purchased "));
	add(0x09, event->get_string(XOR_STR("weapon")));

	output();
}

event_log eventlog;
