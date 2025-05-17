
#ifndef HACKS_CHAMS_H
#define HACKS_CHAMS_H

#include <base/cfg.h>
#include <sdk/cs_player.h>
#include <sdk/model_render.h>

namespace hacks
{
class chams
{
	inline static const auto $ignorez = XOR_STR_STORE("$ignorez");
	inline static const auto $color = XOR_STR_STORE("$color");
	inline static const auto $alpha = XOR_STR_STORE("$alpha");
	inline static const auto $envmaptint = XOR_STR_STORE("$envmaptint");
	inline static const auto wpns = XOR_STR_STORE("weapons/v_");
	inline static const auto arms = XOR_STR_STORE("arms");

	inline static const auto vertexlit = XOR_STR_STORE("VertexLitGeneric");
	inline static const auto unlit = XOR_STR_STORE("UnlitGeneric");

	inline static const auto default_mat = XOR_STR_STORE(R"#("VertexLitGeneric"
		{
			"$basetexture"			  "vgui/white"
			"$model"				  "1"
			"$selfillum"			  "1"
			"$nocull"				  "0"
			"$nofog"				  "1"
			"$halflambert"			  "1"
		})#");
	inline static const auto flat_mat = XOR_STR_STORE(R"#("UnlitGeneric"
		{
			"$basetexture"			  "vgui/white"
			"$model"				  "1"
			"$selfillum"			  "1"
			"$nocull"				  "0"
			"$nofog"				  "1"
			"$halflambert"			  "1"
		})#");
	inline static const auto glow_mat = XOR_STR_STORE(R"#("VertexLitGeneric"
		{
			"$basetexture"			  "vgui/white"
			"$envmap"                 "models/effects/cube_white"
			"$envmaptint"             "[1. 1. 1.]"
			"$envmapfresnel"          "1"
			"$envmapfresnelminmaxexp" "[0 2 2]"
			"$model"				  "1"
			"$selfillum"			  "1"
			"$nocull"				  "0"
			"$nofog"				  "1"
			"$halflambert"			  "1"
		})#");
	inline static const auto pulse_mat = XOR_STR_STORE(R"#("VertexLitGeneric"
		{
			"$basetexture"			  "vgui/white"
			"$envmap"                 "models/effects/cube_white"
			"$envmaptint"             "[1. 1. 1.]"
			"$envmapfresnel"          "1"
			"$envmapfresnelminmaxexp" "[0 2 2]"
			"$model"				  "1"
			"$selfillum"			  "1"
			"$nocull"				  "0"
			"$nofog"				  "1"
			"$halflambert"			  "1"

			"proxies"
			{
				"sine"
				{
					"sineperiod" 1.6
					"sinemin" .8
					"sinemax" 2.6
					"resultvar" "$envmapfresnelminmaxexp[1]"
				}
			}
		})#");
	inline static const auto acryl_mat = XOR_STR_STORE(R"#("VertexLitGeneric"
		{
			"$basetexture"			  "vgui/white"
			"$bumpmap"                "de_nuke/hr_nuke/pool_water_normals_002"
			"$bumptransform"          "center 0.5 0.5 scale 0.25 0.25 rotate 0.0 translate 0.0 0.0"
		    "$color2"                 "[1. 1. 1.]"
			"$envmap"                 "env_cubemap"
			"$envmaptint"             "[.1 .1 .1]"
			"$envmapfresnel"          "1."
			"$envmapfresnelminmaxexp" "[1. 1. 1.]"
			"$model"				  "1"
			"$selfillum"			  "1"
			"$nocull"				  "0"
			"$nofog"				  "1"
			"$halflambert"			  "1"
			"$phong"                  "1"
			"$phongexponent"          "1024"
			"$phongboost"             "4."
			"$phongfresnelranges"     "[1. 1. 1.]"
			"$rimlight"               "1"
			"$rimlightexponent"       "4."
			"$rimlightboost"          "2."
		})#");

public:
	enum cham_material
	{
		enemy_material,
		enemy_attachment_material,
		enemy_vis_material,
		enemy_vis_attachment_material,
		enemy_his_material,
		friend_material,
		friend_attachment_material,
		friend_vis_material,
		friend_vis_attachment_material,
		local_material,
		local_attachment_material,
		fake_material,
		arm_material,
		weapon_material,
		max_material
	};

	chams();
	~chams();

	void on_draw_model(sdk::model_render_info_t &info, const std::function<void(sdk::mat3x4 *)> draw);
	void extend_chams();
	void reset();
	void reload(bool exit = false);

	sdk::material *maintain_material(cfg_t::chams &chams, int32_t index);
	int32_t index_material(cfg_t::chams &chams);

	alignas(16) sdk::mat3x4 override_mat[sdk::max_bones];
	bool has_override_mat{};

private:
	void draw_fake_chams(const std::function<void(sdk::mat3x4 *)> draw);
	void perform_player(sdk::model_render_info_t &info, sdk::cs_player_t *player,
						const std::function<void(sdk::mat3x4 *)> draw, bool is_attachment = false);
	void perform_viewmodel(const bool rendering_arms);

	sdk::material *materials[max_material];
	int8_t modes[max_material];
	uint32_t clr1[max_material];
	uint32_t clr2[max_material];

	bool should_reset{};
};

extern chams cham;
} // namespace hacks

#endif // HACKS_CHAMS_H
