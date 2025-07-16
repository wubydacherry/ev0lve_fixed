
namespace interfaces
{
	constexpr uint32_t vengine_client014 = 0x59d3e8;
	constexpr uint32_t vclient018 = 0x4DFFE80;
	constexpr uint32_t vclient_entity_list003 = 0x4E251DC;
	constexpr uint32_t vengine_cvar007 = 0x3d310;
	constexpr uint32_t game_movement001 = 0x5260238;
	constexpr uint32_t mdlcache004 = 0x64f18;
	constexpr uint32_t localize_001 = 0x38a70;
	constexpr uint32_t vclient_prediction001 = 0x5263150;
	constexpr uint32_t engine_trace_client004 = 0x5a7404;
	constexpr uint32_t vphysics_surface_props001 = 0x116078;
	constexpr uint32_t vmodel_info_client004 = 0x5ad560;
	constexpr uint32_t vdebug_overlay004 = 0x59feb0;
	constexpr uint32_t vengine_render_view014 = 0x5a1d14;
	constexpr uint32_t vgui_surface031 = 0xf47d0;
	constexpr uint32_t vengine_model016 = 0x7847a0;
	constexpr uint32_t vmaterial_system080 = 0xe2ac8;
	constexpr uint32_t vengine_client_string_table001 = 0x5aeeb8;
	constexpr uint32_t material_system_hardware_config013_callback = 0xfd5e8;
	constexpr uint32_t input_system_version001 = 0x45388;
	constexpr uint32_t gameeventsmanager002 = 0x81c548;
	constexpr uint32_t panorama_uiengine001 = 0x23c098;
	constexpr uint32_t vstudio_render026 = 0x41fe98;
	constexpr uint32_t game_console004 = 0xe06624;
}

namespace globals
{
	constexpr uint32_t global_vars_base = 0x59ee60;
	constexpr uint32_t mem_alloc = 0x514f0;
	constexpr uint32_t player_resource = 0x3231380;
	constexpr uint32_t client_state = 0x59F19C;
	constexpr uint32_t input = 0x525C4E0;
	constexpr uint32_t weapon_system = 0x526388C;
	constexpr uint32_t game_rules = 0x5334764;
	constexpr uint32_t view_render = 0x525DFE0;
	constexpr uint32_t glow_manager = 0x535FCB8;
	constexpr uint32_t leaf_system = 0x4E35328;
	constexpr uint32_t key_values_system = 0xd0e0;
	constexpr uint32_t modifier_table = 0xDF7FC0;
	constexpr uint32_t econ_item_system = 0x534C958;
	constexpr uint32_t input_enable = 0x3ac8; // fixed 20/4/24

	constexpr uint32_t hud_instance = 0x523BC98;

	constexpr uint32_t client_side_addons_mask = 0xdb6300;
	constexpr uint32_t post_processing_params = 0xdfa038;
	constexpr uint32_t smoke_count = 0x525caac;

	constexpr uint32_t anim_layer_orders = 0x442b50;
	constexpr uint32_t trace_filter_simple_vtable = 0xbea9c4;

	constexpr uint32_t world_to_screen = 0x220E78;

	constexpr uint32_t softbody_environment = 0x3F3B30;

	constexpr uint32_t patch_cl_interp_clamp = 0x419693;
	constexpr uint32_t patch_send_move_cmds = 0xdd184; // needs checking

	namespace material_system
	{
		constexpr uint32_t disable_render_target_allocation_forever = 0x9ec;
	}
}

namespace functions
{
	namespace v8
	{
		namespace isolate
		{
			constexpr uint32_t enter = 0x2b9d1;
			constexpr uint32_t exit = 0x27d86;
		}

		namespace context
		{
			constexpr uint32_t enter = 0x771b;
			constexpr uint32_t exit = 0x1e42f;
		}

		namespace try_catch
		{
			constexpr uint32_t ctor = 0x1eeb1;
			constexpr uint32_t dtor = 0x2521b;
		}

		namespace handle_scope
		{
			constexpr uint32_t ctor = 0x1e6fa;
			constexpr uint32_t dtor = 0x1325;
			constexpr uint32_t create_handle = 0x59c0;
		}

