
#ifndef SDK_MODEL_RENDER_H
#define SDK_MODEL_RENDER_H

#include <sdk/client_entity.h>
#include <sdk/color.h>
#include <sdk/cs_player.h>
#include <sdk/keyvalues.h>
#include <sdk/macros.h>
#include <sdk/mat.h>
#include <sdk/model_info_client.h>
#include <sdk/vec2.h>
#include <sdk/vec3.h>
#include <sdk/vec4.h>

class IDirect3DTexture9;

namespace sdk
{
inline constexpr auto studio_render = 1;

enum material_primitive_type
{
	material_points,
	material_lines,
	material_triangles,
	material_triangle_strip,
	material_line_strip,
	material_line_loop,
	material_polygon,
	material_quads,
	material_subd_quads_extra,
	material_subd_quads_reg,
	material_instanced_quads,
	material_heterogenous
};

struct model
{
	uintptr_t pad;
	char name[255];
};

struct model_render_info_t
{
	vec3 origin;
	angle angles;
	char pad[0x4];
	client_renderable *renderable;
	const model *model;
	const mat3x4 *model_to_world;
	const mat3x4 *lighting_offset;
	const vec3 *lighting_origin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	uint32_t instance;
};

class material_var
{
	VIRTUAL(10, set_vector_internal(const float x, const float y), void(__thiscall *)(void *, float, float))(x, y);
	VIRTUAL(11, set_vector_internal(const float x, const float y, const float z),
			void(__thiscall *)(void *, float, float, float))
	(x, y, z);

public:
	VIRTUAL(4, set_float(const float val), void(__thiscall *)(void *, float))(val);
	VIRTUAL(5, set_int(const int val), void(__thiscall *)(void *, int))(val);
	VIRTUAL(6, set_string(const char *val), void(__thiscall *)(void *, const char *))(val);
	VIRTUAL(7, get_string(), const char *(__thiscall *)(void *))();
	VIRTUAL(20, set_matrix(viewmat &matrix), void(__thiscall *)(void *, viewmat &))(matrix);
	VIRTUAL(26, set_vector_component(const float val, const int comp), void(__thiscall *)(void *, float, int))
	(val, comp);
	VIRTUAL(27, get_int(), int(__thiscall *)(void *))();
	VIRTUAL(28, get_float(), float(__thiscall *)(void *))();
	VIRTUAL(29, get_vector(), float *(__thiscall *)(void *))();
	VIRTUAL(31, get_vector_size(), int(__thiscall *)(void *))();

	void set_vector(const vec2 vector) { set_vector_internal(vector.x, vector.y); }

