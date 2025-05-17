
#ifndef SDK_MISC_H
#define SDK_MISC_H

// clang-format off
#include <sdk/vec3.h>
#include <sdk/macros.h>
#include <sdk/cs_player.h>
#include <sdk/engine_trace.h>
#include <sdk/econ_item.h>
#include <sdk/model_info_client.h>
#include <sdk/client_state.h>
#include <base/cfg.h>
// clang-format on

namespace sdk
{
class hardware_config_t
{
public:
	VIRTUAL(50, set_hdr_enable(const bool active), void(__thiscall *)(void *, bool))(active);
};

class model_loader_t
{
public:
	void override_last_loaded_hdr_state(bool val)
	{
		*reinterpret_cast<bool *>(reinterpret_cast<uintptr_t>(this) + 0x1B1) = val;
	}
};

class leaf_system_t
{
public:
	VIRTUAL(0, create_renderable_handle(void *obj), void(__thiscall *)(void *, uintptr_t, int, char, int, int))
	(reinterpret_cast<uintptr_t>(obj) + 0x4, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
	VIRTUAL(1, remove_renderable(base_handle handle), void(__thiscall *)(void *, base_handle))(handle);
	VIRTUAL(15, renderable_changed(const client_render_handle handle), void(__thiscall *)(void *, client_render_handle))
	(handle);
};

class image_data
{
public:
	image_data(std::vector<uint32_t> &v)
	{
		const auto root = game->panorama.at(functions::image_data::pat);
		memset(pad, 0, 0x2c);
		*reinterpret_cast<uintptr_t *>(uintptr_t(this)) = uintptr_t(v.data());
		*reinterpret_cast<uint32_t *>(uintptr_t(this) + 4) = sizeof(uint32_t) * v.size();
		*reinterpret_cast<uintptr_t *>(uintptr_t(this) + 0x24) = *reinterpret_cast<uintptr_t *>(root - 12);
		*reinterpret_cast<uintptr_t *>(uintptr_t(this) + 0x28) = *reinterpret_cast<uintptr_t *>(root - 4);
	}

	bool load_png(const uint8_t *img, uint32_t *w, uint32_t *h)
	{
		auto f = game->panorama.at(functions::image_data::load_png);
		auto ret = false;

		__asm
		{
				mov edx, 0xFFFFFF
				push h
				push w
				lea ecx, pad
				push ecx
				mov ecx, img
				call f
				add esp, 0xC
				mov ret, al
		}

		return ret;
	}

	bool load_svg(const uint8_t *img, const size_t s, uint32_t *w, uint32_t *h)
	{
		auto f = game->panorama.at(functions::image_data::load_svg);
		uint16_t magic[] = {0x33, 0x34, 0x35, 0x5c, 0x37, 0x33, 0x30, 0x5c, 0x6c, 0x6f, 0x63, 0x61,
							0x6c, 0x5c, 0x70, 0x61, 0x6e, 0x6f, 0x72, 0x61, 0x6d, 0x61, 0x00};
		auto scl = 1.f;
		auto ret = false;

		__asm
		{
				mov edx, s
				lea ecx, magic
				push ecx
				push scl
				push h
				push w
				lea ecx, pad
				push ecx
				mov ecx, img
				call f
				add esp, 0x14
				mov ret, al
		}

		return ret;
	}

private:
	uint8_t pad[0x2c];
};

class modifier_table_t
{
private:
	FUNC(game->client.at(functions::modifier_table::find), find(uint16_t *symbol, char *lookup),
		 uint16_t *(__thiscall *)(void *, uint16_t *, char *))
	(symbol, lookup);
	FUNC(game->client.at(functions::modifier_table::add_string), add_string(uint16_t *symbol, char *lookup),
		 uint16_t *(__thiscall *)(void *, uint16_t *, char *))
	(symbol, lookup);

public:
	uint16_t find_or_insert(char *lookup)
	{
		uint16_t symbol = 0xFFFF;
		find(&symbol, lookup);
		if (symbol == 0xFFFF)
			symbol = *add_string(&symbol, lookup);

		return symbol;
	}
};

inline uint8_t *load_text_file(const char *name, size_t *len)
{
	const auto ret = ((uint8_t * (__thiscall *)(const char *)) game->client.at(functions::load_text_file))(name);

	if (ret)
		*len = strlen((const char *)ret);

	return ret;
}

struct str_table
{
	VIRTUAL(8, add_string(bool is_server, const char *val),
			int32_t(__thiscall *)(void *, bool, const char *, int32_t, void *))
	(is_server, val, -1, nullptr);
};

class string_table_t
{
private:
	VIRTUAL(3, find_table(const char *name), str_table *(__thiscall *)(void *, const char *))(name);

public:
	void precache_model(const char *name)
	{
		auto table = find_table(XOR_STR("modelprecache"));

		if (table)
		{
			game->model_info_client->find_or_load_model(name);
			table->add_string(false, name);
		}
	}
};

class game_console_t
{
public:
	VIRTUAL(5, is_console_visible(), bool(__thiscall *)(void *))();
};

inline uintptr_t find_hud_element(const char *name)
{
	return reinterpret_cast<uintptr_t(__thiscall *)(uintptr_t, const char *)>(
		game->client.at(functions::find_hud_element))(game->client.at(globals::hud_instance), name);
}

inline void draw_sphere(const vec3 &pos, float rad, const color &clr, int32_t theta_phi = 12)
{
	((void(__cdecl *)())game->client.at(functions::draw_sphere::initialize))();
	((void(__cdecl *)(const void *, float, int32_t, int32_t, uint32_t, uintptr_t, bool))game->engine.at(
		functions::draw_sphere::fn))(&pos, rad, theta_phi, theta_phi, clr.encoded(),
									 *(uintptr_t *)game->client.at(functions::draw_sphere::material), true);
}

template <class t> struct interpolated_value
{
	t target = {};
	t current = {};
	float remaining_frame_time = {};

	inline void update(t target, float remaining_frame_time)
	{
		this->target = target;
		this->remaining_frame_time = remaining_frame_time;
	}

	inline t advance(float frame_time)
	{
		const auto fraction = frame_time / remaining_frame_time;
		remaining_frame_time -= frame_time;

		if (remaining_frame_time <= 0.f)
		{
			current = target;
			remaining_frame_time = 0.f;
		}
		else
			current = interpolate(current, target, fraction);

		return current;
	}
};
} // namespace sdk

#endif // SDK_MISC_H
