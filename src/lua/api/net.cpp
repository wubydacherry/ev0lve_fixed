#include <base/cfg.h>
#include <lua/api_def.h>
#include <lua/helpers.h>
#include <network/helpers.h>

#include <wininet.h>

namespace lua::api_def::net
{
int http_get(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({
		{ltd::string},
		{ltd::string},
		{ltd::function},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.http_get(url, headers, callback(response))"));
		return 0;
	}

	if (!cfg.lua.allow_insecure.get())
	{
		s.error(XOR_STR("http_get is not available with Allow insecure disabled"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	const auto url = helpers::parse_uri(s.get_string(1));
	const auto headers = std::string(s.get_string(2));
	const auto cbk = s.registry_add();

	if (!url)
	{
		s.error(XOR_STR("invalid url specified"));
		return 0;
	}

	// run this in a separate thread to avoid blocking.
	std::thread(
		[l, cbk, url, headers, me]()
		{
			const auto inet = InternetOpenA(XOR_STR("ev0lve"), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
			if (!inet)
				return;

			const auto conn =
				InternetConnectA(inet, url->host.c_str(), url->port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 1);
			if (!conn)
			{
				InternetCloseHandle(inet);
				return;
			}

			const auto req =
				HttpOpenRequestA(conn, XOR_STR("GET"), url->path.c_str(), nullptr, nullptr, nullptr,
								 INTERNET_FLAG_KEEP_CONNECTION | (url->is_secure ? INTERNET_FLAG_SECURE : 0), 1);
			if (!req)
			{
				InternetCloseHandle(conn);
				InternetCloseHandle(inet);
				return;
			}

			runtime_state s(l);
			if (HttpSendRequestA(req, headers.c_str(), headers.size(), nullptr, 0))
			{
				std::string result;

				char buffer[1024]{};
				unsigned long bytes_read{};
				while (InternetReadFile(req, buffer, 1024, &bytes_read) && bytes_read)
				{
					result += std::string(buffer, bytes_read);
					memset(buffer, 0, 1024);
				}

				InternetCloseHandle(req);
				InternetCloseHandle(conn);
				InternetCloseHandle(inet);

				s.registry_get(cbk);
				s.push(result);
				if (!s.call(1, 0))
				{
					me->did_error = true;
					lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
					if (const auto f = api.find_script_file(me->id); f.has_value())
						f->get().should_unload = true;

					return;
				}
			}

			s.registry_remove(cbk);
		})
		.detach();

	return 0;
}

int http_post(lua_State *l)
{
	runtime_state s(l);
	const auto r = s.check_arguments({
		{ltd::string},
		{ltd::string},
		{ltd::string},
		{ltd::function},
	});

	if (!r)
	{
		s.error(XOR_STR("usage: utils.http_post(url, headers, body, callback(response))"));
		return 0;
	}

	if (!cfg.lua.allow_insecure.get())
	{
		s.error(XOR_STR("http_get is not available with Allow insecure disabled"));
		return 0;
	}

	const auto me = api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

	const auto url = helpers::parse_uri(s.get_string(1));
	const auto headers = std::string(s.get_string(2));
	const auto body = std::string(s.get_string(3));
	const auto cbk = s.registry_add();

	if (!url)
	{
		s.error(XOR_STR("invalid url specified"));
		return 0;
	}

	// run this in a separate thread to avoid blocking.
	std::thread(
		[l, cbk, url, headers, body, me]()
		{
			const auto inet = InternetOpenA(XOR_STR("ev0lve"), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
			if (!inet)
				return;

			const auto conn =
				InternetConnectA(inet, url->host.c_str(), url->port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 1);
			if (!conn)
			{
				InternetCloseHandle(inet);
				return;
			}

			const auto req =
				HttpOpenRequestA(conn, XOR_STR("POST"), url->path.c_str(), nullptr, nullptr, nullptr,
								 INTERNET_FLAG_KEEP_CONNECTION | (url->is_secure ? INTERNET_FLAG_SECURE : 0), 1);
			if (!req)
			{
				InternetCloseHandle(conn);
				InternetCloseHandle(inet);
				return;
			}

			runtime_state s(l);
			if (HttpSendRequestA(req, headers.c_str(), headers.size(), (void *)body.c_str(), body.size()))
			{
				std::string result;

				char buffer[1024]{};
				unsigned long bytes_read{};
				while (InternetReadFile(req, buffer, 1024, &bytes_read) && bytes_read)
				{
					result += std::string(buffer, bytes_read);
					memset(buffer, 0, 1024);
				}

				InternetCloseHandle(req);
				InternetCloseHandle(conn);
				InternetCloseHandle(inet);

				s.registry_get(cbk);
				s.push(result);
				if (!s.call(1, 0))
				{
					me->did_error = true;
					lua::helpers::error(XOR_STR("runtime_error"), s.get_last_error_description().c_str());
					if (const auto f = api.find_script_file(me->id); f.has_value())
						f->get().should_unload = true;

					return;
				}
			}

			s.registry_remove(cbk);
		})
		.detach();

	return 0;
}

int listen(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::function}}))
	{
		s.error(XOR_STR("usage: net.listen(id: string, cbk: function())"));
		return 0;
	}

	const auto &me = lua::api.find_by_state(l);
	if (!me)
	{
		s.error(XOR_STR("FATAL: could not find the script. Did it escape the sandbox?"));
		return 0;
	}

#if 0
	const std::string id{s.get_string(1)};
	const auto fn = s.registry_add();
	std::thread(
		[l, fn, id, me]()
		{
			runtime_state s(l);
			network::listen(id,
							[&]()
							{
								s.registry_get(fn);
								if (!s.call(0, 0))
								{
									me->did_error = true;
									lua::helpers::error(XOR_STR("runtime_error"),
														s.get_last_error_description().c_str());
									if (const auto f = api.find_script_file(me->id); f.has_value())
										f->get().should_unload = true;

									return;
								}
								s.registry_remove(fn);
							});
		})
		.detach();
#endif
	return 0;
}

int broadcast(lua_State *l)
{
	runtime_state s(l);
	if (!s.check_arguments({{ltd::string}, {ltd::table}}))
	{
		s.error(XOR_STR("usage: net.broadcast(id: string, data: table)"));
		return 0;
	}

	//network::send(s.get_string(1), helpers::parse_table(l, 2));
	return 0;
}
} // namespace lua::api_def::net