		namespace value
		{
			constexpr uint32_t is_string = 0x1d17e;
			constexpr uint32_t is_string_object = 0x25059;
			constexpr uint32_t is_array = 0x1725b;
			constexpr uint32_t is_object = 0x2509a;
			constexpr uint32_t is_boolean = 0x273f4;
			constexpr uint32_t is_boolean_object = 0x13ffc;
			constexpr uint32_t is_number = 0x1375f;
			constexpr uint32_t is_number_object = 0x2d3e4;
			constexpr uint32_t is_function = 0x273f4;
			constexpr uint32_t boolean_value = 0x27827;
			constexpr uint32_t number_value = 0xf85d;
			constexpr uint32_t to_object = 0x1bf90;
		}

		namespace object
		{
			constexpr uint32_t get = 0x1bcbb;
			constexpr uint32_t get_property_names = 0x2c949;
		}

		namespace array
		{
			constexpr uint32_t length = 0x2f8;
			constexpr uint32_t get = 0x1da52;
		}

		namespace string
		{
			constexpr uint32_t utf8_ctor = 0x31bf6;
			constexpr uint32_t utf8_dtor = 0x11d1a;
		}
	}

	namespace ui_engine
	{
		constexpr uint32_t run_compiled_script = 0x4c080;
		constexpr uint32_t compile = 0x4b580;
		constexpr uint32_t get_panel_context = 0x4b790;
	}

	constexpr uint32_t lookup_sequence = 0x1D9130;
	constexpr uint32_t get_player_viewmodel_arm_config_for_player_model = 0x4364d0;
	//constexpr uint32_t return_to_is_module_whitelisted = 0x2374c085;
	constexpr uint32_t return_to_enable_inventory = 0x412383;
	constexpr uint32_t return_to_update_post_processing = 0x0000000; // MISSING
	constexpr uint32_t return_to_start_sound_entry = 0x3A2DB; // MISSING

	constexpr uint32_t host_shutdown = 0x22EA90;
	constexpr uint32_t hud_draw_scope = 0x4BDA80; // WARNING: COULD BE WRONG HOOK
	constexpr uint32_t particle_draw_model = 0x32cf20;
	constexpr uint32_t ent_info_list_link_before = 0x2a76e0;
	constexpr uint32_t process_input = 0x534010;
	constexpr uint32_t start_sound_immediate = 0x38cb0;
	constexpr uint32_t move_helper_start_sound = 0x31fbc0;
	constexpr uint32_t get_tonemap_settings_from_env_tonemap_controller = 0x226ef0;

	constexpr uint32_t draw_mesh = 0x16d00;
	constexpr uint32_t draw_3d_debug_overlays = 0x178dc0;
	constexpr uint32_t render_glow_boxes = 0x2C9140;
	constexpr uint32_t spawn_smoke_effect = 0x69A130;
	constexpr uint32_t render_iron_sight_scope_effect = 0x6AD510;
	constexpr uint32_t convar_network_change_callback = 0x1ed670;
	constexpr uint32_t tefirebullets_post_data_update = 0x3F6880;
	constexpr uint32_t enforce_competitive_cvar = 0x418CD0;

	constexpr uint32_t check_param = 0x2C80;

	constexpr uint32_t trace_to_studio_csgo_hitgroups_priority = 0x7df220;
	constexpr uint32_t clc_msg_voicedata = 0x19f550;
	constexpr uint32_t clear_death_notices = 0x51a020;
	constexpr uint32_t set_clantag = 0x8dab0;

	constexpr uint32_t load_named_sky = 0x137780;
	constexpr uint32_t reveal_rank = 0x3fe600;
	constexpr uint32_t accept_match = 0x000000;
	constexpr uint32_t clear_hud_weapon_icon = 0x55a5a0;

	/*
	test edi, edi ; Logical Compare - 0x3FE2CC
	jz short loc_103FE33C ; Jump if Zero (ZF=1) - 0x3FE2CE
	*/
	constexpr uint32_t return_to_should_draw_viewmodel = 0x3FE2CE; //0x3FE2CC; // IDA: "55 8B EC 51 57 E8"

	constexpr uint32_t return_to_anim_state_update = 0x43ee50;
	constexpr uint32_t return_to_hitbox_to_world_transforms = 0x1d45d7;
	constexpr uint32_t return_to_cs_player_setup_bones = 0x3E84AA; // WARNING: CAN BE VERY WRONG // 0x3E84C6; // IDA: "84 C0 75 0D F6 87 28 0A 00 00 0A"
	constexpr uint32_t return_to_anim_state_setup_velocity = 0x443481;

