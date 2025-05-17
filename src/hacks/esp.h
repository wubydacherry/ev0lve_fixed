
#ifndef HACKS_ESP_H
#define HACKS_ESP_H

#include <gui_legacy/gui.h>
#include <sdk/cs_player.h>
#include <sdk/weapon.h>

namespace hacks
{
using esp_box_t = std::pair<gui_legacy::vec2, gui_legacy::vec2>;

struct esp_item
{
	enum item_type
	{
		item_type_text,
		item_type_icon,
		item_type_bar,
		item_type_max,
	};

	static esp_item text(const std::string &text, const sdk::color &col)
	{
		return {item_type_text, text, XOR_STR(""), col, 0.f, 0.f};
	}

	static esp_item icon(const std::string &path, const sdk::color &col)
	{
		return {item_type_icon, path, XOR_STR(""), col, 0.f, 0.f};
	}

	static esp_item bar(float fill, float fill_old, const sdk::color &col)
	{
		return {item_type_bar, XOR_STR(""), XOR_STR(""), col, fill, fill_old};
	}

	static esp_item bar(float fill, float fill_old, const std::string &extra, const sdk::color &col)
	{
		return {item_type_bar, XOR_STR(""), extra, col, fill, fill_old};
	}

	item_type type;
	std::string content;
	std::string content_extra;
	sdk::color color;
	float fill;
	float fill_old;
};

enum esp_side
{
	esp_side_left,
	esp_side_right,
	esp_side_top,
	esp_side_bottom,
	esp_side_max,
};

class esp_builder
{
public:
	void draw(const gui_legacy::draw_adapter &draw, const esp_box_t &box, float fractional, float clr_a);

	void add(esp_side side, const esp_item &item)
	{
		(item.type == esp_item::item_type_bar ? bars[side] : items[side]).emplace_back(item);
	}

protected:
	std::array<std::vector<esp_item>, esp_side_max> bars;
	std::array<std::vector<esp_item>, esp_side_max> items;
};

class esp : public gui_legacy::drawable
{
	friend class esp_builder;

public:
	void draw(const gui_legacy::draw_adapter &draw) final;

	std::string get_player_name(sdk::cs_player_t *const player) const;
	void shorten_str(std::string &str) const;
	void shorten_str(std::wstring &str) const;

private:
	void draw_player(const gui_legacy::draw_adapter &draw, sdk::cs_player_t *const ply) const;
	void draw_player_offscreen(const gui_legacy::draw_adapter &draw, sdk::cs_player_t *const ply) const;
	void draw_entity(const gui_legacy::draw_adapter &draw, sdk::entity_t *const ent) const;
	sdk::vec2 draw_nade_impact(const gui_legacy::draw_adapter &draw, sdk::weapon_t *const wpn,
							   bool molotov = false) const;

	void draw_box(const gui_legacy::draw_adapter &draw, const esp_box_t &box, const sdk::color &clr,
				  bool drop_shadow = true) const;
	void draw_bar(const gui_legacy::draw_adapter &draw, const esp_box_t &box, const sdk::color &clr, float percent,
				  float old_percent, std::optional<std::string> text = std::nullopt) const;
	void draw_bones(const gui_legacy::draw_adapter &draw, sdk::cs_player_t *player, const sdk::color &clr) const;

	uint32_t calculate_distance_to_object(sdk::entity_t *const entity) const;
	float estimate_nade_damage_fraction(sdk::cs_player_t *const thrower, sdk::cs_player_t *const player,
										const sdk::vec3 &impact, bool molotov) const;

	std::optional<esp_box_t> get_player_bounds(sdk::cs_player_t *const player) const;
	bool is_point_in_viewport(sdk::vec3 &vec) const;
	void normalize_box(esp_box_t &box) const;
	sdk::vec2 rotate_offscreen(sdk::vec3 pos, float offset, float &rotation) const;

	std::array<std::pair<sdk::entity_t *, float>, size_t(sdk::class_id::spore_trail) + 1> closest_entity{};
};

extern std::shared_ptr<esp> es;
} // namespace hacks

#endif // HACKS_ESP_H
