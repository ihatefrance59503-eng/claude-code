// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <cstdint>
#include <intrin.h>

// -----------------------------------------------------------------------------
// Fortnite 42.30 offsets (CL-58557680-Windows)
// Source: UC dump pasted by user, cross-referenced with Classes dump
// Last updated: 2026-10-03
//
// If ESP shows nothing or garbage, GWorld or a per-struct offset drifted again.
// Pattern: UC posts updated offsets within hours of a Fortnite patch.
// -----------------------------------------------------------------------------

namespace fortnite
{
	namespace offsets
	{
		using offset_t = std::uintptr_t;

		// ---- GWorld + Engine globals (42.30) ----
		// 42.30 uses direct GWorld pointer. No XOR+rotl decryption anymore.
		inline offset_t gworld           = 0x1B2C5BA0;
		inline offset_t gengine          = 0x1B2C7518;

		// ---- UWorld fields ----
		inline offset_t game_instance    = 0x238;   // was 0x240
		inline offset_t game_state       = 0x1C0;   // was 0x1C8
		inline offset_t levels           = 0x1D8;   // was 0x1E0
		inline offset_t persistent_level = 0x38;
		inline offset_t actors           = 0x1A0;   // ULevel::Actors
		inline offset_t game_viewport    = 0xB70;
		inline offset_t seconds          = 0x188;   // UWorld::TimeSeconds (for visibility check)
		inline offset_t server_world_time= 0x2A0;

		// ---- UGameInstance / UGameViewportClient / ULocalPlayer ----
		inline offset_t local_players    = 0x38;    // same
		inline offset_t player_array     = 0x288;

		// ---- ULocalPlayer / APlayerController ----
		inline offset_t player_controller= 0x30;    // same
		inline offset_t local_pawn       = 0x318;
		inline offset_t pawn_private     = 0x2E8;

		// ---- APlayerController / APawn (shared) ----
		inline offset_t relative_location= 0x140;   // same
		inline offset_t location_pointer = 0x168;
		inline offset_t rotation_pointer = 0x178;
		inline offset_t relative_rotation= 0x158;   // kept from old; not in new paste
		inline offset_t root_component   = 0x1B0;   // same (verified in Classes dump)
		inline offset_t fov              = 0x374;

		// ---- AFortPlayerPawn / AFortPawn ----
		inline offset_t mesh             = 0x2F0;   // was 0x330
		inline offset_t player_state     = 0x290;   // was 0x2D0
		inline offset_t current_weapon   = 0x9D0;   // was 0x990
		inline offset_t team_index       = 0xF69;   // was 0x11C1
		inline offset_t b_is_dying       = 0x728;   // same
		inline offset_t b_is_dbno        = 0x881;   // was 0x841
		inline offset_t b_is_a_bot       = 0x27A;
		inline offset_t b_is_crouched    = 0x430;
		inline offset_t habanero_component = 0x918;
		inline offset_t last_render_time = 0x548;   // on Mesh component (for visibility)
		inline offset_t server_critical_health = 0x1BFC;

		// ---- APlayerState ----
		inline offset_t player_name      = 0x9E8;
		inline offset_t kill_score       = 0xF80;
		inline offset_t platform         = 0x400;

		// ---- AFortWeapon ----
		inline offset_t weapon_data      = 0x628;   // was 0x678
		inline offset_t ammo_count       = 0x1100;  // was 0x119C
		inline offset_t b_is_reloading   = 0x371;
		inline offset_t last_fire_time   = 0x1004;
		inline offset_t last_fire_time_verified = 0x100C;
		inline offset_t last_damaged_time= 0xDE8;
		inline offset_t projectile_speed = 0x2608;
		inline offset_t projectile_gravity = 0x260C;
		inline offset_t component_velocity = 0x188;

		// ---- aim / targeting / recoil ----
		inline offset_t targeted_fort_pawn        = 0x16C0; // was 0x1850
		inline offset_t location_under_reticle    = 0x21A0;
		inline offset_t net_connection            = 0x4A8;
		inline offset_t rotation_input            = 0x4B0;
		inline offset_t weapon_offset_correction  = 0x2360;
		inline offset_t weapon_recoil_offset      = 0x2348;
		inline offset_t player_aim_offset         = 0x2330;