	constexpr uint32_t find_hud_element = 0x2d3a40;
	constexpr uint32_t user_cmd_get_checksum = 0x307260; 
	constexpr uint32_t load_text_file = 0x26a950;

	constexpr uint32_t add_to_dirty_kd_tree = 0x28c510;
	constexpr uint32_t random_int = 0xffb0;
	constexpr uint32_t random_float = 0xff40;
	constexpr uint32_t random_seed = 0xff20;


	namespace image_data
	{
		constexpr uint32_t pat = 0x9a982;
		constexpr uint32_t load_png = 0x000000; // MISSING
		constexpr uint32_t load_svg = 0xab6f0;
	}

	namespace modifier_table
	{
		constexpr uint32_t find = 0x9a2c40;
		constexpr uint32_t add_string = 0x9a2cf0;
	}

	namespace keyvalues
	{
		constexpr uint32_t from_msg = 0x2702e0;
		constexpr uint32_t init = 0x999940;
		constexpr uint32_t load_from_buffer = 0x99bcf0;
		constexpr uint32_t set_string_int = 0x99b110;
	}

	namespace draw_sphere
	{
		constexpr uint32_t initialize = 0x9AA4D0;
		constexpr uint32_t fn = 0x30E290;
		constexpr uint32_t material = 0x2C939E;
	}

	namespace net_channel //check
	{
		constexpr uint32_t process_sockets = 0x2624a0;
		constexpr uint32_t netmsg_tick_ctor = 0xC6220; // _CNetMsg_Tick_Constructor
		constexpr uint32_t host = 0xDD696;
		constexpr uint32_t netmsg_tick_dtor = 0xA8D20;
	}

	namespace hdr
	{
		constexpr uint32_t get_sequence_flags = 0x19e0c0;
		constexpr uint32_t find_hash = 0x19df90;
		constexpr uint32_t init_softbody = 0x36CFB0;
		constexpr uint32_t find_mapping = 0x36cac0;
		constexpr uint32_t index_model_sequences = 0x19cdf0;
		constexpr uint32_t init = 0x36B2D0;
		constexpr uint32_t reinitialize = 0x36ca60;
		constexpr uint32_t empty_mapping = 0x524535c;
		constexpr uint32_t lookup = 0x39d0a0;
	}

	namespace server
	{
		constexpr uint32_t setup_bones = 0x48fa90; // 55 8B EC 83 E4 F8 51 53 FF 75 0C 8B D9 FF 75 08
		constexpr uint32_t get_entity = 0x3c9390; // 85 C9 7E 32 A1
		constexpr uint32_t run_command = 0x339890;
	}

	namespace tls
	{
		constexpr uint32_t allocate_thread_id = 0x12910;
		constexpr uint32_t free_thread_id = 0x129a0;
		constexpr uint32_t tls_slots = 0x54e30;
	}

	namespace gameoverlayrenderer
	{
		constexpr uint32_t present = 0x6add0;
		constexpr uint32_t reset = 0x6b150;
		constexpr uint32_t length_disasm = 0x779b0;
	}

	namespace mat_system_surface
	{
		constexpr uint32_t lock_cursor = 0x13e00;
	}

	namespace inputsystem
	{
		constexpr uint32_t wnd_proc = 0x3ac0;
		constexpr uint32_t vk_to_button_code = 0x2bd0;
	}

	namespace input_system
	{
		constexpr uint32_t vk_to_button_code = 0x2bd0;
	}

	namespace merged_mdl
	{
		constexpr uint32_t set_merge_mdl = 0x9AF480;
		constexpr uint32_t init = 0x9AE9A0;
		constexpr uint32_t setup_bones_for_attachment_queries = 0x9AEED0;
	}

	namespace anim_state
	{
		constexpr uint32_t create = 0x43E440;
		constexpr uint32_t reset = 0x43E550;
		constexpr uint32_t get_active_weapon_prefix = 0x4445F0;
		constexpr uint32_t update_layer_order_preset = 0x442B50;
		constexpr uint32_t update = 0x43E9E0;
	}

	namespace hl_client
	{
		constexpr uint32_t frame_stage_notify = 0x262b20;
		constexpr uint32_t create_move = 0x260c80;
		constexpr uint32_t level_init_pre_entity = 0x261670;
		constexpr uint32_t level_shutdown = 0x2618f0;
	}

