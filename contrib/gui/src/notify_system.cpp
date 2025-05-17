#include <gui/controls/layout.h>
#include <gui/controls/notification_control.h>
#include <gui/gui.h>
#include <gui/notify_system.h>

GUI_NAMESPACE;

void notification_system::clear()
{
	std::lock_guard _lock(data_mutex);
	for (const auto &d : data)
	{
		d->expiry.direct(5.f, 5.f);
		remove(d->id);
	}
}

void notification_system::add(const std::shared_ptr<notification> &item)
{
	std::lock_guard _lock(data_mutex);
	for (const auto &d : data)
		d->pos.direct(d->pos.end + popup_rect.height() + 8.f);

	// auto-set id
	item->id = data.size() + 1;

	if (data.empty())
		data.emplace_back(item);
	else
		data.emplace(data.begin(), item);

	auto odd = false;
	if (const auto nc_list = ctx->find(GUI_HASH("gui_nc_list")))
	{
		const auto arr = nc_list->as<layout>();

		arr->clear();
		for (const auto &n : data)
			arr->add(std::make_shared<notification_control>(control_id{GUI_HASH("gui_nc_n") + n->id}, odd = !odd, n));
	}
}

void notification_system::remove(uint64_t id) { remove_queue.emplace_back(id); }

void notification_system::process_removal_queue()
{
	for (auto i = remove_queue.begin(); i != remove_queue.end();)
	{
		const auto &d = *i;

		std::lock_guard _lock(data_mutex);
		const auto p =
			std::find_if(data.begin(), data.end(), [d](const std::shared_ptr<notification> &n) { return n->id == d; });

		if (p != data.end())
		{
			const auto &exp = (*p)->expiry;
			const auto &alpha = (*p)->alpha;
			if (exp.value == exp.end && alpha.value == 0.f)
			{
				data.erase(p);
				i = remove_queue.erase(i);
				continue;
			}
		}

		i++;
	}
}

std::optional<std::shared_ptr<notification>> notification_system::find(uint64_t id)
{
	std::lock_guard _lock(data_mutex);
	const auto p =
		std::find_if(data.begin(), data.end(), [id](const std::shared_ptr<notification> &n) { return n->id == id; });

	return p == data.end() ? std::nullopt : std::make_optional(*p);
}

void notification_system::for_each(const std::function<void(const std::shared_ptr<notification> &)> &fn)
{
	std::lock_guard _lock(data_mutex);
	for (const auto &n : data)
		fn(n);
}

void notification_system::render()
{
	auto &d = draw.layers[l_last];
	const vec2 cur{draw.display.x - popup_rect.width(), 40.f};

	const auto f_bold = draw.fonts[GUI_HASH("gui_bold")]; //gui_bald
	const auto f_main = draw.fonts[GUI_HASH("gui_main")];

	for_each(
		[cur, f_bold, f_main](const std::shared_ptr<notification> &n)
		{
			n->pos.animate();
			n->alpha.animate();
			n->expiry.animate();

			if (n->expiry.value == n->expiry.end && !n->has_toggled_expiry)
			{
				n->has_toggled_expiry = true;
				n->pos.direct(n->pos.value + 20.f);
				n->alpha.direct(0.f);
			}

			const auto r = popup_rect.translate(cur).translate({0.f, n->pos.value});

			const auto old_alpha = d->g.alpha;
			d->g.alpha = n->alpha.value;
			d->add_shadow_rect(r, 6.f);

			add_with_blur(d, r, [&r](auto &d) { d->add_rect_filled(r, color::white()); });
			d->add_rect_filled(r, colors.bg_bottom.mod_a(.8f));
			d->add_rect_filled(r.width(2.f), colors.accent);

			d->font = f_main;// f_bold;
			d->add_text((r.tl() + vec2{8.f, 4.f}).floor(), n->header, colors.text_light);
			d->font = f_main;
			d->add_text((r.tl() + vec2{8.f, 24.f}).floor(), n->text, colors.text);
			d->g.alpha = old_alpha;
		});
}

int notification_system::get_count() const { return (int)data.size(); }

void notification_system::for_each_reverse(const std::function<void(const std::shared_ptr<notification> &)> &fn)
{
	std::lock_guard _lock(data_mutex);
	for (auto n = data.rbegin(); n != data.rend(); n++)
		fn(*n);
}
