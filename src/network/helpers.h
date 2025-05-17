
#ifndef NETWORK_HELPERS_H
#define NETWORK_HELPERS_H

#include <lua/engine.h>
#include <nlohmann/json.hpp>

namespace network
{
struct user_data
{
	std::string username{};
	std::string expiry{};
	unsigned int avatar_size{};
};

struct item_data
{
	std::string name{};
	int price{};
};

inline std::string session{};

bool is_available();

// workshop
void refresh_configs();
void refresh_scripts(std::vector<lua::script_file> &files);
std::string decrypt_script(const std::string &filename, int id, int last_update);

// payments
std::optional<item_data> get_item_data(int id, const std::string &item_id);
int purchase_item(int id, const std::string &item_id);
std::vector<std::string> get_purchased_items(int id);

// lua sockets
void listen(const std::string &id, const std::function<void()> &cbk);
void stop_listen(const std::string &id);
void send(const std::string &id, const nlohmann::json &data);

// startup
void load_required_resources();

// user
void disconnect();
user_data get_user_data();
std::string get_decoded_avatar();
std::string get_build();
} // namespace network

#endif // NETWORK_HELPERS_H