	namespace engine_client
	{
		constexpr uint32_t get_aspect_ratio = 0xB7D50; // MISSING
		constexpr uint32_t cl_move = 0xdd290;
		constexpr uint32_t is_connected = 0x260f0;
	}

	namespace prediction
	{
		constexpr uint32_t run_command = 0x343bf0;
		constexpr uint32_t perform_prediction = 0x344a70;
	}

	namespace cs_game_movement
	{
		constexpr uint32_t process_movement = 0x2bb170;
	}

	namespace client_state
	{
		constexpr uint32_t packet_start = 0x286110;
		constexpr uint32_t send_netmsg = 0x256910;
		constexpr uint32_t send_datagram = 0x253d80;
		constexpr uint32_t send_server_cmd_key_values = 0xf03e0;
		constexpr uint32_t svc_msg_voicedata = 0x2866a0;
	}

	namespace client_mode
	{
		constexpr uint32_t override_view = 0x276d70;
		constexpr uint32_t do_post_screen_space_effects = 0x3fe450;
	}

	namespace leaf_system
	{
		constexpr uint32_t calc_renderable_world_space_aabb_bloated = 0x26f680;
		constexpr uint32_t add_renderables_to_render_list = 0x272980;
		constexpr uint32_t extract_occluded_renderables = 0x272870;
	}

	namespace model_render
	{
		constexpr uint32_t draw_model_execute = 0x13e6a0;
	}

	namespace trace_filter_for_player_head_collision
	{
		constexpr uint32_t should_hit_entity = 0x3794D0;
	}

	namespace ik_context
	{
		constexpr uint32_t construct = 0x7e8de0;
		constexpr uint32_t destruct = 0x1ceec0;
		constexpr uint32_t init = 0x7e8eb0;
		constexpr uint32_t update_targets = 0x7e9ce0;
		constexpr uint32_t solve_dependencies = 0x7eb330;
	}
}

namespace props
{
	namespace animationlayer
	{
		constexpr uint32_t sequence = 0x1dea80;
		constexpr uint32_t cycle = 0x1deb50;
		constexpr uint32_t playback_rate = 0x1debc0;
		constexpr uint32_t weight = 0x1deac0;
		constexpr uint32_t weight_delta_rate = 0x1deb30;
		constexpr uint32_t order = 0x1debe0;
		constexpr uint32_t prev_cycle = 0x2A69A0;
	}

	namespace base_entity
	{
		constexpr uint32_t model_index = 0x2a69e0;
		constexpr uint32_t simulation_time = 0x1e3b50;
	}

	namespace local
	{
		constexpr uint32_t aim_punch_angle = 0x2172d0;
	}

	namespace base_player
	{
		constexpr uint32_t ground_entity = 0x358b60;
	}

	namespace base_view_model
	{
		constexpr uint32_t weapon = 0x1b8a00;
		constexpr uint32_t sequence = 0x1b8c40;
	}

	namespace base_weapon_world_model
	{
		constexpr uint32_t effects = 0x1a76a0;
	}

	namespace csplayer
	{
		constexpr uint32_t wait_for_no_attack = 0x5365184;
		constexpr uint32_t move_state = 0x53653DC;
		constexpr uint32_t strafing = 0x536678C;
		constexpr uint32_t thirdperson_recoil = 0x53667C8;
	}

	namespace cslocal_player_exclusive
	{
		constexpr uint32_t velocity_modifier = 0x5364A70;
	}

	namespace base_csgrenade
	{
		constexpr uint32_t is_held_by_player = 0x538ECC4;//unsure 0xc83818;
		constexpr uint32_t throw_time = 0x538ED3C;//unsure  0xc83844;
	}
}

namespace offsets
{
	namespace predmaps
	{
		constexpr uint32_t csplayer = 0xdb61dc;
		constexpr uint32_t base_csgrenade = 0xdb9aa8;
		constexpr uint32_t decoy_grenade = 0xDBA14C;
		constexpr uint32_t molotov_grenade = 0xDBA314;
		constexpr uint32_t smoke_grenade = 0xDBA494;
		constexpr uint32_t hegrenade = 0xDBA1AC;
		constexpr uint32_t sensor_grenade = 0xDBA44C;
		constexpr uint32_t flashbang = 0xDBA194;
	}

