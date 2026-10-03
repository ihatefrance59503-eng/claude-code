// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "aimbot.hpp"
#include "../../../settings/settings.hpp"
#include <map>
#include <cmath>

static bool on_screen( const fortnite::uemath::fvector2d& p , float margin )
{
	return p.x >= -margin && p.y >= -margin&& p.x <= fortnite::render::width + margin&& p.y <= fortnite::render::height + margin;
}

static float apply_aim_curve( float value , int curve_type )
{
	if ( curve_type == 0 ) return value; 

	float t = std::clamp( std::abs( value ) / 100.0f , 0.0f , 1.0f );
	float curved = 0.0f;

	switch ( curve_type )
	{
	case 1: 
		curved = t * t;
		break;
	case 2: 
		curved = t * ( 2.0f - t );
		break;
	case 3: 
		curved = t < 0.5f ? 2.0f * t * t : -1.0f + ( 4.0f - 2.0f * t ) * t;
		break;
	default:
		curved = t;
	}

	return value < 0 ? -curved * 100.0f : curved * 100.0f;
}

static float calculate_distance_scaling( float distance )
{
	if ( !fortnite::settings::aimbot::use_distance_scaling )
		return 1.0f;

	float max_dist = fortnite::settings::aimbot::max_distance;
	float scaling = 1.0f - ( distance / max_dist );
	return std::clamp( scaling , 0.1f , 1.0f );
}

static fortnite::uemath::fvector predict_position(const fortnite::uemath::fvector& current_pos ,const fortnite::uemath::fvector& velocity ,float prediction_strength )
{
	if ( !fortnite::settings::aimbot::predict_movement )
		return current_pos;

	return fortnite::uemath::fvector {};
}

static fortnite::uemath::fvector get_target_location(
	const fortnite::entity::actor_data& actor )
{
	auto location = fortnite::engine::bone::get_bone_location(
		actor.mesh ,
		fortnite::settings::aimbot::aim_bone );

	if ( fortnite::settings::aimbot::closest_bone )
	{
		auto head = fortnite::engine::bone::get_bone_location( actor.mesh , 110 );
		auto chest = fortnite::engine::bone::get_bone_location( actor.mesh , 7 );
		auto pelvis = fortnite::engine::bone::get_bone_location( actor.mesh , 0 );

		float head_dist = fortnite::engine::camera::location.distance( head ) / 100.0f;

		float chest_dist = fortnite::engine::camera::location.distance( chest ) / 100.0f;

		float pelvis_dist = fortnite::engine::camera::location.distance( pelvis ) / 100.0f;

		if ( head_dist < chest_dist && head_dist < pelvis_dist )
			location = head;
		else if ( chest_dist < pelvis_dist )
			location = chest;
		else
			location = pelvis;
	}

	return location;
}

static bool should_target_actor( const fortnite::entity::actor_data actor )
{
	if ( !actor.root_component )
		return false;

	// Check DBNO state
	if ( !fortnite::settings::aimbot::target_downed )
	{
		// Assume there's a way to check if player is downed
		// if ( is_player_downed( actor ) ) return false;
	}

	// Check visibility
	if ( !fortnite::settings::aimbot::target_invisible )
	{
		// Perform line trace to check visibility
		// if ( !is_visible( actor ) ) return false;
	}

	// Check team
	if ( !fortnite::settings::aimbot::target_teammates )
	{
		// if ( is_same_team( actor ) ) return false;
	}

	// Check if bot
	if ( !fortnite::settings::aimbot::target_bots )
	{
		// if ( is_bot( actor ) ) return false;
	}

	return true;
}