	void set_vector(const vec3 vector) { set_vector_internal(vector.x, vector.y, vector.z); }
};

enum material_var_flags
{
	material_var_debug = 1 << 0,
	material_var_no_debug_override = 1 << 1,
	material_var_no_draw = 1 << 2,
	material_var_use_in_fillrate_mode = 1 << 3,
	material_var_vertexcolor = 1 << 4,
	material_var_vertexalpha = 1 << 5,
	material_var_selfillum = 1 << 6,
	material_var_additive = 1 << 7,
	material_var_alphatest = 1 << 8,
	material_var_znearer = 1 << 10,
	material_var_model = 1 << 11,
	material_var_flat = 1 << 12,
	material_var_nocull = 1 << 13,
	material_var_nofog = 1 << 14,
	material_var_ignorez = 1 << 15,
	material_var_decal = 1 << 16,
	material_var_envmapsphere = 1 << 17,
	material_var_envmapcameraspace = 1 << 19,
	material_var_basealphaenvmapmask = 1 << 20,
	material_var_translucent = 1 << 21,
	material_var_normalmapalphaenvmapmask = 1 << 22,
	material_var_needs_software_skinning = 1 << 23,
	material_var_opaquetexture = 1 << 24,
	material_var_envmapmode = 1 << 25,
	material_var_suppress_decals = 1 << 26,
	material_var_halflambert = 1 << 27,
	material_var_wireframe = 1 << 28,
	material_var_allowalphatocoverage = 1 << 29,
	material_var_alpha_modified_by_proxy = 1 << 30,
	material_var_vertexfog = 1 << 31
};

struct renderable_info
{
	client_renderable *renderable;
	uintptr_t alpha_prop;
	int32_t enum_count;
	int32_t render_frame;
	uint16_t first_shadow;
	uint16_t first_leaf;
	int16_t area;
	uint16_t flags;
	uint16_t render_fast_reflection : 1;
	uint16_t disable_shadow_depth_rendering : 1;
	uint16_t disable_csm_rendering : 1;
	uint16_t disable_shadow_depth_caching : 1;
	uint16_t split_screen_enabled : 2;
	uint16_t translucency_type : 2;
	uint16_t model_type : 8;
	vec3 bloated_abs_mins;
	vec3 bloated_abs_maxs;
	vec3 abs_mins;
	vec3 abs_maxs;
	int32_t pad;
};

struct texture_handle_t
{
	char pad[0xC];
	IDirect3DTexture9 *texture;
};

class texture
{
public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
	VIRTUAL(3, get_actual_width(), int32_t(__thiscall *)(void *))();
	VIRTUAL(4, get_actual_height(), int32_t(__thiscall *)(void *))();
	VIRTUAL(10, increment_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(11, decrement_reference_count(), void(__thiscall *)(void *))();

	char pad[0x50];
	texture_handle_t **handles;
};

class material;

struct stencil_state
{
	bool enable = true;
	uint32_t fail = 1;
	uint32_t zfail = 3;
	uint32_t pass = 1;
	uint32_t cmp = 8;
	int32_t reference = 1;
	uint32_t test = 0xFFFFFFFF;
	uint32_t write = 0xFFFFFFFF;
};

class mat_render_context
{
public:
	VIRTUAL(2, begin_render(), void(__thiscall *)(void *))();
	VIRTUAL(3, end_render(), void(__thiscall *)(void *))();
	VIRTUAL(5, bind_local_cubemap(texture *tex), void(__thiscall *)(void *, texture *))(tex);
	VIRTUAL(6, set_render_target(texture *t), void(__thiscall *)(void *, texture *))(t);
	VIRTUAL(7, get_render_target(), texture *(__thiscall *)(void *))();
	VIRTUAL(12, clear_buffers(bool col, bool depth, bool stencil), void(__thiscall *)(void *, bool, bool, bool))
	(col, depth, stencil);
	VIRTUAL(18, set_ambient_light_cube(vec4 *cube), void(__thiscall *)(void *, vec4 *))(cube);
	VIRTUAL(41, get_view_port(int32_t *x, int32_t *y, int32_t *width, int32_t *height),
			void(__thiscall *)(void *, int32_t *, int32_t *, int32_t *, int32_t *))
	(x, y, width, height);
	VIRTUAL(79, clear_color_4_ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a),
			void(__thiscall *)(void *, uint8_t, uint8_t, uint8_t, uint8_t))
	(r, g, b, a);
	VIRTUAL(114,
			draw_screen_space_rectangle(material *mat, int x, int y, int w, int h, float text_x0, float text_y0,
										float text_x1, float text_y1, int text_w, int text_h,
										void *renderable = nullptr, int x_dice = -1, int y_dice = -1),
			void(__thiscall *)(void *, material *, int, int, int, int, float, float, float, float, int, int, void *,
							   int, int))
	(mat, x, y, w, h, text_x0, text_y0, text_x1, text_y1, text_w, text_h, renderable, x_dice, y_dice);
	VIRTUAL(119, push_render_target_and_viewport(), void(__thiscall *)(void *))();
	VIRTUAL(120, pop_render_target_and_viewport(), void(__thiscall *)(void *))();
	VIRTUAL(126, set_int_rendering_parameter(int param, int value), void(__thiscall *)(void *, int, int))(param, value);
	VIRTUAL(128, set_stencil_state(stencil_state *state), void(__thiscall *)(void *, stencil_state *))(state);
	VIRTUAL(154, set_tone_mapping_scale_linear(vec3 *vec), void(__thiscall *)(void *, vec3 *))(vec);
	VIRTUAL(155, get_tone_mapping_scale_linear(), vec3(__thiscall *)(void *))();
	VIRTUAL(158, set_lighting_origin(const vec3 &origin), void(__thiscall *)(void *, vec3))(origin);

	inline material *get_current_material() { return *reinterpret_cast<material **>(uintptr_t(this) + 0xC); }
	inline void set_current_material(material *mat) { *reinterpret_cast<material **>(uintptr_t(this) + 0xC) = mat; }