	namespace base_entity
	{
		constexpr uint32_t fn_on_latch_interpolated_variables = 0x1e8ce0;
		constexpr uint32_t fn_set_abs_angles = 0x1ea950;
		constexpr uint32_t fn_estimate_abs_velocity = 0x1ecae0;
		constexpr uint32_t fn_invalidate_physics_recursive = 0x1aec40;
		constexpr uint32_t fn_notify_should_transmit = 0x1E7B20; // correct 20/4/2024	
		constexpr uint32_t fn_emit_sound = 0x3644f0;
		constexpr uint32_t fn_is_breakable = 0x320bd0;
		constexpr uint32_t fn_on_simulation_time_changing = 0x1e86f0;
	}

	namespace base_view_model
	{
		constexpr uint32_t fn_remove_viewmodel_arm_models = 0x3EF290;
		constexpr uint32_t fn_update_viewmodel_arm_models = 0x215080;

		constexpr uint32_t weapon = 0x29d8;
		constexpr uint32_t owner = 0x29dc;
		constexpr uint32_t sequence = 0x28c0;
	}

	namespace weapon_csbase
	{
		constexpr uint32_t fn_deploy = 0x6a16e0;
		constexpr uint32_t fn_get_weapon_type = 0x699690; // https://www.unknowncheats.me/forum/2681707-post12604.html
		constexpr uint32_t fn_get_econ_static_data = 0x715880;
	}

	namespace weapon_csbase_gun
	{
		constexpr uint32_t zoom_level = 0x33e0;
	}

	namespace script_created_item
	{
		constexpr uint32_t item_definition_index = 0x2FBA; //0x1ea;
		constexpr uint32_t entity_quality = 0x2fbc;
		constexpr uint32_t account_id = 0x2fd8;
		constexpr uint32_t item_idhigh = 0x2fd0;
		constexpr uint32_t initialized = 0x2fe4;
	}

	namespace econ_entity
	{
		constexpr uint32_t fallback_seed = 0x31dc;
		constexpr uint32_t fallback_stat_trak = 0x31e4;
		constexpr uint32_t fallback_paint_kit = 0x31d8;
		constexpr uint32_t fallback_wear = 0x31e0;
		constexpr uint32_t original_owner_xuid_high = 0x31d4;
		constexpr uint32_t original_owner_xuid_low = 0x31d0;
	}

	namespace base_grenade
	{
		constexpr uint32_t thrower = 0x29b0;
		constexpr uint32_t dmg_radius = 0x2994;
	}

	namespace base_csgrenade
	{
		constexpr uint32_t pin_pulled = 0x33e2;
		constexpr uint32_t throw_time = 0x33e4;
		constexpr uint32_t throw_strength = 0x33ec;
		constexpr uint32_t is_held_by_player = 0x33e1;
	}

	namespace base_csgrenade_projectile
	{
		constexpr uint32_t v_initial_velocity = 0x29e0;
		constexpr uint32_t bounces = 0x29ec;
		constexpr uint32_t spawn_time = 0x2A04; // CBaseCSGrenadeProjectile->m_vecExplodeEffectOrigin + 0xC ( float )
		constexpr uint32_t next_trail_line = 0x2A0C; //0x2A14; // IDA: "0F 2F 86 14 2A 00 00" 
		//NEW :" 0F 2F 86 08 2A 00 00"
		//also check this "F3 0F 11 86 0C 2A 00 00"


		constexpr uint32_t last_trail_pos = 0x2A08; // IDA: "89 86 08 2A 00 00 8B 86 B0 00 00 00"

		constexpr uint32_t fn_client_think = 0x3CC510;
		constexpr uint32_t fn_post_data_update = 0x3CC380; // 55 8B EC 83 EC 0C 56 57 8B 7D 08 8B F1 57 надеюсь это оно
	}

	namespace molotov_projectile
	{
		constexpr uint32_t is_inc_grenade = 0x2a20;
	}

	namespace smoke_grenade_projectile
	{
		constexpr uint32_t smoke_effect_tick_begin = 0x2a20;
	}

	namespace base_weapon_world_model
	{
		constexpr uint32_t combat_weapon_parent = 0x2a00;
		constexpr uint32_t model_index = 0x258;
	}

	namespace weapon_csbase
	{
		constexpr uint32_t postpone_fire_ready_time = 0x335c;
		constexpr uint32_t weapon_mode = 0x3328;
		constexpr uint32_t accuracy_penalty = 0x3340;
		constexpr uint32_t recoil_index = 0x3350;
		constexpr uint32_t last_shot_time = 0x33b8;
		constexpr uint32_t iron_sight_controller = 0x33d0;
		constexpr uint32_t last_client_fire_bullet_time = 0x33cc;
		constexpr uint32_t custom_materials = 9e8; // can be wrong, needs testing.
		constexpr uint32_t custom_materials_initialized = 0x3370;

