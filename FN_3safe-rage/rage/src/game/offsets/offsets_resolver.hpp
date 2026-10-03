// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "offsets.hpp"

#include "../primitives/primitives.hpp"
#include "../../../dependenices/memory/memory.hpp"

#include <algorithm>
#include <iostream>

namespace
{
	[[nodiscard]] bool ptr_ok_user( std::uint64_t p )
	{
		return p > 0x10000 && p < 0x00007FFFFFFFFFFFULL;
	}
}

bool fortnite::offsets::resolve_all( std::uintptr_t module_base )
{
	resolved = false;

	for ( const auto gw_off : candidates::gworld_ptr )
	{
		const std::uint64_t encrypted = communcations::read<std::uint64_t>( module_base + gw_off );
		if ( !encrypted )
			continue;

		for ( const int rot : candidates::uworld_rotl_bits )
		{
			for ( const std::uint64_t xk : candidates::uworld_xor_key )
			{
				gworld_xor_key = xk;
				gworld_rotl_bits = rot;

				const std::uint64_t uworld = decrypt_uworld( encrypted );
				if ( !ptr_ok_user( uworld ) )
					continue;

				const int max_offset_idx = (0x800 / 8) * 2 + 1; 

				for ( int lv_idx = 0; lv_idx < max_offset_idx; ++lv_idx )
				{
					const int lv_delta = (lv_idx == 0) ? 0 : (((lv_idx + 1) / 2) * 8 * ((lv_idx % 2 == 1) ? 1 : -1));
					if ( lv_delta < 0 && std::uintptr_t(-lv_delta) > levels ) continue;
					const std::uintptr_t lv_off = levels + lv_delta;

					const auto levels_arr = communcations::read<fortnite::ueegnine::tarray<std::uint64_t>>( uworld + lv_off );
					if ( !levels_arr.is_valid( ) )
						continue;

					const std::uint32_t cnt = levels_arr.get_count( );
					if ( cnt == 0 || cnt > 5000 )
						continue;

					const std::uint64_t level0 = levels_arr.get( 0 );
					if ( !ptr_ok_user( level0 ) )
						continue;

					for ( int a_idx = 0; a_idx < max_offset_idx; ++a_idx )
					{
						const int a_delta = (a_idx == 0) ? 0 : (((a_idx + 1) / 2) * 8 * ((a_idx % 2 == 1) ? 1 : -1));
						if ( a_delta < 0 && std::uintptr_t(-a_delta) > aactors ) continue;
						const std::uintptr_t actors_off = aactors + a_delta;

						const auto actors_arr = communcations::read<fortnite::ueegnine::tarray<std::uint64_t>>( level0 + actors_off );
						if ( !actors_arr.is_valid( ) )
							continue;

						const std::uint32_t actor_cnt = actors_arr.get_count( );
						if ( actor_cnt > 100000 ) 
							continue;

						for ( int gi_idx = 0; gi_idx < max_offset_idx; ++gi_idx )
						{
							const int gi_delta = (gi_idx == 0) ? 0 : (((gi_idx + 1) / 2) * 8 * ((gi_idx % 2 == 1) ? 1 : -1));
							if ( gi_delta < 0 && std::uintptr_t(-gi_delta) > game_instance ) continue;
							const std::uintptr_t gi_off = game_instance + gi_delta;

							const std::uint64_t gi = communcations::read<std::uint64_t>( uworld + gi_off );
							if ( !ptr_ok_user( gi ) )
								continue;

							for ( int gs_idx = 0; gs_idx < max_offset_idx; ++gs_idx )
							{
								const int gs_delta = (gs_idx == 0) ? 0 : (((gs_idx + 1) / 2) * 8 * ((gs_idx % 2 == 1) ? 1 : -1));
								if ( gs_delta < 0 && std::uintptr_t(-gs_delta) > game_state ) continue;
								const std::uintptr_t gs_off = game_state + gs_delta;

								const std::uint64_t gs = communcations::read<std::uint64_t>( uworld + gs_off );
								if ( !ptr_ok_user( gs ) )
									continue;

								gworld = gw_off;
								std::cout << std::hex << gw_off << std::endl;
								game_instance = gi_off;
								std::cout << std::hex << gi_off << std::endl;

								game_state = gs_off;
								std::cout << std::hex << gs_off << std::endl;

								levels = lv_off;
								std::cout << std::hex << lv_off << std::endl;

								aactors = actors_off;
								std::cout << std::hex << actors_off << std::endl;

								resolved = true;
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}
