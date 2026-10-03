// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "health.hpp"

namespace
{
	struct state
	{
		float hp = 100.0f;
		float shield = 0.0f;
		float last_damaged_time = 0.0f;
		float last_shield_damaged_time = 0.0f;
		bool was_healing = false;
	};

	static std::unordered_map<std::uint64_t , state> g_state;

	[[nodiscard]] float clampf( float v , float lo , float hi )
	{
		return ( std::max ) ( lo , ( std::min ) ( v , hi ) );
	}

	[[nodiscard]] bool is_healing_weapon( const std::string& weapon_name )
	{
		if ( weapon_name.empty( ) )
			return false;

		std::string lower = weapon_name;
		std::transform( lower.begin( ) , lower.end( ) , lower.begin( ) , ::tolower );

		return lower.find( "med" ) != std::string::npos ||
			lower.find( "bandage" ) != std::string::npos ||
			lower.find( "shield" ) != std::string::npos ||
			lower.find( "potion" ) != std::string::npos ||
			lower.find( "slurp" ) != std::string::npos ||
			lower.find( "chug" ) != std::string::npos ||
			lower.find( "heal" ) != std::string::npos ||
			lower.find( "campfire" ) != std::string::npos ||
			lower.find( "first aid" ) != std::string::npos ||
			lower.find( "apple" ) != std::string::npos ||
			lower.find( "fish" ) != std::string::npos;
	}

	[[nodiscard]] float damage_for_core_anim( std::uint8_t core )
	{
		switch ( core )
		{
		case 0:  // DMR
			return 45.0f;
		case 1:  // Pistol
			return 24.0f;
		case 2:  // Shotgun
			return 80.0f;
		case 3:  // SMG
			return 18.0f;
		case 4:  // Rifle
			return 38.0f;
		case 5:  // Sniper
			return 105.0f;
		case 6:  // MachinePistol
			return 16.0f;
		case 7:  // GrenadeLauncher
			return 95.0f;
		case 8:  // ExplosiveRifle
			return 65.0f;
		case 9:  // Crossbow
			return 72.0f;
		case 10: // AssaultRifle
			return 35.0f;
		case 11: // TacticalShotgun
			return 70.0f;
		case 12: // SniperRifle
			return 115.0f;
		case 13: // LMG
			return 22.0f;
		case 14: // Minigun
			return 24.0f;
		case 15: // Trap
			return 75.0f;
		case 16: // Tool
			return 0.0f;
		case 17: // Consumable
			return 0.0f;
		case 18: // Thrown
			return 50.0f;
		case 19: // VehicleWeapon
			return 40.0f;
		case 20: // AR_BullPup
			return 36.0f;
		case 21: // AR_ForwardGrip
			return 37.0f;
		case 22: // AR_FlintlockRifle
			return 48.0f;
		case 23: // SemiAutoSniper
			return 75.0f;
		case 24: // AR_DrumGun
			return 40.0f;
		case 25: // Harpoon
			return 45.0f;
		case 26: // RailGun
			return 90.0f;
		case 27: // LugerSMG
			return 19.0f;
		case 28: // GeometryGun
			return 55.0f;
		case 29: // BallisticShield
			return 0.0f;
		case 30: // OrbitalStrike
			return 120.0f;
		case 31: // Jetpack
			return 0.0f;
		case 32: // Grappler
			return 0.0f;
		case 33: // Shield
			return 0.0f;
		case 34: // Unarmed
			return 0.0f;
		default:
			return 20.0f;
		}
	}

	[[nodiscard]] float scale_for_distance( float distance_cm )
	{
		float distance_m = distance_cm / 100.0f;

		if ( distance_m >= 200.0f )
			return 0.65f;
		if ( distance_m >= 175.0f )
			return 0.72f;
		if ( distance_m >= 150.0f )
			return 0.80f;
		if ( distance_m >= 125.0f )
			return 0.87f;
		if ( distance_m >= 100.0f )
			return 0.92f;
		if ( distance_m >= 75.0f )
			return 0.97f;
		return 1.0f;
	}

	[[nodiscard]] float get_game_time( )
	{
		auto world = fortnite::world::get( );
		if ( !world || !world->gworld )
			return 0.0f;

		uint64_t game_state = fortnite::communcations::read<uint64_t>( world->gworld + fortnite::offsets::game_state );
		if ( !game_state )
			return 0.0f;

		return fortnite::communcations::read<float>( game_state + 0x308 );
	}

	[[nodiscard]] float calculate_shield_for_game_time( float game_time )
	{
		if ( game_time < 60.0f )
			return 0.0f;
		if ( game_time < 300.0f )
			return 25.0f;
		if ( game_time < 600.0f )
			return 50.0f;
		if ( game_time < 1200.0f )
			return 75.0f;
		return 100.0f;
	}