		constexpr uint32_t fn_get_econ_item_view = 0x1abcf0;
		constexpr uint32_t fn_equip_glove = 0x723170;
	}

	namespace base_combat_weapon
	{
		constexpr uint32_t clip1 = 0x3274;
		constexpr uint32_t next_primary_attack = 0x3248;
		constexpr uint32_t next_secondary_attack = 0x324c;
		constexpr uint32_t weapon_world_model = 0x3264;
		constexpr uint32_t world_model_index = 0x3254;
		constexpr uint32_t world_dropped_model_index = 0x3258;
		constexpr uint32_t view_model_index = 0x3250;

		constexpr uint32_t fn_send_weapon_anim = 0x1a9bd0;
	}

	namespace base_grenade
	{
		constexpr uint32_t velocity = 0x114;
	}

	namespace planted_c4
	{
		constexpr uint32_t bomb_ticking = 0x2990;
		constexpr uint32_t bomb_site = 0x2994;
		constexpr uint32_t c4_blow = 0x29a0;
		constexpr uint32_t timer_length = 0x29a4;
		constexpr uint32_t defuse_count_down = 0x29bc;
		constexpr uint32_t bomb_defused = 0x29c0;
		constexpr uint32_t defuse_length = 0x29b8;
		constexpr uint32_t bomb_defuser = 0x29c4;
	}

	namespace csgame_rules
	{
		constexpr uint32_t freeze_period = 0x20;
		constexpr uint32_t warmup_period = 0x21;
		constexpr uint32_t match_waiting_for_resume = 0x41;
		constexpr uint32_t is_valve_ds = 0x7c;
		constexpr uint32_t play_all_step_sounds_on_server = 0x7e;
	}

	namespace base_combat_character
	{
		constexpr uint32_t active_weapon = 0x2f08;
		constexpr uint32_t my_weapons = 0x2e08;
		constexpr uint32_t my_wearables = 0x2f14;
		constexpr uint32_t next_attack = 0x2d80;
	}

	namespace local
	{
		constexpr uint32_t view_punch_angle = 0x64;
		constexpr uint32_t aim_punch_angle = 0x70;
		constexpr uint32_t aim_punch_angle_vel = 0x7c;
		constexpr uint32_t ducking = 0x89;
		constexpr uint32_t ducked = 0x88;
		constexpr uint32_t step_size = 0x60;
	}

	namespace base_player
	{
		constexpr uint32_t duck_speed = 0x2fc0;
		constexpr uint32_t last_duck_time = 0x8c;
		//constexpr uint32_t has_walk_moved_since_last_jump = 0x0000000; DATAVARMAP
		constexpr uint32_t use_new_animstate = 0x9B14;
		constexpr uint32_t surface_props = 0x35dc;
		constexpr uint32_t command_context = 0x350c;
		constexpr uint32_t final_predicted_tick = 0x3444;
		constexpr uint32_t ground_entity = 0x150;
		//constexpr uint32_t surface_friction = 0x0000000; DATAVARMAP
		constexpr uint32_t maxspeed = 0x3258;
		constexpr uint32_t fovstart = 0x31f8;
		constexpr uint32_t fov = 0x31f4;
		constexpr uint32_t fovtime = 0x3218;
		constexpr uint32_t view_model_0 = 0x3308;
		constexpr uint32_t observer_mode = 0x3388;
		constexpr uint32_t observer_target = 0x339c;
		constexpr uint32_t tick_base = 0x3440;
		constexpr uint32_t flags = 0x104;
		constexpr uint32_t ammo = 0x2d88;
		constexpr uint32_t fn_init_local_data = 0x211c00;
		constexpr uint32_t fn_physics_simulate = 0x20e2b0;

	}

