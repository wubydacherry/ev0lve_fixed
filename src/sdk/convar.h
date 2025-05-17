
#ifndef SDK_CONVAR_H
#define SDK_CONVAR_H

#include <base/game.h>
#include <sdk/color.h>
#include <sdk/macros.h>

#define FCVAR_NONE 0
#define FCVAR_UNREGISTERED (1 << 0) // If this is set, don't add to linked list, etc.
#define FCVAR_DEVELOPMENTONLY                                                                                          \
	(1 << 1) // Hidden in released products. Flag is removed automatically if ALLOW_DEVELOPMENT_CVARS is defined.
#define FCVAR_GAMEDLL (1 << 2)	 // defined by the game DLL
#define FCVAR_CLIENTDLL (1 << 3) // defined by the client DLL
#define FCVAR_HIDDEN                                                                                                   \
	(1 << 4) // Hidden. Doesn't appear in find or autocomplete. Like DEVELOPMENTONLY, but can't be compiled out.

// ConVar only
#define FCVAR_PROTECTED                                                                                                \
	(1 << 5) // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not
			 // bland/zero, 0 otherwise as value
#define FCVAR_SPONLY (1 << 6)	// This cvar cannot be changed by clients connected to a multiplayer server.
#define FCVAR_ARCHIVE (1 << 7)	// set to cause it to be saved to vars.rc
#define FCVAR_NOTIFY (1 << 8)	// notifies players when changed
#define FCVAR_USERINFO (1 << 9) // changes the client's info string
#define FCVAR_CHEAT (1 << 14)	// Only useable in singleplayer / debug / multiplayer & sv_cheats

#define FCVAR_PRINTABLEONLY                                                                                            \
	(1 << 10) // This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
#define FCVAR_UNLOGGED                                                                                                 \
	(1 << 11) // If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
#define FCVAR_NEVER_AS_STRING (1 << 12) // never try to print that cvar

// It's a ConVar that's shared between the client and the server.
// At signon, the values of all such ConVars are sent from the server to the client (skipped for local
//  client, of course )
// If a change is requested it must come from the console (i.e., no remote client changes)
// If a value is changed while a server is active, it's replicated to all connected clients
#define FCVAR_REPLICATED (1 << 13) // server setting enforced on clients, TODO rename to FCAR_SERVER at some time
#define FCVAR_DEMO (1 << 16)	   // record this cvar when starting a demo file
#define FCVAR_DONTRECORD (1 << 17) // don't record these command in demofiles

#define FCVAR_NOT_CONNECTED (1 << 22) // cvar cannot be changed by a client that is connected to a server

#define FCVAR_ARCHIVE_XBOX (1 << 24) // cvar written to config.cfg on the Xbox

#define FCVAR_SERVER_CAN_EXECUTE                                                                                       \
	(1 << 28) // the server is allowed to execute this command on clients via
			  // ClientCommand/NET_StringCmd/CBaseClientState::ProcessStringCmd.
#define FCVAR_SERVER_CANNOT_QUERY                                                                                      \
	(1 << 29) // If this is set, then the server is not allowed to query this cvar's value (via
			  // IServerPluginHelpers::StartQueryCvarValue).
#define FCVAR_CLIENTCMD_CAN_EXECUTE (1 << 30) // IVEngineClient::ClientCmd is allowed to execute this command.

namespace sdk
{
class convar;
}

namespace hooks::miscellaneous
{
void __cdecl convar_network_change_callback(sdk::convar *var, const char *old1, float old2);
}

namespace sdk
{
class c_command
{
public:
	enum
	{
		COMMAND_MAX_ARGC = 64,
		COMMAND_MAX_LENGTH = 512,
	};

	int m_nArgc;
	int m_nArgv0Size;
	char m_pArgSBuffer[COMMAND_MAX_LENGTH]{};
	char m_pArgvBuffer[COMMAND_MAX_LENGTH]{};
	const char *m_ppArgv[COMMAND_MAX_ARGC]{};

	explicit c_command(const char *command)
	{
		strcpy(m_pArgSBuffer, command);
		strcpy(m_pArgvBuffer, command);

		m_nArgc = 1;
		m_ppArgv[0] = m_pArgvBuffer;

		for (size_t i = 0; i < strlen(m_pArgSBuffer); i++)
		{
			if (m_pArgvBuffer[i] == ' ')
			{
				m_pArgvBuffer[i] = 0;
				m_ppArgv[m_nArgc] = &m_pArgvBuffer[i + 1]; // dont ever call that with a space at the end.
				m_nArgc++;
			}
		}
		m_nArgv0Size = strlen(m_pArgvBuffer);
	}
};

struct convar_value_t
{
	char *string{};
	int32_t string_length{};
	float value{};
	int32_t n_val{};
};

class convar
{
public:
	// NOTE: These are in IConVar, do not call them unless you know what you are doing.
	VIRTUAL(5, get_base_name(), const char *(__thiscall *)(convar *))();
	VIRTUAL(6, has_flag(uint32_t flag), bool(__thiscall *)(convar *, uint32_t))(flag);

