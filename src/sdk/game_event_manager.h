
#ifndef SDK_GAME_EVENT_MANAGER_H
#define SDK_GAME_EVENT_MANAGER_H

#include <sdk/keyvalues.h>
#include <sdk/macros.h>

namespace sdk
{
class game_event
{
public:
	virtual ~game_event() = default;
	virtual const char *get_name() const = 0;

	virtual bool is_reliable() const = 0;
	virtual bool is_local() const = 0;
	virtual bool is_empty(const char *keyname = nullptr) = 0;

	virtual bool get_bool(const char *keyname = nullptr, bool default_value = false) = 0;
	virtual int get_int(const char *keyname = nullptr, int default_value = 0) = 0;
	virtual uint64_t get_uint64(const char *keyname = nullptr, uint64_t default_value = 0) = 0;
	virtual float get_float(const char *keyname = nullptr, float default_value = 0.0f) = 0;
	virtual const char *get_string(const char *keyname = nullptr, const char *default_value = XOR_STR("")) = 0;
	virtual const wchar_t *get_wstring(const char *keyname = nullptr, const wchar_t *default_value = L"") = 0;
	virtual void func11() = 0;
	virtual void set_bool(const char *keyname = nullptr, bool value = false) = 0;
	virtual void set_int(const char *keyname = nullptr, int value = 0) = 0;
	virtual void set_uint64(const char *keyname = nullptr, uint64_t value = 0) = 0;
	virtual void set_float(const char *keyname = nullptr, float value = 0.0f) = 0;
	virtual void set_string(const char *keyname = nullptr, const char *value = nullptr) = 0;
	virtual void set_wstring(const char *keyname = nullptr, const wchar_t *value = nullptr) = 0;

	void *descriptor;
	keyvalues *kv;
};

class game_event_listener
{
public:
	virtual ~game_event_listener() = default;

	virtual void fire_game_event(game_event *event) = 0;
	virtual int get_event_debug_id() = 0;
};

class game_event_manager_t
{
public:
	virtual ~game_event_manager_t() = 0;
	virtual int load_events_from_file(const char *filename) = 0;
	virtual void reset() = 0;
	virtual bool add_listener(game_event_listener *listener, const char *name, bool serverside) = 0;
	virtual bool find_listener(game_event_listener *listener, const char *name) = 0;
	virtual void remove_listener(game_event_listener *listener) = 0;
	virtual void add_listener_global(game_event_listener *listener, bool serverside) = 0;
};
} // namespace sdk

#endif // SDK_GAME_EVENT_MANAGER_H