	namespace csplayer
	{
		constexpr uint32_t flash_duration = 0x10470;
		constexpr uint32_t ragdoll = 0x1043c;
		constexpr uint32_t is_player_ghost = 0x9ae1;
		constexpr uint32_t assassination_target_addon = 0x2a0c;
		//constexpr uint32_t cycle = 0x0000000; DATAMAPVAR
		constexpr uint32_t bounds_change_time = 0x9924;
		constexpr uint32_t view_height = 0x9920;
		constexpr uint32_t snapshot_entire_body = 0x9b20;
		constexpr uint32_t snapshot_upper_body = 0xcf70;
		constexpr uint32_t anim_state = 0x9960;
		constexpr uint32_t animation_layers = 0x2990;
		constexpr uint32_t lod_data = 0xa24;
		constexpr uint32_t ground_accel_linear_frac_last_time = 0x103f0;
		constexpr uint32_t stamina = 0x103d8;
		constexpr uint32_t is_holding_look_at_weapon = 0x11975;
		constexpr uint32_t is_looking_at_weapon = 0x11974;
		constexpr uint32_t secondary_addon = 0x103cc;
		constexpr uint32_t primary_addon = 0x103c8;
		constexpr uint32_t addon_bits = 0x103c4;
		constexpr uint32_t thirdperson_recoil = 0x119ec;
		constexpr uint32_t wait_for_no_attack = 0x99bc;
		constexpr uint32_t shots_fired = 0x103e0;
		constexpr uint32_t move_state = 0x99d0;
		constexpr uint32_t strafing = 0x9ae0;
		constexpr uint32_t carried_hostage = 0x10448;
		constexpr uint32_t lower_body_yaw_target = 0x9adc;
		constexpr uint32_t eye_angles = 0x117d0;
		constexpr uint32_t flash_bang_time = 0x10470;
		constexpr uint32_t flash_max_alpha = 0x1046c;
		constexpr uint32_t gun_game_immunity = 0x9990;
		constexpr uint32_t has_heavy_armor = 0x117c1;
		constexpr uint32_t has_helmet = 0x117c0;
		constexpr uint32_t is_walking = 0x9975;
		constexpr uint32_t is_scoped = 0x9974;

		constexpr uint32_t fn_get_ammo_def = 0x4136C0;
		constexpr uint32_t fn_is_other_enemy = 0x42CF50;
		constexpr uint32_t fn_calc_view = 0x3e0fb0;
		constexpr uint32_t fn_fire_event = 0x3ea080;
		constexpr uint32_t fn_obb_change_callback = 0x432580;
		constexpr uint32_t fn_reevauluate_anim_lod = 0x3e81b0;
		constexpr uint32_t fn_get_fov = 0x3ea920;
		constexpr uint32_t fn_get_default_fov = 0x1b5bb0;
		constexpr uint32_t fn_update_clientside_animation = 0x3e7a80;
		constexpr uint32_t fn_post_build_transformations = 0x432e20;
		constexpr uint32_t fn_update_addon_models = 0x3e13e0;
	}

	namespace cslocal_player_exclusive
	{
		constexpr uint32_t velocity_modifier = 0xbfed48;
	}

	namespace player_resource
	{
		constexpr uint32_t health = 0x1184;
	}

	namespace csplayer
	{
		constexpr uint32_t fn_post_data_update = 0x3e5f70;
	}

	namespace csplayer_resource
	{
		constexpr uint32_t has_helmet = 0x1839;
		constexpr uint32_t has_defuser = 0x17f8;
		constexpr uint32_t armor = 0x187c;
		constexpr uint32_t player_c4 = 0x165c;
		constexpr uint32_t clan = 0x43ac;
		constexpr uint32_t crosshair_codes = 0x5225;
		constexpr uint32_t bombsite_center_a = 0x1664;
		constexpr uint32_t bombsite_center_b = 0x1670;
	}

	namespace base_animating
	{
		constexpr uint32_t enable_invalidate_bone_cache = 0xDAC0D0;
		constexpr uint32_t fn_do_animation_events = 0x1e01b0;
		constexpr uint32_t fn_maintain_sequence_transitions = 0x1d11f0;
		constexpr uint32_t fn_update_relevant_interpolated_vars = 0x1cf130;
		constexpr uint32_t fn_invalidate_mdl_cache = 0x1acbf0;
		constexpr uint32_t fn_lookup_sequence = 0x1d9130;
		constexpr uint32_t fn_get_sequence_activity = 0x1D94A0;
		constexpr uint32_t fn_lookup_bone = 0x1cf8f0;
		constexpr uint32_t fn_setup_bones = 0x3e8480;
	}

	namespace base_animating_overlay
	{
		constexpr uint32_t fn_do_animation_events = 0x1E01B0;
	}
}

