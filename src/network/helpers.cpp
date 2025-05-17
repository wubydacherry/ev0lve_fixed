
//#include <gzip/decompress.hpp>

#include <fstream>
#include <network/app_hack.h>
#include <network/helpers.h>

//#include <data/common.h>
//#include <data/payments.h>
//#include <data/resources.h>
#include <filesystem>

#include <VirtualizerSDK.h>
#include <base/cfg.h>
#include <base/debug_overlay.h>

#include <network/load_indicator.h>

#ifdef CSGO_SECURE
#undef min
#undef max
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <zip_file.hpp>


bool network::is_available() { return NETWORK() != nullptr; }

void network::disconnect()
{
	VIRTUALIZER_SHARK_WHITE_START;
	NETWORK()->shutdown();
	VIRTUALIZER_SHARK_WHITE_END;
}

network::user_data network::get_user_data() { return {NETWORK()->username, NETWORK()->expiry, NETWORK()->avatar_size}; }

std::string network::get_decoded_avatar()
{
	const auto s = evo::util::base64_decode(NETWORK()->avatar);
	return gzip::decompress(s.data(), s.size());
}

void network::load_required_resources()
{
	constexpr auto logo_resources = 70;
	constexpr auto total_resources = logo_resources;

	std::thread(
		[&]()
		{
			VIRTUALIZER_SHARK_WHITE_START;

			const auto fraction = 1.f / (float)total_resources;
			int tot_loaded{};
			if (std::filesystem::exists(XOR_STR("ev0lve/rc")))
			{
				std::ifstream f(XOR_STR("ev0lve/rc"), std::ios::binary);
				if (f.is_open())
				{
					for (int i{1}; i <= logo_resources; ++i)
					{
						uint32_t sz{};
						f.read(reinterpret_cast<char *>(&sz), sizeof(sz));
						
						if (sz < 256 || f.eof())
						{
							// can't be less than 256 b here.
							f.close();
							std::filesystem::remove(XOR_STR("ev0lve/rc"));
							load_ind.logo_frames.clear();
							load_ind.section_progress = 0.f;
							break;
						}

						std::vector<char> data{};
						data.resize(sz + 1);
						f.read(data.data(), sz);

						try
						{
							const auto tex = std::make_shared<evo::ren::texture>((void *)data.data(), sz);
							tex->create();

							if (i == 1)
								load_ind.first_frame = tex;

							load_ind.logo_frames.emplace_back(tex);
						}
						catch (...)
						{
							f.close();
							std::filesystem::remove(XOR_STR("ev0lve/rc"));
							load_ind.logo_frames.clear();
							load_ind.section_progress = 0.f;
							break;
						}

						load_ind.section_progress += fraction;
						++tot_loaded;
					}

					if (tot_loaded == logo_resources)
					{
						load_ind.begin_next_section();
						return;
					}
				}
			}

			const auto load_res = [](const std::string &name)
			{
				auto m = msg::message(msg::msg_get_resource);
				m.set_data(name);

				auto resp = MESSENGER()->add_and_wait(m);
				if (!resp.message.id || resp.message.error.code != 0)
					return std::string();

				return resp.message.get_data<std::string>();
			};

			std::ofstream f(XOR_STR("ev0lve/rc"), std::ios::binary);
			for (int i{1}; i <= logo_resources; ++i)
			{
				const auto d = load_res(tfm::format(XOR_STR("logo/logo (%d).png"), i));
				if (!d.empty())
				{
					const auto content = evo::util::base64_decode(d);
					const auto tex = std::make_shared<evo::ren::texture>((void *)content.data(), content.size());
					tex->create();

					if (i == 1)
						load_ind.first_frame = tex;

					load_ind.logo_frames.emplace_back(tex);

					const auto sz = (uint32_t)content.size();
					f.write(reinterpret_cast<const char *>(&sz), sizeof(sz));
					f.write(content.data(), sz);
				}

				load_ind.section_progress += fraction;
			}

			load_ind.begin_next_section();
			VIRTUALIZER_SHARK_WHITE_END;
		})
		.detach();
}
#endif

namespace script_db
{
struct cache_data_t
{
	std::string filename{};
	bool is_proprietary{};
	int id{};
	int last_update{};

	std::string name{};
	std::string author{};
	std::string description{};

	bool is_library{};
};

std::vector<cache_data_t> read()
{
	// read file
	std::vector<cache_data_t> data;

	const auto path = std::string(XOR("ev0lve/scripts/remote/cache.json"));
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open())
		return data;

	// parse file
	nlohmann::json j;
	file >> j;

	for (const auto &i : j)
	{
		cache_data_t d{i[XOR("filename")], i[XOR("is_proprietary")], i[XOR("id")],			i[XOR("last_update")],
					   i[XOR("name")],	   i[XOR("author")],		 i[XOR("description")], i[XOR("is_library")]};
		data.emplace_back(d);
	}

	return data;
}

