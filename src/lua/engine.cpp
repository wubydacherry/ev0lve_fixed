#include <base/game.h>
#include <filesystem>
#include <gui/controls.h>
#include <lua/engine.h>
#include <sdk/surface.h>
#include <util/fnv1a.h>
#include <util/misc.h>

#include <fstream>

#ifdef CSGO_SECURE
#include <network/helpers.h>
#include <network/load_indicator.h>
#endif

using namespace evo;

void lua::engine::refresh_scripts()
{
	if (!std::filesystem::exists(XOR_STR("ev0lve/scripts")))
		std::filesystem::create_directories(XOR_STR("ev0lve/scripts"));
	if (!std::filesystem::exists(XOR_STR("ev0lve/scripts/remote")))
		std::filesystem::create_directories(XOR_STR("ev0lve/scripts/remote"));
	if (!std::filesystem::exists(XOR_STR("ev0lve/scripts/lib")))
		std::filesystem::create_directories(XOR_STR("ev0lve/scripts/lib"));

	script_files.clear();

#if defined(CSGO_SECURE) && !defined(_DEBUG)
	network::refresh_scripts(script_files);
#endif

	// get total file counter for progress bar
	int total_files{};
	for (const auto &p : std::filesystem::directory_iterator(XOR_STR("ev0lve/scripts")))
		if (p.path().extension() == XOR_STR(".lua"))
			++total_files;

	for (const auto &p : std::filesystem::directory_iterator(XOR_STR("ev0lve/scripts/lib")))
		if (p.path().extension() == XOR_STR(".lua"))
			++total_files;

	const auto progress_fraction = (1.f / (float)total_files) * 0.5f;
	for (auto &f : std::filesystem::directory_iterator(XOR_STR("ev0lve/scripts")))
	{
		if (f.is_directory() || f.path().extension() != XOR_STR(".lua"))
			continue;

		script_file file{st_script, f.path().filename().replace_extension(XOR_STR("")).string()};

		file.parse_metadata();
		script_files.emplace_back(file);

#if defined(CSGO_SECURE) && !defined(_DEBUG)
		network::load_ind.section_progress += progress_fraction;
#endif
	}

	for (auto &f : std::filesystem::directory_iterator(XOR_STR("ev0lve/scripts/lib")))
	{
		if (f.is_directory() || f.path().extension() != XOR_STR(".lua"))
			continue;

		script_file file{st_library, f.path().filename().replace_extension(XOR_STR("")).string()};

		file.parse_metadata();
		script_files.emplace_back(file);

#if defined(CSGO_SECURE) && !defined(_DEBUG)
		network::load_ind.section_progress += progress_fraction;
#endif
	}

	// remove scripts that are no longer in the list.
	std::lock_guard _lock(access_mutex);
	for (const auto &script : scripts)
	{
		const auto p = std::find_if(script_files.begin(), script_files.end(),
									[&](const script_file &f) { return f.make_id() == script.first; });

		if (p != script_files.end())
			continue;

		scripts[script.first]->unload();
	}
}

void lua::engine::run_autoload()
{
	std::vector<uint32_t> unload{};
	for (const auto &[id, s] : scripts)
	{
		if (std::find(autoload.begin(), autoload.end(), id) == autoload.end())
			unload.emplace_back(id);
	}

	for (const auto &id : unload)
	{
		if (const auto f = api.find_script_file(id); f.has_value())
			f->get().should_unload = true;
		else
			stop_script(id);
	}

	for (const auto &f_id : autoload)
	{
		const auto file = std::find_if(script_files.begin(), script_files.end(),
									   [f_id](const script_file &f) { return f.make_id() == f_id; });

		if (file == script_files.end())
			continue;

		const auto scr = find_by_id(file->make_id());
		if (file != script_files.end() && (!scr || !scr->is_running))
			run_script(*file, false);
	}

	const auto script_list = gui::ctx->find(GUI_HASH("scripts.general.list"));
	if (script_list)
		script_list->as<gui::list>()->for_each_control(
			[&](std::shared_ptr<gui::control> &c)
			{
				const auto sel = c->as<gui::selectable>();
				if (lua::api.exists(sel->id))
					sel->is_loaded = true;
				else
					sel->is_loaded = false;

				sel->reset();
			});
}

void lua::engine::enable_autoload(const script_file &file)
{
	if (std::find(autoload.begin(), autoload.end(), file.make_id()) != autoload.end())
		return;

	autoload.emplace_back(file.make_id());
}

void lua::engine::disable_autoload(const script_file &file)
{
	if (const auto f = std::find(autoload.begin(), autoload.end(), file.make_id()); f != autoload.end())
		autoload.erase(f);
}

bool lua::engine::is_autoload_enabled(const script_file &file)
{
	return std::find(autoload.begin(), autoload.end(), file.make_id()) != autoload.end();
}

