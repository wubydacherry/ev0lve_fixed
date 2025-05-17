
#ifndef SDK_CLIENT_ENTITY_H
#define SDK_CLIENT_ENTITY_H

#include <sdk/macros.h>
#include <sdk/mat.h>
#include <sdk/recv.h>

namespace sdk
{
class entity_t;
class collideable;
class client_networkable;
class client_renderable;
class client_entity;
class client_thinkable;
class client_unknown;
class client_alpha_property;
class game_trace;

struct ray
{
	vec3_aligned start;
	vec3_aligned delta;
	vec3_aligned start_offset;
	vec3_aligned extents;

	const mat3x4 *root_move_parent{};

	bool is_ray{};
	bool is_swept{};

public:
	__forceinline void init(const vec3 src, const vec3 end)
	{
		*reinterpret_cast<vec3 *>(&delta) = end - src;
		is_swept = delta.length() != 0.f;
		extents.x = extents.y = extents.z = 0.0f;
		root_move_parent = nullptr;
		is_ray = true;
		start_offset.x = start_offset.y = start_offset.z = 0.0f;
		*reinterpret_cast<vec3 *>(&start) = src;
	}

	__forceinline void init(const vec3 src, const vec3 end, const vec3 mins, const vec3 maxs)
	{
		root_move_parent = nullptr;
		*reinterpret_cast<vec3 *>(&delta) = end - src;
		*reinterpret_cast<vec3 *>(&extents) = maxs - mins;
		extents *= .5f;

		is_swept = sqrt_ps(delta.length()) != 0.f;
		is_ray = sqrt_ps(extents.length()) < 1e-6f;

		*reinterpret_cast<vec3 *>(&start_offset) = maxs + mins;
		start_offset *= .5f;
		*reinterpret_cast<vec3 *>(&this->start) = src + start_offset;
		start_offset *= -1.f;
	}
};

struct model;
struct renderable_instance
{
	uint8_t alpha;
};

union full_handle
{
	struct
	{
		uint16_t index;
		uint16_t serial;
	};
	uint32_t full;
};

using base_handle = uint32_t;
using client_shadow_handle = uint32_t;
using client_render_handle = uint32_t;
using model_instance_handle = uint32_t;
using create_client_class = client_networkable *(*)(int ent, int serial);
using create_event = client_networkable *(*)();

enum solid_type
{
	solid_none = 0,
	solid_bsp = 1,
	solid_bbox = 2,
	solid_obb = 3,
	solid_obb_yaw = 4,
	solid_custom = 5,
	solid_vphysics = 6,
	solid_last
};

enum bone
{
	bone_used_mask = 0x0007FF00,
	bone_used_by_anything = 0x0007FF00,
	bone_used_by_hitbox = 0x00000100,
	bone_used_by_attachment = 0x00000200,
	bone_used_by_vertex_mask = 0x0003FC00,
	bone_used_by_vertex_lod0 = 0x00000400,
	bone_used_by_vertex_lod1 = 0x00000800,
	bone_used_by_vertex_lod2 = 0x00001000,
	bone_used_by_vertex_lod3 = 0x00002000,
	bone_used_by_vertex_lod4 = 0x00004000,
	bone_used_by_vertex_lod5 = 0x00008000,
	bone_used_by_vertex_lod6 = 0x00010000,
	bone_used_by_vertex_lod7 = 0x00020000,
	bone_used_by_bone_merge = 0x00040000,
	bone_always_setup = 0x00080000
};

class handle_entity
{
public:
	virtual ~handle_entity() = default;
	virtual void set_ref_handle(const base_handle &handle) = 0;
	virtual const base_handle &get_handle() const = 0;
};

enum class class_id : int
{
	CAI_BaseNPC = 0,
	CAK47,
	CBaseAnimating,
	CBaseAnimatingOverlay,
	CBaseAttributableItem,
	CBaseButton,
	CBaseCombatCharacter,
	CBaseCombatWeapon,
	CBaseCSGrenade,
	base_csgrenade_projectile,
	CBaseDoor,
	base_entity,
	CBaseFlex,
	CBaseGrenade,
	CBaseParticleEntity,
	CBasePlayer,
	CBasePropDoor,
	CBaseTeamObjectiveResource,
	CBaseTempEntity,
	CBaseToggle,
	CBaseTrigger,
	CBaseViewModel,
	CBaseVPhysicsTrigger,
	base_weapon_world_model,
	CBeam,
	CBeamSpotlight,
	CBoneFollower,
	CBRC4Target,
	CBreachCharge,
	CBreachChargeProjectile,
	CBreakableProp,
	breakable_surface,
	CBumpMine,
	CBumpMineProjectile,
	c4,
	CCascadeLight,
	chicken,
	CColorCorrection,
	CColorCorrectionVolume,
	CCSGameRulesProxy,
	csplayer,
	CCSPlayerResource,
	csragdoll,
	CCSTeam,
	CDangerZone,
	CDangerZoneController,
	CDEagle,
	CDecoyGrenade,
	CDecoyProjectile,
	CDrone,
	CDronegun,
	CDynamicLight,
	CDynamicProp,
	econ_entity,
	econ_wearable,
	CEmbers,
	CEntityDissolve,
	CEntityFlame,
	CEntityFreezing,
	CEntityParticleTrail,
	CEnvAmbientLight,
	CEnvDetailController,
	CEnvDOFController,
	CEnvGasCanister,
	CEnvParticleScript,
	CEnvProjectedTexture,
	CEnvQuadraticBeam,
	CEnvScreenEffect,
	CEnvScreenOverlay,
	CEnvTonemapController,
	CEnvWind,
	CFEPlayerDecal,
	CFireCrackerBlast,
	CFireSmoke,
	CFireTrail,
	CFish,
	CFists,
	CFlashbang,
	CFogController,
	CFootstepControl,
	CFunc_Dust,
	CFunc_LOD,
	CFuncAreaPortalWindow,
	func_brush,
	CFuncConveyor,
	CFuncLadder,
	CFuncMonitor,
	CFuncMoveLinear,
	CFuncOccluder,
	CFuncReflectiveGlass,
	CFuncRotating,
	CFuncSmokeVolume,
	CFuncTrackTrain,
	CGameRulesProxy,
	CGrassBurn,
	CHandleTest,
	CHEGrenade,
	hostage,
	CHostageCarriableProp,
	incendiary_grenade,
	inferno,
	CInfoLadderDismount,
	CInfoMapRegion,
	CInfoOverlayAccessor,
	CItem_Healthshot,
	CItemCash,
	CItemDogtags,
	CKnife,
	CKnifeGG,
	CLightGlow,
	CMapVetoPickController,
	CMaterialModifyControl,
	CMelee,
	molotov_grenade,
	molotov_projectile,
	CMovieDisplay,
	CParadropChopper,
	CParticleFire,
	CParticlePerformanceMonitor,
	CParticleSystem,
	CPhysBox,
	CPhysBoxMultiplayer,
	CPhysicsProp,
	CPhysicsPropMultiplayer,
	CPhysMagnet,
	CPhysPropAmmoBox,
	CPhysPropLootCrate,
	CPhysPropRadarJammer,
	CPhysPropWeaponUpgrade,
	planted_c4,
	CPlasma,
	CPlayerPing,
	CPlayerResource,
	CPointCamera,
	CPointCommentaryNode,
	CPointWorldText,
	CPoseController,
	CPostProcessController,
	CPrecipitation,
	CPrecipitationBlocker,
	predicted_view_model,
	CProp_Hallucination,
	CPropCounter,
	CPropDoorRotating,
	CPropJeep,
	CPropVehicleDriveable,
	CRagdollManager,
	CRagdollProp,
	CRagdollPropAttached,
	CRopeKeyframe,
	CSCAR17,
	CSceneEntity,
	CSensorGrenade,
	sensor_grenade_projectile,
	CShadowControl,
	CSlideshowDisplay,
	CSmokeGrenade,
	smoke_grenade_projectile,
	CSmokeStack,
	snowball,
	CSnowballPile,
	CSnowballProjectile,
	CSpatialEntity,
	CSpotlightEnd,
	CSprite,
	CSpriteOriented,
	CSpriteTrail,
	CStatueProp,
	CSteamJet,
	sun,
	CSunlightShadowControl,
	CSurvivalSpawnChopper,
	CTablet,
	CTeam,
	CTeamplayRoundBasedRulesProxy,
	CTEArmorRicochet,
	CTEBaseBeam,
	CTEBeamEntPoint,
	CTEBeamEnts,
	CTEBeamFollow,
	CTEBeamLaser,
	CTEBeamPoints,
	CTEBeamRing,
	CTEBeamRingPoint,
	CTEBeamSpline,
	CTEBloodSprite,
	CTEBloodStream,
	CTEBreakModel,
	CTEBSPDecal,
	CTEBubbles,
	CTEBubbleTrail,
	CTEClientProjectile,
	CTEDecal,
	CTEDust,
	CTEDynamicLight,
	CTEEffectDispatch,
	CTEEnergySplash,
	CTEExplosion,
	CTEFireBullets,
	CTEFizz,
	CTEFootprintDecal,
	CTEFoundryHelpers,
	CTEGaussExplosion,
	CTEGlowSprite,
	CTEImpact,
	CTEKillPlayerAttachments,
	CTELargeFunnel,
	CTEMetalSparks,
	CTEMuzzleFlash,
	CTEParticleSystem,
	CTEPhysicsProp,
	CTEPlantBomb,
	CTEPlayerAnimEvent,
	CTEPlayerDecal,
	CTEProjectedDecal,
	CTERadioIcon,
	CTEShatterSurface,
	CTEShowLine,
	CTesla,
	CTESmoke,
	CTESparks,
	CTESprite,
	CTESpriteSpray,
	CTest_ProxyToggle_Networkable,
	CTestTraceline,
	CTEWorldDecal,
	CTriggerPlayerMovement,
	CTriggerSoundOperator,
	CVGuiScreen,
	CVoteController,
	CWaterBullet,
	CWaterLODControl,
	CWeaponAug,
	CWeaponAWP,
	CWeaponBaseItem,
	CWeaponBizon,
	CWeaponCSBase,
	CWeaponCSBaseGun,
	CWeaponCycler,
	CWeaponElite,
	CWeaponFamas,
	CWeaponFiveSeven,
	CWeaponG3SG1,
	CWeaponGalil,
	CWeaponGalilAR,
	CWeaponGlock,
	CWeaponHKP2000,
	CWeaponM249,
	CWeaponM3,
	CWeaponM4A1,
	CWeaponMAC10,
	CWeaponMag7,
	CWeaponMP5Navy,
	CWeaponMP7,
	CWeaponMP9,
	CWeaponNegev,
	CWeaponNOVA,
	CWeaponP228,
	CWeaponP250,
	CWeaponP90,
	CWeaponSawedoff,
	CWeaponSCAR20,
	CWeaponScout,
	CWeaponSG550,
	CWeaponSG552,
	CWeaponSG556,
	CWeaponShield,
	CWeaponSSG08,
	CWeaponTaser,
	CWeaponTec9,
	CWeaponTMP,
	CWeaponUMP45,
	CWeaponUSP,
	CWeaponXM1014,
	CWeaponZoneRepulsor,
	CWorld,
	CWorldVguiText,
	DustTrail,
	MovieExplosion,
	ParticleSmokeGrenade,
	RocketTrail,
	SmokeTrail,
	SporeExplosion,
	spore_trail,
};

class client_class
{
public:
	create_client_class create_fn;
	create_event create_event_fn;
	char *network_name;
	recv_table *table;
	client_class *next;
	class_id id;
};

class collideable
{
protected:
	~collideable() = default;

public:
	virtual handle_entity *get_entity_handle() = 0;
	virtual vec3 &obb_mins() const = 0;
	virtual vec3 &obb_maxs() const = 0;
	virtual void world_space_trigger_bounds(vec3 *mins, vec3 *maxs) const = 0;
	virtual bool test_collision(const ray &ray, unsigned int mask, void *trace) = 0;
	virtual bool test_hitboxes(const ray &ray, unsigned int mask, void *trace) = 0;
	virtual int get_collision_model_index() = 0;
	virtual const model *get_collision_model() = 0;
	virtual vec3 &get_collision_origin() const = 0;
	virtual angle &get_collision_angles() const = 0;
	virtual const mat3x4 &collision_to_world_transform() const = 0;
	virtual solid_type get_solid() const = 0;
	virtual int get_solid_flags() const = 0;
	virtual client_unknown *get_client_unknown() = 0;
	virtual int get_collision_group() const = 0;
	virtual void world_space_surrounding_bounds(vec3 *mins, vec3 *maxs) = 0;
	virtual bool should_touch_trigger(int flags) const = 0;
	virtual const mat3x4 *get_root_parent_to_world_transform() const = 0;
};

class client_unknown : public handle_entity
{
public:
	virtual collideable *get_collideable() = 0;
	virtual client_networkable *get_client_networkable() = 0;
	virtual client_renderable *get_client_renderable() = 0;
	virtual client_entity *get_client_entity() = 0;
	virtual entity_t *get_base_entity() = 0;
	virtual client_thinkable *get_client_thinkable() = 0;
	virtual client_alpha_property *get_client_alpha_property() = 0;
};

class client_thinkable
{
public:
	virtual ~client_thinkable() = default;
};

class client_model_renderable
{
public:
	virtual bool get_render_data(void* data, int model_data_category) = 0;
};

class client_renderable
{
protected:
	~client_renderable() = default;

public:
	virtual client_unknown *get_client_unknown() = 0;
	virtual vec3 const &get_render_origin() = 0;
	virtual vec3 const &get_render_angles() = 0;
	virtual bool should_draw() = 0;
	virtual int get_render_flags() = 0;
	virtual bool is_transparent() = 0; // fixed 27/4/24
	virtual client_shadow_handle get_shadow_handle() const = 0;
	virtual client_render_handle &render_handle() = 0;
	virtual const model *get_model() const = 0;
	virtual int draw_model(int flags, uint8_t alpha) = 0;
	virtual int get_body() = 0;
	virtual void get_color_modulation(float *color) = 0;
	virtual bool lod_test() = 0;
	virtual bool setup_bones(mat3x4 *bones, int max, int mask, float time) = 0;
	virtual void setup_weights(const mat3x4 *bones, int count, float *weights, float *delayed_weights) = 0;
	virtual void do_animation_events() = 0;
	virtual void *get_pvs_notify_interface() = 0;
	virtual void get_render_bounds(vec3 &mins, vec3 &maxs) = 0;
	virtual void get_render_bounds_worldspace(vec3 &mins, vec3 &maxs) = 0;
	virtual void get_shadow_render_bounds(vec3 &mins, vec3 &maxs, int type) = 0;
	virtual bool should_receive_projected_textures(int flags) = 0;
	virtual bool get_shadow_cast_distance(float *dist, int type) const = 0;
	virtual bool get_shadow_cast_direction(vec3 *direction, int type) const = 0;
	virtual bool is_shadow_dirty() = 0;
	virtual void mark_shadow_dirty(bool dirty) = 0;
	virtual client_renderable *get_shadow_parent() = 0;
	virtual client_renderable *first_shadow_child() = 0;
	virtual client_renderable *next_shadow_peer() = 0;
	virtual int shadow_cast_type() = 0;
	virtual void unused2() = 0; // fixed 27/4/24
	virtual void create_model_instance() = 0;
	virtual model_instance_handle get_model_instance() = 0;
	virtual const mat3x4 &renderable_to_world_transform() = 0;
	virtual int lookup_attachment(const char *name) = 0;
	virtual bool get_attachment(int index, vec3 &origin, vec3 &angles) = 0;
	virtual bool get_attachment(int index, mat3x4 &matrix) = 0;
	virtual float *get_render_clip_plane() = 0;
	//virtual void pad0() = 0; // fixed 27/4/24
	virtual int get_skin() = 0;
	virtual void on_threaded_draw_setup() = 0;
	virtual bool uses_flex_delayed_weights() = 0;
	virtual void record_tool_message() = 0;
	virtual bool should_draw_for_split_screen_user(int slot) = 0;
	virtual uint8_t override_alpha_modulation(uint8_t alpha) = 0;
	virtual uint8_t override_shadow_alpha_modulation(uint8_t alpha) = 0;
	virtual client_model_renderable* get_client_model_renderable() = 0;
};

enum class entity_type
{
	none = -1,
	hostage,
	player,
	weapon_entity,
	item,
	projectile,
	c4,
	plantedc4,
	chicken
};

enum item_definition_index : short
{
	weapon_none = 0,
	weapon_deagle = 1,
	weapon_elite = 2,
	weapon_fiveseven = 3,
	weapon_glock = 4,
	weapon_ak47 = 7,
	weapon_aug = 8,
	weapon_awp = 9,
	weapon_famas = 10,
	weapon_g3sg1 = 11,
	weapon_galilar = 13,
	weapon_m249 = 14,
	weapon_m4a1 = 16,
	weapon_mac10 = 17,
	weapon_p90 = 19,
	weapon_mp5sd = 23,
	weapon_ump45 = 24,
	weapon_xm1014 = 25,
	weapon_bizon = 26,
	weapon_mag7 = 27,
	weapon_negev = 28,
	weapon_sawedoff = 29,
	weapon_tec9 = 30,
	weapon_taser = 31,
	weapon_hkp2000 = 32,
	weapon_mp7 = 33,
	weapon_mp9 = 34,
	weapon_nova = 35,
	weapon_p250 = 36,
	weapon_shield = 37,
	weapon_scar20 = 38,
	weapon_sg556 = 39,
	weapon_ssg08 = 40,
	weapon_knifegg = 41,
	weapon_knife = 42,
	weapon_flashbang = 43,
	weapon_hegrenade = 44,
	weapon_smokegrenade = 45,
	weapon_molotov = 46,
	weapon_decoy = 47,
	weapon_incgrenade = 48,
	weapon_c4 = 49,
	weapon_healthshot = 57,
	weapon_knife_t = 59,
	weapon_m4a1_silencer = 60,
	weapon_usp_silencer = 61,
	weapon_cz75a = 63,
	weapon_revolver = 64,
	weapon_tagrenade = 68,
	weapon_fists = 69,
	weapon_breachcharge = 70,
	weapon_tablet = 72,
	weapon_melee = 74,
	weapon_axe = 75,
	weapon_hammer = 76,
	weapon_spanner = 78,
	weapon_knife_ghost = 80,
	weapon_firebomb = 81,
	weapon_diversion = 82,
	weapon_frag_grenade = 83,
	weapon_snowball = 84,
	weapon_bumpmine = 85,
	weapon_bayonet = 500,
	weapon_knife_css = 503,
	weapon_knife_flip = 505,
	weapon_knife_gut = 506,
	weapon_knife_karambit = 507,
	weapon_knife_m9_bayonet = 508,
	weapon_knife_tactical = 509,
	weapon_knife_falchion = 512,
	weapon_knife_survival_bowie = 514,
	weapon_knife_butterfly = 515,
	weapon_knife_push = 516,
	weapon_knife_cord = 517,
	weapon_knife_canis = 518,
	weapon_knife_ursus = 519,
	weapon_knife_gypsy_jackknife = 520,
	weapon_knife_outdoor = 521,
	weapon_knife_stiletto = 522,
	weapon_knife_widowmaker = 523,
	weapon_knife_skeleton = 525,
	glove_broken_fang = 4725,
	studded_bloodhound_gloves = 5027,
	glove_t_side = 5028,
	glove_ct_side = 5029,
	glove_sporty = 5030,
	glove_slick = 5031,
	glove_leather_wrap = 5032,
	glove_motorcycle = 5033,
	glove_specialist = 5034,
	studded_hydra_gloves = 5035,
};

class client_networkable
{
protected:
	~client_networkable() = default;

public:
	virtual client_unknown *get_client_unknown() = 0;
	virtual void release() = 0;
	virtual client_class *get_client_class() = 0;
	virtual void notify_should_transmit(int state) = 0;
	virtual void on_pre_data_changed(int type) = 0;
	virtual void on_data_changed(int type) = 0;
	virtual void pre_data_update(int type) = 0;
	virtual void post_data_update(int type) = 0;
	virtual void on_data_unchanged_in_pvs() = 0;
	virtual bool is_dormant() = 0;
	virtual int index() const = 0;
	virtual void receive_message(int class_id, uintptr_t msg) = 0;
	virtual void *get_data_table() = 0;
	virtual void set_destroyed_on_recreate_entities() = 0;

	__forceinline class_id get_class_id() { return get_client_class()->id; }

	__forceinline bool is_player() { return get_class_id() == class_id::csplayer; }

	__forceinline bool is_entity()
	{
		auto next = get_client_class();

		do
		{
			if (next && next->id == class_id::base_entity)
				return true;
		} while (next && (next = next->next));

		return false;
	}
};

class client_entity : public client_unknown,
					  public client_renderable,
					  public client_networkable,
					  public client_thinkable
{
public:
	virtual ~client_entity(){};
};
} // namespace sdk

#endif // SDK_CLIENT_ENTITY_H