void write(const std::vector<cache_data_t> &data)
{
	// write file
	const auto path = std::string(XOR("ev0lve/scripts/remote/cache.json"));
	std::ofstream file(path, std::ios::binary);

	if (!file.is_open())
		return;

	// parse file
	nlohmann::json j;
	for (const auto &i : data)
	{
		nlohmann::json d{{XOR_STR("filename"), i.filename},
						 {XOR_STR("is_proprietary"), i.is_proprietary},
						 {XOR_STR("id"), i.id},
						 {XOR_STR("last_update"), i.last_update},
						 {XOR_STR("name"), i.name},
						 {XOR_STR("author"), i.author},
						 {XOR_STR("description"), i.description},
						 {XOR_STR("is_library"), i.is_library}};
		j.push_back(d);
	}

	file << j;
}
} // namespace script_db

#ifdef CSGO_SECURE
void network::refresh_configs()
{
	VIRTUALIZER_SHARK_WHITE_START;

	cfg.remote_configs.clear();

	auto m = msg::message(msg::msg_refresh_configs);
	auto resp = MESSENGER()->add_and_wait(m, 30);
	if (!resp.message.id || resp.message.error.code != 0)
		return;

	const auto list = resp.message.get_data<std::vector<data::config>>();
	for (const auto &i : list)
	{
		cfg.remote_configs.push_back({i.id, i.name, i.content});
	}

	VIRTUALIZER_SHARK_WHITE_END;
}

void network::refresh_scripts(std::vector<lua::script_file> &files)
{
	VIRTUALIZER_SHARK_WHITE_START;

	// read local cache
	auto cache = script_db::read();

	const auto add_files = [&cache, &files]()
	{
		for (const auto &c : cache)
		{
			if (c.is_library)
				continue;

			if (!std::filesystem::exists(XOR("ev0lve/scripts/remote/") + c.filename))
				continue;

			lua::script_file f{lua::st_remote, c.filename.substr(0, c.filename.find(XOR(".lua")))};

			f.is_proprietary = c.is_proprietary;
			f.id = c.id;
			f.last_update = c.last_update;
			f.metadata.name = c.name;
			f.metadata.description = c.description;
			f.metadata.author = c.author;

			files.push_back(f);
		}
	};

	// compose delta to send
	std::vector<data::script_cached_info> delta;
	for (const auto &c : cache)
	{
		delta.push_back({c.id, c.last_update});
	}

	// compose message
	auto m = msg::message(msg::msg_refresh_scripts);
	m.set_data(delta);

	// send message and wait 2 minutes
	auto resp = MESSENGER()->add_and_wait(m, 120);
	if (!resp.message.id || resp.message.error.code != 0)
	{
		add_files();
		return;
	}

	// get response data
	const auto delta_in = resp.message.data.get<std::vector<data::script>>();
	if (delta_in.empty())
	{
		add_files();
		return;
	}

	for (const auto &d : delta_in)
	{
		data::script_cached_info tmp{d.id, d.last_update};
		m = msg::message(msg::msg_get_script_key);
		m.set_data(tmp);

		resp = MESSENGER()->add_and_wait(m, 30);
		if (!resp.message.id || resp.message.error.code != 0)
			continue;

		const auto key = resp.message.data.get<std::string>();

		auto it = std::find_if(cache.begin(), cache.end(), [&](const auto &c) { return c.id == d.id; });

		if (d.should_erase)
		{
			// remove from cache
			if (it != cache.end())
			{
				// delete from disk
				const auto path =
					std::string(XOR("ev0lve/scripts/") + std::string(d.is_library ? XOR("lib") : XOR("remote")) +
								XOR("/") + it->filename);
				if (std::filesystem::exists(path))
					std::filesystem::remove(path.c_str());
				cache.erase(it);
			}
		}
		else
		{
			const auto filename = d.is_library ? d.filename : (std::to_string(d.id) + XOR(".lua"));

			// check if not in the cache yet or needs update
			if (it == cache.end() || it->is_proprietary != d.is_proprietary || it->last_update != d.last_update)
			{
				// special case if it's a zip file
				if (d.filename.ends_with(XOR("zip")))
				{
					std::stringstream ss(d.content);
					miniz_cpp::zip_file zip(ss);

					// malformed package.
					if (!zip.has_file(XOR_STR("scripts/remote/script.lua")))
						continue;

					for (const auto &n : zip.infolist())
					{
						std::filesystem::create_directories(
							std::filesystem::path(XOR("ev0lve/") + n.filename).remove_filename());

						// encrypt lua file if it's proprietary
						auto content = zip.read(n);
						if (n.filename.ends_with(XOR("lua")) && n.filename.find(XOR("scripts/lib/")) != 0 &&
							d.is_proprietary)
							content = evo::util::aes256_encrypt(content, key);

						auto tmp_filename = n.filename;
						if (tmp_filename == XOR("scripts/remote/script.lua"))
							tmp_filename = XOR("scripts/remote/") + filename;

						// write file
						const auto path = std::string(XOR("ev0lve/") + tmp_filename);
						if (path.find(XOR("ev0lve/scripts/lib/")) != 0 || !std::filesystem::exists(path))
						{
							std::ofstream file(path, std::ios::binary);
							if (!file.is_open())
								continue;

							file << content;
						}
					}
				}
				else
				{
					const auto path = XOR("ev0lve/scripts/") + std::string(d.is_library ? XOR("lib") : XOR("remote")) +
									  XOR("/") + filename;

					// if proprietary then fetch encryption key
					if (d.is_proprietary)
					{
						// write script to file
						std::ofstream file(path, std::ios::binary);
						if (!file.is_open())
							continue;

						// encrypt script
						file << evo::util::aes256_encrypt(d.content, key);
					}
					else
					{
						// write script to file
						std::ofstream file(path, std::ios::binary);
						if (!file.is_open())
							continue;

						file << d.content;
					}
				}

				// add to cache
				if (it == cache.end())
				{
					cache.push_back({filename, d.is_proprietary, d.id, d.last_update, d.name, d.author, d.description,
									 d.is_library});
				}
				else
				{
					// update cache
					it->is_proprietary = d.is_proprietary;
					it->last_update = d.last_update;
					it->name = d.name;
					it->author = d.author;
					it->description = d.description;
				}
			}
		}
	}

	// write cache
	add_files();
	script_db::write(cache);

	VIRTUALIZER_SHARK_WHITE_END;
}