static float calculate_target_score(float screen_x , float screen_y ,float distance ,int targeting_mode )
{
	const float center_x = fortnite::render::width * 0.5f;
	const float center_y = fortnite::render::height * 0.5f;

	float dx = screen_x - center_x;
	float dy = screen_y - center_y;
	float crosshair_distance = std::sqrt( dx * dx + dy * dy );

	switch ( targeting_mode )
	{
	case 0:
		return crosshair_distance;

	case 1: 
		return distance;

	case 2: 
		return ( crosshair_distance * 0.6f ) + ( distance * 0.4f );

	default:
		return crosshair_distance;
	}
}
inline float normalize_angle( float angle )
{
	while ( angle > 180.0f ) angle -= 360.0f;
	while ( angle < -180.0f ) angle += 360.0f;
	return angle;
}

inline fortnite::uemath::frotator find_look_at_rotation(const fortnite::uemath::fvector& start ,const fortnite::uemath::fvector& target )
{
	fortnite::uemath::fvector direction {};

	direction.x = target.x - start.x;
	direction.y = target.y - start.y;
	direction.z = target.z - start.z;

	double length = sqrt(direction.x * direction.x +direction.y * direction.y +direction.z * direction.z);

	if ( length < 0.001 )
		return {};

	direction.x /= length;
	direction.y /= length;
	direction.z /= length;

	double yaw = atan2( direction.y , direction.x ) * ( 180.0 / std::numbers::pi );

	double pitch = -atan2(direction.z ,sqrt( direction.x * direction.x + direction.y * direction.y )) * ( 180.0 / std::numbers::pi );

	return fortnite::uemath::frotator(( float ) pitch ,( float ) yaw ,0.0f);
}
static void get_weapon_info(float& out_fov ,float& out_smooth_x ,float& out_smooth_y , const uint64_t local_pawn )
{
	auto current_weapon = fortnite::communcations::read<uint64_t>( local_pawn + fortnite::offsets::current_weapon );

	out_fov = fortnite::settings::aimbot::fov_size;
	out_smooth_x = fortnite::settings::aimbot::smooth_x;
	out_smooth_y = fortnite::settings::aimbot::smooth_y;
	if ( fortnite::settings::aimbot::weapon_configs ) {
		if ( !current_weapon )
			return;

		auto weapon_type = fortnite::communcations::read<EFortWeaponCoreAnimation>( current_weapon + fortnite::offsets::weapon_core_animation );


		switch ( weapon_type )
		{
		case EFortWeaponCoreAnimation::Shotgun:
			out_fov = fortnite::settings::aimbot::shotgun_fov;
			out_smooth_x = fortnite::settings::aimbot::shotgun_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::shotgun_smooth_y;
			break;

		case EFortWeaponCoreAnimation::SniperRifle:
			out_fov = fortnite::settings::aimbot::sniper_fov;
			out_smooth_x = fortnite::settings::aimbot::sniper_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::sniper_smooth_y;
			break;

		case EFortWeaponCoreAnimation::AssaultRifle:
		case EFortWeaponCoreAnimation::AR_BullPup:
		case EFortWeaponCoreAnimation::AR_DrumGun:
			out_fov = fortnite::settings::aimbot::ar_fov;
			out_smooth_x = fortnite::settings::aimbot::ar_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::ar_smooth_y;
			break;

		case EFortWeaponCoreAnimation::Rifle:
			out_fov = fortnite::settings::aimbot::rifle_fov;
			out_smooth_x = fortnite::settings::aimbot::rifle_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::rifle_smooth_y;
			break;

		case EFortWeaponCoreAnimation::MachinePistol:
			out_fov = fortnite::settings::aimbot::muzzle_fov;
			out_smooth_x = fortnite::settings::aimbot::muzzle_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::muzzle_smooth_y;
			break;

		case EFortWeaponCoreAnimation::GrenadeLauncher:
		case EFortWeaponCoreAnimation::ShoulderLauncher:
			out_fov = fortnite::settings::aimbot::grenade_fov;
			out_smooth_x = fortnite::settings::aimbot::grenade_smooth_x;
			out_smooth_y = fortnite::settings::aimbot::grenade_smooth_y;
			break;

		default:
			break;
		}
	}
	else {
		return;
	}
}
inline void draw_target_line( float target_screen_x , float target_screen_y )
{
	if ( !fortnite::settings::aimbot::show_target_line )
		return;

	float center_x = fortnite::render::width * 0.5f;
	float center_y = fortnite::render::height * 0.5f;

	auto draw_list = ImGui::GetBackgroundDrawList( );
	if ( !draw_list )
		return;

	float* color = fortnite::settings::aimbot::target_line_color;

	draw_list->AddLine(ImVec2( center_x , center_y ) ,ImVec2( target_screen_x , target_screen_y ) ,ImColor(color [ 0 ] * 255.f ,color [ 1 ] * 255.f ,color [ 2 ] * 255.f ,color [ 3 ] * 255.f) ,1.5f);
}

