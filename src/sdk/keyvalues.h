
#ifndef SDK_KEY_VALUES_H
#define SDK_KEY_VALUES_H

#include <base/game.h>
#include <sdk/mem_alloc.h>

namespace sdk
{
class net_message;

class key_values_system_t
{
public:
	VIRTUAL(4, get_symbol_for_string(const char *name, bool create), int(__thiscall *)(void *, const char *, bool))
	(name, create);
	VIRTUAL(5, get_symbol_for_string(int symbol), const char *(__thiscall *)(void *, int))(symbol);
};

class keyvalues
{
private:
	FUNC(game->client.at(functions::keyvalues::init), init(const char *name),
		 void(__thiscall *)(void *, const char *, int, int))
	(name, 0, 0);
	FUNC(game->client.at(functions::keyvalues::load_from_buffer),
		 load_from_buffer(char const *name, const char *buffer),
		 bool(__thiscall *)(void *, const char *, const char *, void *, void *, void *, void *))
	(name, buffer, nullptr, nullptr, nullptr, nullptr);
	FUNC(game->client.at(functions::keyvalues::set_string_int), set_string_int(const char *value),
		 void(__thiscall *)(void *, const char *))
	(value);

public:
	enum types_t
	{
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_UINT64,
		TYPE_COMPILED_INT_BYTE,
		TYPE_COMPILED_INT_0,
		TYPE_COMPILED_INT_1,
		TYPE_NUMTYPES,
	};

	static inline keyvalues *produce(const char *name, const char *type, const char *shader)
	{
		const auto kv = reinterpret_cast<keyvalues *const>(game->mem_alloc->alloc(44));
		kv->init(type);
		kv->load_from_buffer(name, shader);
		return kv;
	}

	static inline keyvalues *from_msg(uintptr_t msg)
	{
		return ((keyvalues * (__thiscall *)(uintptr_t)) game->engine.at(functions::keyvalues::from_msg))(msg);
	}

	inline const char *get_name() const
	{
		return game->key_values_system->get_symbol_for_string((uint16_t(key_name_case_sensitive2) << 8) |
															  uint8_t(key_name_case_sensitive1));
	}

	inline keyvalues *find_key(const char *key_name)
	{
		if (!key_name || !key_name[0])
			return this;

		auto sub_str = strchr(key_name, '/');
		auto search_str = key_name;

		if (sub_str)
		{
			int size = sub_str - key_name;
			auto buf = reinterpret_cast<char *>(alloca(size + 1));
			memcpy(buf, key_name, size);
			buf[size] = 0;
			if (strlen(key_name) > 1)
				search_str = buf;
		}

		int32_t search_str_idx = game->key_values_system->get_symbol_for_string(search_str, false);
		if (search_str_idx == -1)
			return nullptr;

		keyvalues *last_item = nullptr;
		auto dat = sub;
		for (; dat != nullptr; dat = dat->peer)
		{
			last_item = dat;

			if (dat->key_name == uint32_t(search_str_idx))
				break;
		}

		if (!dat && chain)
			dat = chain->find_key(key_name);

		if (!dat)
			return nullptr;

		if (sub_str)
			return dat->find_key(sub_str + 1);

		return dat;
	}

	inline keyvalues *get_first_value()
	{
		auto ret = sub;
		while (ret && ret->data_type == TYPE_NONE)
			ret = ret->peer;
		return ret;
	}

	inline keyvalues *get_next_value()
	{
		auto ret = peer;
		while (ret && ret->data_type == TYPE_NONE)
			ret = ret->peer;
		return ret;
	}

	inline void *get_ptr(const char *key = nullptr, void *def = nullptr)
	{
		if (auto dat = find_key(key); dat)
		{
			switch (dat->data_type)
			{
			case TYPE_FLOAT:
			case TYPE_INT:
			case TYPE_PTR:
				return dat->p_value;
			case TYPE_WSTRING:
				return dat->ws_value;
			case TYPE_STRING:
				return dat->s_value;
			case TYPE_UINT64:
			default:
				break;
			};
		}

		return def;
	}

	inline void set_string(const char *name, const char *value)
	{
		auto key = find_key(name);
		if (key)
			key->set_string_int(value);
	}

	keyvalues() = delete;

	inline ~keyvalues() { game->mem_alloc->free(this); }

	uint32_t key_name : 24;
	uint32_t key_name_case_sensitive1 : 8;

	char *s_value = nullptr;
	wchar_t *ws_value = nullptr;

	union
	{
		int i_value;
		float f_value;
		void *p_value;
		uint8_t color[4];
	};

	char data_type = 0;
	char has_escape_sequences = 0;
	uint16_t key_name_case_sensitive2 = 0;

	char pad[8]{};
	keyvalues *peer = nullptr;
	keyvalues *sub = nullptr;
	keyvalues *chain = nullptr;

	uintptr_t expression_get_symbol_proc = 0;
};
} // namespace sdk

#endif // SDK_KEY_VALUES_H