	VIRTUAL(14, set_string_internal(const char *v), void(__thiscall *)(convar *, const char *))(v);
	VIRTUAL(15, set_float_internal(const float v), void(__thiscall *)(convar *, float))(v);
	VIRTUAL(16, set_int_internal(const int32_t v), void(__thiscall *)(convar *, int32_t))(v);

	inline void set_string(const char *v)
	{
		set_string_internal(v);
		orig_value = value;
	}

	inline void set_float(float v)
	{
		set_float_internal(v);
		orig_value = value;
	}

	inline void set_int(int32_t v)
	{
		set_int_internal(v);
		orig_value = value;
	}

	inline float get_float()
	{
		const auto val = *reinterpret_cast<uint32_t *>(&value.value);
		auto xored = static_cast<uint32_t>(val ^ reinterpret_cast<uint32_t>(this));
		return *reinterpret_cast<float *>(&xored);
	}

	inline int32_t get_int()
	{
		const auto val = *reinterpret_cast<uint32_t *>(&value.n_val);
		auto xored = static_cast<uint32_t>(val ^ reinterpret_cast<uint32_t>(this));
		return *reinterpret_cast<int32_t *>(&xored);
	}

	inline void enforce_sent_value(bool &current, bool sent)
	{
		if (current == sent)
			return;

		char val[] = {sent ? '1' : '0', '\0'};
		const auto bak = value.string;
		value.string = val;
		orig_value.string = val;
		hooks::miscellaneous::convar_network_change_callback(reinterpret_cast<convar *>(uintptr_t(this) + 0x18),
															 nullptr, NAN);
		value.string = bak;
		orig_value.string = val;
		current = sent;
	}

	char pad0[0x4]{};
	convar *next{};
	int32_t orig_flags{};
	char *name{};
	char *help_string{};
	int32_t flags{};
	char pad1[0x4]{};
	convar *parent{};
	char *default_value{};

	convar_value_t value{};
	convar_value_t orig_value{};

	int32_t has_min{};
	float min_val{};
	int32_t has_max{};
	float max_val{};
	void *change_callback{};
};

class concommand
{
public:
	VIRTUAL(14, dispatch(), void(__thiscall *)(concommand *, void *))(nullptr);
};

class cvar_t
{
public:
	VIRTUAL(16, find_var(const char *name), convar *(__thiscall *)(void *, const char *))(name);
	VIRTUAL(18, find_command(const char *name), concommand *(__thiscall *)(void *, const char *))(name);
	VIRTUAL(25, print(color &clr, const char *text), void (*)(void *, const color &, const char *, ...))(clr, text);

	inline void unlock()
	{
		const auto first = **(convar ***)(uintptr_t(this) + 0x34);
		for (auto cur = first->next; cur != nullptr; cur = cur->next)
		{
			cur->flags &= ~(FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN);
			cur->orig_flags = cur->flags | 1;
		}
	}
};
} // namespace sdk

#define GET_CONVAR_INT(name)                                                                                           \
	[]() -> int32_t                                                                                                    \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->get_int();                                                                                         \
	}()
#define GET_CONVAR_FLOAT(name)                                                                                         \
	[]() -> float                                                                                                      \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->get_float();                                                                                       \
	}()
#define GET_CONVAR_STR(name)                                                                                           \
	[]() -> char *                                                                                                     \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->value.string;                                                                                      \
	}()
#define SET_CONVAR_INT(name, val)                                                                                      \
	[&]()                                                                                                              \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->set_int(val);                                                                                      \
	}()
#define SET_CONVAR_FLOAT(name, val)                                                                                    \
	[&]()                                                                                                              \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->set_float(val);                                                                                    \
	}()
#define SET_CONVAR_STR(name, val)                                                                                      \
	[&]()                                                                                                              \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::convar>(game->cvar->find_var(XOR_STR(name)));                 \
		return var->set_string(val);                                                                                   \
	}()
#define DISPATCH_COMMAND(name)                                                                                         \
	[&]()                                                                                                              \
	{                                                                                                                  \
		static const auto var = util::encrypted_ptr<sdk::concommand>(game->cvar->find_command(XOR_STR(name)));         \
		var->dispatch();                                                                                               \
	}()

#endif // SDK_CONVAR_H