		// ---- loot / pickups ----
		inline offset_t spawn_source_override     = 0xB78;
		inline offset_t searched_flags            = 0xCE2;
		inline offset_t search_text               = 0xD38; // was 0xD78
		inline offset_t chosen_random_upgrade     = 0xBC4;
		inline offset_t simulating_too_long_length= 0x290; // was 0x2D0
		inline offset_t primary_pickup_item_entry = 0x368; // was 0x3A8
		inline offset_t item_entry_item_definition= 0x10;
		inline offset_t item_definition_name      = 0x38;
		inline offset_t item_definition_data_list = 0x68;
		inline offset_t item_entry_item_data_list = 0x28;
		inline offset_t weapon_display_tier       = 0x296;
		inline offset_t rarity_struct             = 0x1873E5D8;
		inline offset_t pickup_flags              = 0x28C;
		inline offset_t pickup_extended_flags     = 0x28D;
		inline offset_t pickup_location_data      = 0x410;

		// ---- bones / skeletal mesh ----
		inline offset_t component_to_world              = 0x1E0;
		inline offset_t bone_array                      = 0x660;
		inline offset_t bone_array_cache                = 0x670;
		inline offset_t cached_component_space_transforms = 0x9D0;
		inline offset_t current_read_component_transforms = 0x48;

		// ---- backwards compat: fields our aimbot/visuals still reference ----
		// Not in new paste; kept at old values. If aimbot behavior is wrong
		// on 42.30 these are the likely culprits.
		inline offset_t camera_manager          = 0x368;   // WARN: not verified for 42.30
		inline offset_t weapon_core_animation   = 0x1770;  // WARN: not verified for 42.30
		inline offset_t building_state          = 0x21F0;  // WARN: not verified for 42.30
		inline offset_t aim_pitch_min           = 0x2148;  // only used by removed silent_aim
		inline offset_t aim_pitch_max           = 0x214C;  // only used by removed silent_aim
		inline offset_t view_yaw_min            = 0x27F4;  // only used by removed silent_aim
		inline offset_t initial_squad_size      = 0x19F8;  // WARN: not verified for 42.30
		inline offset_t revive_time_from_dbno   = 0x4AD8;  // was 0x4F98
		inline offset_t lifespan_after_death    = 0x10A8;  // was 0x1160
		inline offset_t in_storm_despawn_time   = 0xA90;   // WARN: not verified for 42.30
		inline offset_t vert_snap_grid_size     = 0x5CC;   // WARN: not verified for 42.30
		inline offset_t max_level               = 0x2EC;   // WARN: not verified for 42.30
		inline offset_t b_hit                   = 0x2E0;   // WARN: not verified for 42.30
		inline offset_t b_active                = 0x2E0;   // WARN: not verified for 42.30
		inline offset_t last_fired_direction    = 0x5B98;  // WARN: not verified for 42.30
		inline offset_t last_fired_location     = 0x5B80;  // WARN: not verified for 42.30
		inline offset_t current_projected_impact_distance = 0x10C0; // WARN
		inline offset_t aactors                 = 0x38;    // kept for legacy actor-array walk

		// ---- resolver state (unchanged) ----
		inline bool resolved = false;

		namespace candidates
		{
			// 42.30 primary + a couple historical fallbacks
			inline constexpr offset_t gworld_ptr[] = {
				0x1B2C5BA0,
				0x19D0D570,
				0x19401C60,
			};
			inline constexpr offset_t game_instance_off[] = { 0x238, 0x240, 0x248, 0x250 };
			inline constexpr offset_t levels_off[]        = { 0x1D8, 0x1E0, 0x1F0 };
			inline constexpr offset_t current_weapon_off[]= { 0x9D0, 0x990 };
			inline constexpr offset_t mesh_off[]          = { 0x2F0, 0x330 };
			inline constexpr offset_t weapon_data_off[]   = { 0x628, 0x678, 0x5F0, 0x600 };
			inline constexpr offset_t player_state_off[]  = { 0x290, 0x2D0, 0x2B8 };
			inline constexpr offset_t team_index_off[]    = { 0xF69, 0x11C1, 0x11B1 };
			inline constexpr offset_t ammo_count_off[]    = { 0x1100, 0x119C };
			inline constexpr offset_t targeted_pawn_off[] = { 0x16C0, 0x1850 };
			inline constexpr offset_t item_display_name_off[] = { 0x28, 0x30, 0x38, 0x40, 0x48, 0x50 };
			inline constexpr offset_t item_rarity_off[]   = { 0xAA, 0xA1 };
		}

		[[nodiscard]] constexpr offset_t add(offset_t base, offset_t off) { return base + off; }

		template <typename T = offset_t>
		[[nodiscard]] constexpr T ptr(T base, offset_t off) { return static_cast<T>(base + off); }

		[[nodiscard]] bool resolve_all(std::uintptr_t module_base);
	}
}
