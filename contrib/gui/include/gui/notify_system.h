#ifndef NOTIFY_SYSTEM_56CBF92B0A0843E2B3890B123FD334AF_H
#define NOTIFY_SYSTEM_56CBF92B0A0843E2B3890B123FD334AF_H

#include <functional>
#include <memory>
#include <mutex>
#include <optional>

#include <ren/types/animator.h>

namespace evo::gui
{
class notification
{
public:
	notification(const std::string &h, const std::string &t) : header(h), text(t)
	{
		pos.direct(0.f);
		alpha.direct(1.f);
		expiry.direct(5.f);
	}

	std::string header{};
	std::string text{};

	uint64_t id{};

	ren::anim_float pos = ren::anim_float(-20.f, 0.25f, ren::ease_in_out);
	ren::anim_float alpha = ren::anim_float(0.f, 0.25f, ren::ease_in_out);

	bool has_toggled_expiry{};
	ren::anim_float expiry = ren::anim_float(0.f, 5.f);
};

class notification_system
{
	inline static const ren::rect popup_rect = {0.f, 0.f, 200.f, 46.f};

public:
	void clear();

	int get_count() const;

	void add(const std::shared_ptr<notification> &item);
	void remove(uint64_t id);

	void render();
	void process_removal_queue();

	void for_each(const std::function<void(const std::shared_ptr<notification> &)> &fn);
	void for_each_reverse(const std::function<void(const std::shared_ptr<notification> &)> &fn);
	std::optional<std::shared_ptr<notification>> find(uint64_t id);

private:
	std::mutex data_mutex{};
	std::vector<uint64_t> remove_queue{};
	std::vector<std::shared_ptr<notification>> data{};
};
} // namespace evo::gui

#endif // NOTIFY_SYSTEM_56CBF92B0A0843E2B3890B123FD334AF_H