bool lua::engine::run_script(const script_file &file, bool sounds)
{
	if (file.type == st_library)
		return false;

	std::lock_guard _lock(access_mutex);

	// make sure file still exists
	if (!std::filesystem::exists(file.get_file_path()))
	{
		if (sounds)
			EMIT_ERROR_SOUND();

		// show the error message to user
		const auto msg = std::make_shared<gui::message_box>(evo::gui::control_id{GUI_HASH("script.error"), ""},
															XOR_STR("Script error"), XOR_STR("File not found."));
		msg->open();

		return false;
	}

	// delete previously running script
	const auto id = file.make_id();
	if (scripts.find(id) != scripts.end())
		scripts.erase(id);

	// create script (lua will auto-initialize)
	const auto s = std::make_shared<script>();
	s->name = file.get_name();
	s->id = id;
	s->type = file.type;
	s->file = file.get_file_path();
	s->remote.is_proprietary = file.is_proprietary;
	s->remote.last_update = file.last_update;
	s->remote.id = file.id;

	// add to the list
	scripts[s->id] = s;

	// init the script
	if (!s->initialize() || !s->call_main())
	{
		if (sounds)
			EMIT_ERROR_SOUND();

		// show the error message to user
		const auto msg =
			std::make_shared<gui::message_box>(evo::gui::control_id{GUI_HASH("script.error"), ""},
											   XOR_STR("Script error"), XOR_STR("Unable to initialize script."));
		msg->open();

		s->is_running = false;
		s->unload();

		scripts.erase(s->id);
		return false;
	}

	// mark as running
	s->is_running = true;

	if (sounds)
		EMIT_SUCCESS_SOUND();

	return true;
}

void lua::engine::stop_script(const script_file &file)
{
	// "unload" script (lua state will be auto-erased)
	stop_script(file.make_id());
}

void lua::engine::stop_script(uint32_t id)
{
	std::lock_guard _lock(access_mutex);
	if (const auto s = scripts.find(id); s != scripts.end() && s->second)
	{
		if (!s->second->did_error)
			s->second->call_forward(FNV1A("on_shutdown"));

		s->second->is_running = false;
		s->second->unload();
		scripts.erase(id);
	}
}

void lua::engine::for_each_script_name(const std::function<void(script_file &)> &fn)
{
	for (auto &f : script_files)
	{
		if (f.type != st_library)
			fn(f);
	}
}

std::optional<std::reference_wrapper<lua::script_file>> lua::engine::find_script_file(uint32_t id)
{
	const auto p = std::find_if(script_files.begin(), script_files.end(),
								[id](const script_file &f) { return f.make_id() == id; });

	if (p != script_files.end())
		return std::ref(*p);

	return {};
}

void lua::engine::callback(uint32_t id, const std::function<int(state &)> &arg_callback, int ret,
						   const std::function<void(state &)> &callback)
{
	try
	{
		std::lock_guard _lock(access_mutex);
		for (const auto &[_, s] : scripts)
		{
			if (s->is_running)
				s->call_forward(id, arg_callback, ret, callback);
		}
	}
	catch (...)
	{
	}
}

void lua::engine::create_callback(const char *n)
{
	std::lock_guard _lock(access_mutex);
	for (const auto &[_, s] : scripts)
	{
		if (!s->has_forward(util::fnv1a(n)))
			s->create_forward(n);
	}
}

void lua::engine::stop_all()
{
	std::lock_guard _lock(access_mutex);
	scripts.clear();
}

void lua::engine::run_timers()
{
	std::lock_guard _lock(access_mutex);
	for (const auto &[_, s] : scripts)
	{
		if (s->is_running)
			s->run_timers();
	}
}

lua::script_t lua::engine::find_by_state(lua_State *state)
{
	for (const auto &[_, s] : scripts)
	{
		if (s->l.get_state() == state)
			return s;
	}

	return nullptr;
}

lua::script_t lua::engine::find_by_id(uint32_t id)
{
	for (const auto &[_, s] : scripts)
	{
		if (_ == id)
			return s;
	}

	return nullptr;
}

uint32_t lua::script_file::make_id() const { return util::fnv1a(name.c_str()) ^ type; }

std::string lua::script_file::get_file_path() const
{
	std::string base_path{XOR_STR("ev0lve/")};
	switch (type)
	{
	case st_script:
		base_path += XOR_STR("scripts/");
		break;
	case st_remote:
		base_path += XOR_STR("scripts/remote/");
		break;
	case st_library:
		base_path += XOR_STR("scripts/lib/");
		break;
	default:
		return XOR_STR("");
	}

	return base_path + name + XOR_STR(".lua");
}

void lua::script_file::parse_metadata()
{
	std::ifstream file(get_file_path());
	if (!file.is_open())
		return;

	std::string line{};
	while (std::getline(file, line))
	{
		// check our own shebang notation
		if (line.find(XOR_STR("--.")) != 0)
			continue;

		// remove shebang
		line = line.erase(0, 3);

		// split in parts
		const auto parts = util::split(line, XOR_STR(" "));
		if (parts.empty())
			continue;

		const auto item = util::fnv1a(parts[0].c_str());
		switch (item)
		{
		case FNV1A("name"):
			if (line.size() < 5)
				continue;
			metadata.name = line.substr(5);
			break;
		case FNV1A("description"):
			if (line.size() < 12)
				continue;
			metadata.description = line.substr(12);
			break;
		case FNV1A("author"):
			if (line.size() < 7)
				continue;
			metadata.author = line.substr(7);
			break;
		default:
			break;
		}
	}
}