
#ifdef CSGO_SECURE

#include <base/game.h>
#include <data/common.h>
#include <data/hello.h>
#include <data/version.h>
#include <gui/gui.h>
#include <lua/helpers.h>
#include <network/app_hack.h>
#include <network/helpers.h>
#include <ren/renderer.h>
#include <sdk/input_system.h>

#include <ren/types/animated_texture.h>

#include <VirtualizerSDK.h>
#include <util/fnv1a.h>

#include <sdk/convar.h>

using namespace evo;
using namespace network;

void app_hack::bind_callbacks(const std::shared_ptr<services::messenger> &msg)
{
	VIRTUALIZER_SHARK_WHITE_START;

	msg->bind(evo::msg::msg_lua_data,
			  [](const data::data_t &data, data::msg_t &m)
			  {
				  auto d = m.get_data<data::lua_data>();
				  lua::api.callback(FNV1A("on_net_data"),
									[&](lua::state &s)
									{
										s.push(d.key);
										s.push((int)d.from);
										lua::helpers::load_table(s.get_state(), d.data);
										return 3;
									});
			  });

	msg->allow_quit = false;
	msg->on_connect = [&]()
	{
		data::hello inf{};
		inf.version = data::version_hack;
		inf.session = session;
		inf.client = data::ct_hack;

		data::msg_t m(msg::msg_hello);
		m.set_data(inf);

		MESSENGER()->add(m,
						 [&](const data::data_t &d, data::msg_t &resp)
						 {
							 if (!resp.id || resp.error.code)
							 {
								 ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
								 QUIT_ERROR("Connection failed");
								 return;
							 }

							 const auto user = resp.get_data<data::user_data>();
							 username = user.username;
							 avatar = user.avatar;
							 avatar_size = user.avatar_size;
							 expiry = user.expiry;
							 build = user.build;

							 is_verified = true;
							 start_heartbeat();
						 });

		if (expecting_failure)
			gui::notify.add(
				std::make_shared<gui::notification>(XOR("Connected"), XOR("Network services are available")));

		expecting_failure = false;
		retries_left = 3;
	};

	msg->on_connect_error = [&](const std::string &e)
	{
		if (!expecting_failure)
		{
			ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
			QUIT_ERROR("Connection failed");
		}
		else
		{
			reconnect();
			gui::notify.add(std::make_shared<gui::notification>(XOR("Connection failed"), XOR("Retrying in 30s")));
		}
	};

	msg->on_disconnect = [&]()
	{
		gui::notify.add(
			std::make_shared<gui::notification>(XOR("Connection lost"), XOR("Network services are unavailable")));
		expecting_failure = true;
		reconnect();
	};

	VIRTUALIZER_SHARK_WHITE_END;
}

void app_hack::reconnect()
{
	VIRTUALIZER_SHARK_WHITE_START;

	if (!retries_left)
	{
		ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
		QUIT_ERROR("Connection failed");
	}

	std::thread(
		[&]()
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(30s);
			std::this_thread::yield();

			--retries_left;
			MESSENGER()->start();
		})
		.detach();

	VIRTUALIZER_SHARK_WHITE_END;
}

void app_hack::start_heartbeat()
{
	VIRTUALIZER_SHARK_WHITE_START;

	using namespace std::chrono_literals;

	std::thread(
		[&]()
		{
			while (MESSENGER()->get_connected())
			{
				std::this_thread::sleep_for(1min);
				std::this_thread::yield();

				heartbeat();
			}
		})
		.detach();

	VIRTUALIZER_SHARK_WHITE_END;
}

void app_hack::heartbeat()
{
	data::msg_t m(msg::msg_heartbeat);
	auto resp = MESSENGER()->add_and_wait(m, 60);

	if (!MESSENGER()->get_connected())
		return;

	if (resp.message.error.code != 0)
	{
		switch (resp.message.error.code)
		{
		case data::error_code::ec_banned:
			ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
			QUIT_ERROR("You have been banned");
			break;
		case data::error_code::ec_no_access:
			ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
			QUIT_ERROR("Subscription expired");
			break;
		case data::error_code::ec_server_error:
			ShowWindow(game->input_system->get_attached_window(), SW_HIDE);
			QUIT_ERROR("Server error");
			break;
		default:
			return;
		}
	}

	const auto beat = resp.message.get_data<data::heartbeat>();
	session = beat.token;

	if (beat.expires_soon)
		gui::notify.add(std::make_shared<gui::notification>(XOR("Warning"), XOR("Your subscription expires soon!")));

	if (beat.alerts > 0)
	{
		if (beat.alerts == 1)
			gui::notify.add(std::make_shared<gui::notification>(XOR("Forum"), XOR("You have a new alert")));
		else
			gui::notify.add(std::make_shared<gui::notification>(
				XOR("Forum"), tfm::format(XOR("You have %d new alerts"), beat.alerts)));
	}

	if (!beat.conversations.empty())
	{
		for (const auto &un : beat.conversations)
			gui::notify.add(
				std::make_shared<gui::notification>(XOR("Forum"), tfm::format(XOR("New message from %s"), un)));
	}

	// ahem
	if (!gui::ctx)
		return;

	gui::ctx->user.username = beat.data.username;
	gui::ctx->user.active_until = beat.data.expiry;

	if (avatar_size != beat.data.avatar_size)
	{
		avatar_size = beat.data.avatar_size;
		avatar = beat.data.avatar;

		if (!avatar_size)
		{
			ren::draw.textures[GUI_HASH("user_avatar")]->destroy();
			ren::draw.textures[GUI_HASH("user_avatar")] = {};
			gui::ctx->user.avatar = nullptr;
		}
		else
		{
			ren::draw.textures[GUI_HASH("user_avatar")]->destroy();
			gui::ctx->user.avatar = std::make_shared<ren::animated_texture>(
				reinterpret_cast<void *>(network::get_decoded_avatar().data()), avatar_size);
			gui::ctx->user.avatar->create();
			ren::draw.textures[GUI_HASH("user_avatar")] = gui::ctx->user.avatar;
		}
	}
}

#endif