	inline material *get_override_material() { return *reinterpret_cast<material **>(uintptr_t(this) + 0x24C); }
	inline void set_override_material(material *mat) { *reinterpret_cast<material **>(uintptr_t(this) + 0x24C) = mat; }
};

class shader
{
public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
};

class material
{
	VIRTUAL(28, color_modulate_internal(const float r, const float g, const float b),
			void(__thiscall *)(void *, float, float, float))
	(r, g, b);

public:
	VIRTUAL(0, get_name(), const char *(__thiscall *)(void *))();
	VIRTUAL(1, get_group(), const char *(__thiscall *)(void *))();
	VIRTUAL(12, increment_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(13, decrement_reference_count(), void(__thiscall *)(void *))();
	VIRTUAL(27, alpha_modulate(const float a), void(__thiscall *)(void *, float))(a);
	VIRTUAL(28, color_modulate(const float r, const float g, const float b),
			void(__thiscall *)(void *, float, float, float))
	(r, g, b);
	VIRTUAL(29, set_material_var_flag(const material_var_flags flag, const bool on),
			void(__thiscall *)(void *, material_var_flags, bool))
	(flag, on);
	VIRTUAL(30, get_material_var_flag(const material_var_flags flag), bool(__thiscall *)(void *, material_var_flags))
	(flag);
	VIRTUAL(37, refresh(), void(__thiscall *)(void *))();
	VIRTUAL(47, find_var_fast(char const *name, uint32_t *token),
			material_var *(__thiscall *)(void *, const char *, uint32_t *))
	(name, token);
	VIRTUAL(68, get_shader(), shader *(__thiscall *)(void *))();

	inline void modulate(const color clr)
	{
		color_modulate_internal(clr.red() / 255.f, clr.green() / 255.f, clr.blue() / 255.f);
		alpha_modulate(clr.alpha() / 255.f);
	}
};

struct mesh_instance_data
{
	int32_t index_offset;
	int32_t index_count;
	int32_t bone_count;
	uintptr_t bone_remap;
	mat3x4 *pose;
	texture *env_cubemap;
	uintptr_t lighting_state;
	material_primitive_type prim_type;
	uintptr_t vertex;
	int32_t vertex_offset;
	uintptr_t index;
	uintptr_t color;
	int32_t color_offset;
	uintptr_t stencil_state;
	quaternion diffuse_modulation;
	int32_t lightmap_page_id;
	bool indirect_lighting_only;
};

class model_render_t
{
public:
	VIRTUAL(0,
			draw_model(int32_t flags, client_renderable *ren, model_instance_handle inst, int32_t index, const model *m,
					   const vec3 *org, const angle *ang, int32_t skin, int32_t body, int32_t set,
					   mat3x4 *to_world = nullptr, mat3x4 *lighting = nullptr),
			int32_t(__thiscall *)(void *, int32_t, client_renderable *, model_instance_handle, int32_t, const model *,
								  const vec3 *, const angle *, int32_t, int32_t, int32_t, mat3x4 *, mat3x4 *))
	(flags, ren, inst, index, m, org, ang, skin, body, set, to_world, lighting);
	VIRTUAL(1, forced_material_override(material *mat), void(__thiscall *)(void *, material *, int, int))(mat, 0, -1);
	VIRTUAL(4, create_instance(client_renderable *ren, void *cache = nullptr),
			model_instance_handle(__thiscall *)(void *, client_renderable *, void *))
	(ren, cache);
	VIRTUAL(5, destroy_instance(model_instance_handle handle), void(__thiscall *)(void *, model_instance_handle))
	(handle);
	VIRTUAL(20,
			draw_model_setup(mat_render_context *ctx, model_render_info_t *info, draw_model_state *state, mat3x4 **out),
			void(__thiscall *)(void *, mat_render_context *, model_render_info_t *, draw_model_state *state, mat3x4 **))
	(ctx, info, state, out);
	VIRTUAL(24, suppress_engine_lighting(bool suppress), void(__thiscall *)(void *, bool))(suppress);
};

enum render_target_size_mode
{
	rt_size_no_change,
	rt_size_default,
	rt_size_picmip,
	rt_size_hdr,
	rt_size_full_frame_buffer,
	rt_size_offscreen,
	rt_size_full_frame_buffer_rounded_up
};

enum material_render_target_depth
{
	mat_rt_depth_shared,
	mat_rt_depth_separate,
	mat_rt_depth_none,
	mat_rt_depth_only
};

enum compiled_vtf_flags
{
	textureflags_pointsample = 0x00000001,
	textureflags_trilinear = 0x00000002,
	textureflags_clamps = 0x00000004,
	textureflags_clampt = 0x00000008,
	textureflags_anisotropic = 0x00000010,
	textureflags_hint_dxt5 = 0x00000020,
	textureflags_pwl_corrected = 0x00000040,
	textureflags_normal = 0x00000080,
	textureflags_nomip = 0x00000100,
	textureflags_nolod = 0x00000200,
	textureflags_all_mips = 0x00000400,
	textureflags_procedural = 0x00000800,
	textureflags_onebitalpha = 0x00001000,
	textureflags_eightbitalpha = 0x00002000,
	textureflags_envmap = 0x00004000,
	textureflags_rendertarget = 0x00008000,
	textureflags_depthrendertarget = 0x00010000,
	textureflags_nodebugoverride = 0x00020000,
	textureflags_singlecopy = 0x00040000,
	textureflags_srgb = 0x00080000,
	textureflags_default_pool = 0x00100000,
	textureflags_combined = 0x00200000,
	textureflags_async_download = 0x00400000,
	textureflags_nodepthbuffer = 0x00800000,
	textureflags_skip_initial_download = 0x01000000,
	textureflags_clampu = 0x02000000,
	textureflags_vertextexture = 0x04000000,
	textureflags_ssbump = 0x08000000,
	textureflags_most_mips = 0x10000000,
	textureflags_border = 0x20000000,
};

class material_system_t
{
public:
	VIRTUAL(36, get_back_buffer_format(), int(__thiscall *)(void *))();
	VIRTUAL(83, create_material(const char *name, keyvalues *kv),
			material *(__thiscall *)(void *, const char *, keyvalues *))
	(name, kv);
	VIRTUAL(84, find_material(const char *mat, const char *group = nullptr),
			material *(__thiscall *)(void *, const char *, const char *, bool, const char *))
	(mat, group, true, nullptr);
	VIRTUAL(86, first_material(), uint16_t(__thiscall *)(void *))();
	VIRTUAL(87, next_material(uint16_t handle), uint16_t(__thiscall *)(void *, uint16_t))(handle);
	VIRTUAL(88, invalid_material(), uint16_t(__thiscall *)(void *))();
	VIRTUAL(89, get_material(uint16_t handle), material *(__thiscall *)(void *, uint16_t))(handle);
	VIRTUAL(91, find_texture(const char *name, const char *groupname, bool complain = true, int32_t flags = 0),
			texture *(__thiscall *)(void *, const char *, const char *, bool, int32_t))
	(name, groupname, complain, flags);
	VIRTUAL(94, begin_render_target_allocation(), void(__thiscall *)(void *))();
	VIRTUAL(95, end_render_target_allocation(), void(__thiscall *)(void *))();
	VIRTUAL(97,
			create_named_render_target_texture_ex(const char *name, int w, int h, int size_mode, int format, int depth,
												  uint32_t flags, uint32_t target_flags),
			texture *(__thiscall *)(void *, const char *, int, int, int, int, int, uint32_t, uint32_t))
	(name, w, h, size_mode, format, depth, flags, target_flags);
	VIRTUAL(115, get_render_context(), mat_render_context *(__thiscall *)(void *))();
	VIRTUAL(136, finish_render_target_allocation(), void(__thiscall *)(void *))();