	[[nodiscard]] float get_player_current_health( std::uint64_t actor )
	{
		// Try reading from ability system first (most accurate)
		uint64_t ability_system = fortnite::communcations::read<uint64_t>( actor + 0x1818 );
		if ( ability_system )
		{
			// Health attribute offset - adjust if needed
			float health = fortnite::communcations::read<float>( ability_system + 0x2c8 );
			if ( health > 0.0f )
				return health;
		}

		// Fallback to state map
		auto& st = g_state [ actor ];
		return st.hp;
	}

	[[nodiscard]] float get_player_current_shield( std::uint64_t actor )
	{
		// Try reading from ability system first
		uint64_t ability_system = fortnite::communcations::read<uint64_t>( actor + 0x1818 );
		if ( ability_system )
		{
			// Shield attribute offset - adjust if needed
			float shield = fortnite::communcations::read<float>( ability_system + 0x2d8 );
			if ( shield > 0.0f )
				return clampf( shield , 0.0f , 100.0f );
		}

		// Fallback to state map
		auto& st = g_state [ actor ];
		return st.shield;
	}

	[[nodiscard]] std::uint8_t get_weapon_core( std::uint64_t weapon )
	{
		if ( !weapon )
			return 34; // Unarmed

		// Core animation type offset
		std::uint8_t core = fortnite::communcations::read<std::uint8_t>( weapon + fortnite::offsets::weapon_core_animation );
		return core;
	}
}

fortnite::health::bar fortnite::health::get_bar( std::uint64_t actor , float distance_cm )
{
	bar out {};
	out.value = 100.0f;
	out.max = 100.0f;
	out.shield = 0.0f;
	out.max_shield = 100.0f;
	out.dbno = false;
	out.is_healing = false;

	if ( !actor )
		return out;

	const float revive_from_dbno_time = fortnite::communcations::read<float>( actor + 0x4e10 );
	const float dbno_start_time = fortnite::communcations::read<float>( actor + 0x4e18 );
	const float death_time = fortnite::communcations::read<float>( actor + 0x4e1c );

	if ( dbno_start_time > 0.0f && death_time <= 0.0f && revive_from_dbno_time > 0.0f )
	{
		out.dbno = true;
		out.max = 10.0f;
		out.value = clampf( revive_from_dbno_time - dbno_start_time , 0.0f , out.max );
		out.shield = 0.0f;
		out.max_shield = 0.0f;
		return out;
	}

	// Get timing information for damage detection
	const float last_damaged = fortnite::communcations::read<float>( actor + 0xce0 );
	auto& st = g_state [ actor ];

	// Get weapon information
	const std::uint64_t weapon = fortnite::communcations::read<std::uint64_t>( actor + fortnite::offsets::current_weapon );
	std::string weapon_name = "";

	if ( weapon )
	{
		const std::uint64_t weapon_data = fortnite::communcations::read<std::uint64_t>( weapon + fortnite::offsets::weapon_data );
		if ( weapon_data )
		{
			ueegnine::ftext weapon_text = fortnite::communcations::read<ueegnine::ftext>( weapon_data + 0x38 );
			weapon_name = weapon_text.get( );
		}
	}

	out.is_healing = is_healing_weapon( weapon_name );

	// Initialize state if new actor
	if ( st.last_damaged_time == 0.0f )
	{
		st.hp = 100.0f;
		st.shield = calculate_shield_for_game_time( get_game_time( ) );
		st.last_damaged_time = last_damaged;
		st.last_shield_damaged_time = last_damaged;
	}
	// Detect damage event
	else if ( last_damaged > st.last_damaged_time + 0.0001f )
	{
		//PlaySoundA( "C:\\Users\\dark\\Downloads\\pop.wav" , NULL , SND_FILENAME | SND_ASYNC );
		const std::uint8_t core = get_weapon_core( weapon );
		const float base_dmg = damage_for_core_anim( core );
		const float scaled_dmg = base_dmg * scale_for_distance( distance_cm );

		// Damage shield first, then overflow to health
		if ( st.shield > 0.0f )
		{
			if ( st.shield >= scaled_dmg )
			{
				st.shield -= scaled_dmg;
			}
			else
			{
				float overflow = scaled_dmg - st.shield;
				st.shield = 0.0f;
				st.hp = clampf( st.hp - overflow , 0.0f , 100.0f );
			}
		}
		else
		{
			st.hp = clampf( st.hp - scaled_dmg , 0.0f , 100.0f );
		}

		st.last_damaged_time = last_damaged;
		st.last_shield_damaged_time = last_damaged;
	}
	// Reset if no damage for 5+ seconds (player reset state)
	else if ( last_damaged < st.last_damaged_time - 5.0f )
	{
		float game_time = get_game_time( );
		st.hp = 100.0f;
		st.shield = calculate_shield_for_game_time( game_time );
		st.last_damaged_time = last_damaged;
		st.last_shield_damaged_time = last_damaged;
	}

	// Get current values (with fallback to ability system)
	out.value = get_player_current_health( actor );
	out.max = 100.0f;
	out.shield = get_player_current_shield( actor );
	out.max_shield = 100.0f;

	return out;
}