
#ifndef SDK_CLIENT_ENTITY_LIST_H
#define SDK_CLIENT_ENTITY_LIST_H

#include <sdk/entity.h>
#include <sdk/global_vars_base.h>
#include <sdk/hl_client.h>
#include <sdk/macros.h>
#include <sdk/view_render.h>

namespace sdk
{
class cs_player_t;

struct ent_info_t
{
	client_entity *entity;
	uint32_t serial;
	ent_info_t *prev;
	ent_info_t *next;
};

struct ent_info_list_t
{
	ent_info_t *head;
	ent_info_t *tail;
};

class client_entity_list_t
{
	inline static auto constexpr num_ent_entries = 8192;

	struct entity_iterator_t
	{
		using value_type = entity_t *;

		ent_info_t *ptr;
		ent_info_t *tail;

		inline entity_iterator_t &operator++()
		{
			if (ptr == tail || ptr->next > tail)
				ptr = nullptr;
			else
				ptr = ptr->next;
			return *this;
		}

		inline entity_t *operator->() const { return (entity_t *)ptr->entity; }

		inline entity_t *operator*() const { return (entity_t *)ptr->entity; }

		inline bool operator==(entity_iterator_t const &other) const { return ptr == other.ptr; }

		inline bool operator!=(entity_iterator_t const &other) const { return ptr != other.ptr; }
	};

	struct player_iterator_t : public entity_iterator_t
	{
		inline cs_player_t *operator->() const { return (cs_player_t *)ptr->entity; }

		inline cs_player_t *operator*() const { return (cs_player_t *)ptr->entity; }
	};

	struct players_t
	{
		client_entity_list_t *parent;

		players_t(client_entity_list_t *parent) : parent(parent) {}

		[[nodiscard]] __forceinline player_iterator_t begin()
		{
			auto world = &((ent_info_t *)parent)[0 - (num_ent_entries + 1)];
			return {world->next && world->next->entity ? world->next : nullptr,
					&((ent_info_t *)parent)[64 - (num_ent_entries + 1)]};
		}

		[[nodiscard]] __forceinline player_iterator_t end()
		{
			// begin->prev holds the last entity reachable.
			return {nullptr, nullptr};
		}
	};

public:
	[[nodiscard]] __forceinline int32_t get_highest_entity_index() { return *(uint32_t *)(uintptr_t(this) + 0x24); }

	[[nodiscard]] __forceinline ent_info_list_t *get_active_list()
	{
		return (ent_info_list_t *)(uintptr_t(this) - 0x10);
	}

	[[nodiscard]] __forceinline client_entity *get_client_entity(uint32_t ent)
	{
		if (ent >= 0 && ent < num_ent_entries)
			return ((ent_info_t *)this)[ent - (num_ent_entries + 1)].entity;

		return nullptr;
	}

	[[nodiscard]] __forceinline client_entity *get_client_entity_from_handle(base_handle e)
	{
		full_handle ent{.full = e};
		if (ent.full == -1)
			return nullptr;

		auto &inf = ((ent_info_t *)this)[ent.index - (num_ent_entries + 1)];
		if (inf.serial == ent.serial)
			return inf.entity;

		return nullptr;
	}

	[[nodiscard]] __forceinline entity_iterator_t begin()
	{
		auto active = get_active_list();
		return {active->head ? active->head : nullptr, active->tail};
	}

	[[nodiscard]] __forceinline entity_iterator_t end()
	{
		// begin->prev holds the last entity reachable.
		return {nullptr, nullptr};
	}

	[[nodiscard]] __forceinline ent_info_t *get_next_info(ent_info_t *info)
	{
		const auto end = &((ent_info_t *)this)[num_ent_entries - (num_ent_entries + 1)];
		for (info++; end > info; info++)
			if (info->entity)
				return info;
		return nullptr;
	}

	inline players_t get_players() { return players_t(this); }

	template <typename f> inline void for_each(f fn)
	{
		for (auto ent : *this)
			fn(ent);
	}

	template <typename f> inline void for_each_z(f fn)
	{
		if (!game->view_render)
			return;

		const auto view_setup = game->view_render->get_view_setup();

		if (!view_setup)
			return;

		static std::vector<std::pair<entity_t *, float>> order;
		order.clear();

		for (auto ent : *this)
			order.push_back(std::make_pair(ent, (ent->get_abs_origin() - view_setup->origin).length2d()));

		std::sort(order.begin(), order.end(),
				  [](const std::pair<entity_t *, float> &a, const std::pair<entity_t *, float> &b) -> bool
				  { return a.second > b.second; });

		for (auto const &ent : order)
			fn(ent.first);
	}

	template <typename f> inline void for_each_player(f fn)
	{
		for (auto ent : get_players())
			fn(reinterpret_cast<cs_player_t *>(ent));
	}
};
} // namespace sdk

#endif // SDK_CLIENT_ENTITY_LIST_H
