
#ifndef NETWORK_APP_HACK_H
#define NETWORK_APP_HACK_H

#ifdef CSGO_SECURE
#include <app.h>
#include <services/messenger.h>

#define NETWORK() evo::app->get<network::app_hack>()
#define MESSENGER() SERVICE(messenger)

namespace network
{
using namespace evo;

class app_hack : public application
{
public:
	app_hack() : application(XOR("hack"))
	{
		const auto messenger = std::make_shared<services::messenger>(XOR("loader.ev0lve.xyz"));
		bind_callbacks(messenger);

		add_service(FNV("messenger"), messenger);
	}

	void start() override
	{
		if (!ssl->create_client())
		{
			MessageBoxA(nullptr, XOR("Could not initialize SSL."), XOR("Critical error"), MB_ICONERROR | MB_OK);
			quit(1);
		}

		application::start();
	}

	std::string username{};
	std::string avatar{};
	std::string build{};
	uint32_t avatar_size{};
	std::string expiry{};

	bool is_verified{};
	bool expecting_failure{};

private:
	int retries_left{3};
	void reconnect();
	void bind_callbacks(const std::shared_ptr<services::messenger> &msg);

	void start_heartbeat();
	void heartbeat();
};
} // namespace network
#endif
#endif // NETWORK_APP_HACK_H