	VAR(globals::material_system, disable_render_target_allocation_forever, bool);
};

class studio_render_t
{
public:
	VIRTUAL(20, set_ambient_light_colors(vec4 *cols), void(__thiscall *)(void *, vec4 *))(cols);
	VIRTUAL(22, set_local_lights(int num, void *desc), void(__thiscall *)(void *, int, void *))(num, desc);
};

struct mdl_sequence_layer_t
{
	int sequence;
	float weight;
};

inline constexpr auto max_studio_flex_controls = 96 * 4;

struct mdl_t
{
	char pad0[48];
	material *override;
	texture *env_cubemap;
	texture *hdr_env_cubemap;
	uint16_t handle;
	char pad1[8];
	color col;
	char pad2[2];
	int skin;
	int body;
	int sequence;
	int lod;
	float playback_rate;
	float time;
	float current_anim_end_time;
	float flex_controls[max_studio_flex_controls];
	vec3 view_target;
	bool world_space_view_target;
	bool use_sequence_playback_fps;
	char pad3[2];
	void *proxy_data;
	float time_basis_adjustment;
	char pad4[0x1C];
};

struct mdl_data_t
{
	mdl_t mdl;
	mat3x4 mdl_to_world;
	bool request_bone_merge_takeover;
};

class render_to_rt_helper_object_t
{
public:
	virtual void draw(const mat3x4 &root_to_world) = 0;
	virtual bool bounding_sphere(vec3 &center, float &radius) = 0;
	virtual texture *get_env_cube_map() = 0;
};

class merged_mdl_t : public render_to_rt_helper_object_t
{
public:
	virtual ~merged_mdl_t() {}
	virtual void set_mdl(uint16_t handle, void *owner, void *data) = 0;
	virtual void set_mdl(const char *name, void *owner, void *data) = 0;

	FUNC(game->client.at(functions::merged_mdl::init), init(), void(__thiscall *)(void *))();
	FUNC(game->client.at(functions::merged_mdl::set_merge_mdl), set_merge_mdl(const char *name),
		 uint16_t(__thiscall *)(void *, const char *, void *, void *, bool))
	(name, nullptr, nullptr, false);
	FUNC(game->client.at(functions::merged_mdl::setup_bones_for_attachment_queries),
		 setup_bones_for_attachment_queries(), void(__thiscall *)(void *))
	();

	mdl_data_t root;
	mdl_data_t *merge;
	int merge_alloc;
	int merge_grow;
	int merge_size;
	mdl_data_t *merge_els;
	float pose_parameters[24];
	void *layers;
	int layers_alloc;
	int layers_grow;
	int layers_size;
	void *layers_els;
};
} // namespace sdk

#endif // SDK_MODEL_RENDER_H