std::string network::decrypt_script(const std::string &filename, int id, int last_update)
{
	VIRTUALIZER_SHARK_WHITE_START;

	// read file contents
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
		return {};

	std::string content;
	file.seekg(0, std::ios::end);
	content.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// get encryption key
	auto m = msg::message(msg::msg_get_script_key);
	m.set_data(data::script_cached_info{id, last_update});
	auto resp = MESSENGER()->add_and_wait(m, 30);

	if (!resp.message.id || resp.message.error.code != 0)
		return {};

	const auto key = resp.message.data.get<std::string>();

	// decrypt script
	VIRTUALIZER_SHARK_WHITE_END;
	return evo::util::aes256_decrypt(content, key);
}

std::string network::get_build() { return NETWORK()->build; }

std::optional<network::item_data> network::get_item_data(int id, const std::string &item_id)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (!id)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::this_thread::yield();

		return {{XOR_STR("Test Item"), 199}};
	}

	auto m = msg::message(msg::msg_get_isp_info);
	m.set_data(data::isp_info{item_id, id});
	auto resp = MESSENGER()->add_and_wait(m, 30);
	if (!resp.message.id || resp.message.error.code != 0)
		return {};

	const auto data = resp.message.get_data<data::isp_info>();
	VIRTUALIZER_SHARK_WHITE_END;

	return {{data.name, data.price}};
}

int network::purchase_item(int id, const std::string &item_id)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (!id)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::this_thread::yield();

		return (int)data::purchase_error::ok;
	}

	auto m = msg::message(msg::msg_isp_purchase);
	m.set_data(data::isp_purchase{item_id, id});
	auto resp = MESSENGER()->add_and_wait(m, 30);
	if (!resp.message.id || resp.message.error.code != 0)
		return {};

	const auto result = resp.message.data.get<data::purchase_error>();
	VIRTUALIZER_SHARK_WHITE_END;

	return (int)result;
}

std::vector<std::string> network::get_purchased_items(int id)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (!id)
		return {};

	auto m = msg::message(msg::msg_isp_restore);
	m.set_data(data::isp_purchased{id});
	auto resp = MESSENGER()->add_and_wait(m, 30);
	if (!resp.message.id || resp.message.error.code != 0)
		return {};

	const auto result = resp.message.data.get<data::isp_purchased>();
	VIRTUALIZER_SHARK_WHITE_END;

	return result.purchased;
}

void network::listen(const std::string &id, const std::function<void()> &cbk)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (id.empty())
		return;

	auto m = msg::message(msg::msg_lua_data);
	m.set_data(data::lua_data{id, nlohmann::json{}, true, false, 0});

	const auto r = MESSENGER()->add_and_wait(m, 30);
	cbk();

	VIRTUALIZER_SHARK_WHITE_END;
}

void network::stop_listen(const std::string &id)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (id.empty())
		return;

	auto m = msg::message(msg::msg_lua_data);
	m.set_data(data::lua_data{id, nlohmann::json{}, false, true, 0});

	MESSENGER()->add(m);
	VIRTUALIZER_SHARK_WHITE_END;
}

void network::send(const std::string &id, const nlohmann::json &data)
{
	VIRTUALIZER_SHARK_WHITE_START;
	if (id.empty())
		return;

	auto m = msg::message(msg::msg_lua_data);
	m.set_data(data::lua_data{id, data, false, false, 0});

	MESSENGER()->add(m);
	VIRTUALIZER_SHARK_WHITE_END;
}
#endif