// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <cstdint>
#include <intrin.h>

namespace fortnite
{
	namespace offsets
	{
		using offset_t = std::uintptr_t;

		inline offset_t gworld = 0x19D0D570;
		inline offset_t game_instance = 0x240;
		inline offset_t game_state = 0x1c8;
		inline offset_t levels = 0x1E0;
		inline offset_t local_players = 0x38;
		inline offset_t relative_location = 0x140;
		inline offset_t relative_rotation = 0x158;

		inline offset_t camera_manager = 0x368;
		inline offset_t current_weapon = 0x990;
		inline offset_t aactors = 0x38;
		inline offset_t simulating_too_long_length = 0x2d0;
		inline offset_t revive_time_from_dbno = 0x4f98;
		inline offset_t view_yaw_min = 0x27F4;
		inline offset_t aim_pitch_min = 0x2148;
		inline offset_t aim_pitch_max = 0x214C;
		inline offset_t team_index = 0x11c1;
		inline offset_t initial_squad_size = 0x19f8;
		inline offset_t weapon_data = 0x678;
		inline offset_t player_state = 0x2d0;
		inline offset_t mesh = 0x330;
		inline offset_t lifespan_after_death = 0x1160;
		inline offset_t search_text = 0xd78;
		inline offset_t in_storm_despawn_time_in_seconds = 0xa90;
		inline offset_t vert_snap_grid_size = 0x5cc;
		inline offset_t primary_pickup_item_entry = 0x3a8;
		inline offset_t root_component = 0x1b0;
		inline offset_t max_level = 0x2ec;
		inline offset_t b_hit = 0x2e0;
		inline offset_t b_active = 0x2e0;
		inline offset_t b_is_dying = 0x728;
		inline offset_t b_is_dbno = 0x841;
		inline offset_t last_fired_direction = 0x5b98;
		inline offset_t last_fired_location = 0x5b80;
		inline offset_t current_projected_impact_distance = 0x10c0;
		inline offset_t ammo_count = 0x119c;
		inline offset_t building_state = 0x21f0;
		inline offset_t weapon_core_animation = 0x1770;
		inline offset_t player_controller = 0x30;
		inline offset_t targeted_fort_pawn = 0x1850;

		

		inline std::uint64_t gworld_xor_key = 0xCF76574CLL;
		inline int gworld_rotl_bits = 48;

		inline bool resolved = false;

		[[nodiscard]] inline std::uint64_t decrypt_uworld( std::uint64_t encrypted )
		{

			encrypted ^= 0xC774FEull;
			encrypted = _rotl64( encrypted , 56 );              // == _rotr64(v, 8)
			encrypted ^= 0xF81BA2C7ull;
			encrypted *= 0x80CD98FC8EC532E9ull;
			return encrypted;
		}

		namespace candidates
		{
			inline constexpr offset_t gworld_ptr[] = {
				0x19401C60 ,
				0x178685D8 ,
				0x178C37A8 ,
			};
			inline constexpr std::uint64_t uworld_xor_key[] = {
				0x35E5647853A0CBFULL ,
				0xFFFFFFFFE869D197ULL ,
			};
			inline constexpr int uworld_rotl_bits[] = { 32 , 0 };

			inline constexpr offset_t game_instance_off[] = { 0x250 , 0x240 , 0x248 , 0x238 };
			inline constexpr offset_t levels_off[] = { 0x1f0 , 0x1e0 , 0x1d8 };

			inline constexpr offset_t current_weapon_off[] = { 0x990 , 0xa80 };
			inline constexpr offset_t weapon_data_off[] = {
				0x678 , 0x638 , 0x5c8 , 0x5f0 , 0x5f8 , 0x550 , 0x600
			};
			inline constexpr offset_t item_display_name_off[] = {
				0x28 , 0x30 , 0x38 , 0x40 , 0x48 , 0x50
			};
			inline constexpr offset_t item_rarity_off[] = { 0xaa , 0xa1 };

			inline constexpr offset_t team_index_off[] = { 0x11c1 , 0x11b1 };
			inline constexpr offset_t initial_squad_size_off[] = { 0x1970 , 0x1968 };
			inline constexpr offset_t player_state_off[] = { 0x2B8 , 0x2D0 };
		}

		[[nodiscard]] constexpr offset_t add( offset_t base , offset_t off )
		{
			return base + off;
		}
		template <typename T = offset_t>
		[[nodiscard]] constexpr T ptr( T base , offset_t off )
		{
			return static_cast< T >( base + off );
		}

		[[nodiscard]] bool resolve_all( std::uintptr_t module_base );
	}
}