void fortnite::aimbot::tick( )
{
	static uint64_t last_target = 0;

	if ( !fortnite::settings::aimbot::aimbot )
		return;
	auto current_weapon = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::current_weapon );

	auto weapon_type = fortnite::communcations::read<EFortWeaponCoreAnimation>( current_weapon + fortnite::offsets::weapon_core_animation );
	auto current_ammo = fortnite::communcations::read<int32_t>( current_weapon + fortnite::offsets::ammo_count );
	auto building_state = fortnite::communcations::read<EFortBuildingState>( fortnite::entity::local_pawn + fortnite::offsets::building_state );

	if ( fortnite::settings::aimbot::disable_on_zero_ammo &&current_ammo <= 0 )
	{
		return;
	}
	if ( fortnite::settings::aimbot::disable_on_pickaxe && ( weapon_type == EFortWeaponCoreAnimation::Melee || weapon_type == EFortWeaponCoreAnimation::Unarmed ) )
	{
		return;
	}
	if ( fortnite::settings::aimbot::disable_on_build_mode && ( building_state == EFortBuildingState::EditMode|| building_state == EFortBuildingState::Placement ) )
	{
		return;
	}
	if ( !( GetAsyncKeyState( fortnite::settings::aimbot::hotkey ) & 0x8000 ) )
		return;

	auto actors = fortnite::entity::get_a( );
	if ( !actors || actors->empty( ) )
		return;

	auto local_pawn = fortnite::entity::local_pawn;
	if ( !local_pawn )
		return;

	const float center_x = fortnite::render::width * 0.5f;
	const float center_y = fortnite::render::height * 0.5f;

	float fov_size , smooth_x , smooth_y;

	get_weapon_info( fov_size , smooth_x , smooth_y , local_pawn );
	fortnite::settings::aimbot::fov_size = fov_size;
	struct BestTarget
	{
		uint64_t actor = 0;
		fortnite::uemath::fvector location;
		float distance = FLT_MAX;
		float score = FLT_MAX;
		bool found = false;
	} best_target;

	for ( const auto& actor : *actors )
	{
		if ( !should_target_actor( actor ) )
			continue;

		auto location = get_target_location( actor );

		auto screen = fortnite::engine::camera::world_to_screen( location );

		if ( !on_screen( fortnite::uemath::fvector2d( screen.x , screen.y ) , 150 ) )
			continue;

		float dist_m = fortnite::engine::camera::location.distance( location ) / 100.0f;

		if ( dist_m < fortnite::settings::aimbot::min_distance )
			continue;

		if ( dist_m > fortnite::settings::aimbot::max_distance )
			continue;

		float dx = screen.x - center_x;
		float dy = screen.y - center_y;

		float screen_distance = std::sqrt( ( dx * dx ) + ( dy * dy ) );

		if ( screen_distance > fov_size )
			continue;

		float score = calculate_target_score(screen.x ,screen.y ,dist_m ,fortnite::settings::aimbot::targeting_mode );

		if ( score < best_target.score )
		{
			best_target.actor = actor.current;
			best_target.distance = dist_m;
			best_target.score = score;
			best_target.location = location;
			best_target.found = true;
		}
	}

	if ( !best_target.found )
	{
		last_target = 0;
		return;
	}
	if ( fortnite::settings::aimbot::show_targeting_notification ) {
		if ( last_target != best_target.actor )
		{
			last_target = best_target.actor;

			widgets->notify.add( "aimbot target" , "locked onto target" , notify_info );
		}
	}

	auto final_screen = fortnite::engine::camera::world_to_screen( best_target.location );
	if ( fortnite::settings::aimbot::show_target_line ) {
		draw_target_line( final_screen.x , final_screen.y );
	}

	float delta_x = final_screen.x - center_x;
	float delta_y = final_screen.y - center_y;

	float deadzone_check = std::sqrt( ( delta_x * delta_x ) + ( delta_y * delta_y ) );

	if ( deadzone_check < fortnite::settings::aimbot::deadzone )
		return;

	delta_x = apply_aim_curve( delta_x , fortnite::settings::aimbot::aim_curve );
	delta_y = apply_aim_curve( delta_y , fortnite::settings::aimbot::aim_curve );

	float distance_scale = calculate_distance_scaling( best_target.distance );

	float current_smooth_x = smooth_x;
	float current_smooth_y = smooth_y;

	if ( fortnite::settings::aimbot::enable_close_aim &&
		best_target.distance < fortnite::settings::aimbot::close_aim_distance )
	{
		current_smooth_x = fortnite::settings::aimbot::close_aim_smooth_x;
		current_smooth_y = fortnite::settings::aimbot::close_aim_smooth_y;

		delta_x *= fortnite::settings::aimbot::close_aim_multiplier;
		delta_y *= fortnite::settings::aimbot::close_aim_multiplier;
	}

	current_smooth_x = std::clamp( current_smooth_x , 0.0f , 1.0f );
	current_smooth_y = std::clamp( current_smooth_y , 0.0f , 1.0f );

	float factor_x = 1.0f - current_smooth_x;
	float factor_y = 1.0f - current_smooth_y;

	factor_x = factor_x * factor_x;
	factor_y = factor_y * factor_y;

	factor_x = max( factor_x , 0.01f );
	factor_y = max( factor_y , 0.01f );

	factor_x *= distance_scale;
	factor_y *= distance_scale;

	if ( fortnite::settings::aimbot::acceleration )
	{
		factor_x *= fortnite::settings::aimbot::acceleration_factor;
		factor_y *= fortnite::settings::aimbot::acceleration_factor;
	}

	float move_x = delta_x * factor_x;
	float move_y = delta_y * factor_y;

	if ( std::abs( move_x ) > 0.001f || std::abs( move_y ) > 0.001f )
	{
		if ( std::abs( move_x ) > 0.001f || std::abs( move_y ) > 0.001f )
		{
			auto world_data = fortnite::world::get( );
			if ( !world_data )
				return;

			uint64_t local_player = world_data->local_players.get( 0 );
			auto player_controller = communcations::read<uint64_t>( local_player + fortnite::offsets::player_controller );
			if ( !player_controller )
				return;

			auto current_rotation = fortnite::communcations::read<fortnite::uemath::frotator>(player_controller + 0x26f0);

			fortnite::uemath::frotator new_rotation {};

			new_rotation.pitch = current_rotation.pitch - ( move_y * 0.01f );
			new_rotation.yaw = current_rotation.yaw + ( move_x * 0.01f );
			new_rotation.roll = 0.0f;
			fortnite::communcations::write<fortnite::uemath::frotator>(player_controller + 0x26f0 ,new_rotation);
		}
	}
}
float fortnite::aimbot::calculate_fov_distance( uemath::fvector2d screen_pos )
{
	float center_x = static_cast< float >( fortnite::render::width ) / 2.0f;
	float center_y = static_cast< float >( fortnite::render::height ) / 2.0f;
	float dx = static_cast< float >( screen_pos.x ) - center_x;
	float dy = static_cast< float >( screen_pos.y ) - center_y;
	return sqrtf( dx * dx + dy * dy );
}