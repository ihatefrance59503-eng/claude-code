// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "sdk.hpp"


void fortnite::engine::camera::setup_camera( )
{
	auto world_data = fortnite::world::get( );
	if ( !world_data )
		return;
	uint64_t local_player = world_data->local_players.get( 0 );
	ueegnine::tarray<uint64_t> view_matrix = communcations::read<ueegnine::tarray<uint64_t>>( local_player + 0xd0 );
	camera::view_matrix = view_matrix.get( 1 );
}


void fortnite::engine::camera::update_camera( )
{
	auto projection = communcations::read<ueegnine::fmatrix>( camera::view_matrix + 0x940 );

	camera::rotation.pitch = asin( projection.z_plane.w ) * 180.0f / std::numbers::pi;
	camera::rotation.yaw = atan2( projection.y_plane.w , projection.x_plane.w ) * 180.0f / std::numbers::pi;
	camera::rotation.roll = 0.0f;

	camera::location.x = projection.m [ 3 ][ 0 ];
	camera::location.y = projection.m [ 3 ][ 1 ];
	camera::location.z = projection.m [ 3 ][ 2 ];

	auto fov_radians = 2.0f * atanf( 1.0f / static_cast< float >( communcations::read<double>( camera::view_matrix + 0x740 ) ) );
	camera::fov = fov_radians * 180.0f / std::numbers::pi;
}

fortnite::uemath::fvector2d fortnite::engine::camera::world_to_screen( uemath::fvector location )
{
	D3DMATRIX rotation_matrix = ueegnine::create_rotation_matrix( camera::rotation );
	auto& axis_x = rotation_matrix.m [ 0 ];
	auto& axis_y = rotation_matrix.m [ 1 ];
	auto& axis_z = rotation_matrix.m [ 2 ];
	uemath::fvector delta = location - camera::location;
	double transformed_x = delta.dot( uemath::fvector( axis_y [ 0 ] , axis_y [ 1 ] , axis_y [ 2 ] ) );
	double transformed_y = delta.dot( uemath::fvector( axis_z [ 0 ] , axis_z [ 1 ] , axis_z [ 2 ] ) );
	double transformed_z = max( delta.dot( uemath::fvector( axis_x [ 0 ] , axis_x [ 1 ] , axis_x [ 2 ] ) ) , 1.0f );
	float aspect_ratio = static_cast< float >( render::width ) / render::height;
	float inv_fov = tanf( camera::fov * static_cast< float >( std::numbers::pi ) / 360.0f ) / aspect_ratio;
	double screen_x = render::width / 2 + transformed_x * ( render::height / 2 / inv_fov ) / transformed_z;
	double screen_y = render::height / 2 - transformed_y * ( render::height / 2 / inv_fov ) / transformed_z;

	return uemath::fvector2d( screen_x , screen_y );
}

fortnite::uemath::fvector fortnite::engine::bone::get_bone_location( uint64_t mesh , int32_t index )
{
	uintptr_t bone_array = communcations::read<uintptr_t>( mesh + 0x628 );
	if ( bone_array == 0 )
		bone_array = communcations::read<uintptr_t>( mesh + 0x628 + 0x10 );
	fortnite::ueegnine::ftransform bone = communcations::read<ueegnine::ftransform>( bone_array + ( index * 0x60 ) );
	fortnite::ueegnine::ftransform component_to_world = communcations::read<ueegnine::ftransform>( mesh + 0x1E0 );
	D3DMATRIX matrix = fortnite::ueegnine::matrix_multiplication( bone.to_matrix_with_scale( ) , component_to_world.to_matrix_with_scale( ) );
	return uemath::fvector( matrix._41 , matrix._42 , matrix._43 );
}

fortnite::engine::bone::fbox_sphere_bounds fortnite::engine::bone::get_bounds( uint64_t mesh )
{
	fbox_sphere_bounds bounds { };
	bounds.origin = communcations::read<uemath::fvector>( mesh + 0x108 );
	bounds.box_extent = communcations::read<uemath::fvector>( mesh + 0x120 );
	bounds.sphere_radius = communcations::read<double>( mesh + 0x138 );

	return bounds;
}

struct DWORD64TArray
{
	uintptr_t Array;
	int ArrayCount;
};

std::string fortnite::engine::helpers::decrypt_name( uint64_t player_state , int in_lobby )
{
	std::int64_t ftext;
	int length;
	auto state = player_state;




	if ( in_lobby ) {
		length = communcations::read< int >( state + 0x348 + 0x8 );
		if ( !length || length > 100 )
			return "BOT";

		ftext = communcations::read< uintptr_t >( state + 0x348 );
		if ( !ftext )
			return "BOT";

		if ( length <= 32 ) {
			wchar_t stack_buffer [ 33 ] = { 0 };
			if ( !communcations::read_memory( ( PVOID ) ftext , stack_buffer , length * sizeof( wchar_t ) ) ) {
				return "BOT";
			}

			try {
				auto v6 = ( std::int64_t ) length;
				char v21;
				int v22;
				int i;
				int v25;
				WORD* v23;

				v21 = v6 - 1;
				if ( !( WORD ) v6 )
					v21 = 0;
				v22 = 0;
				v23 = ( WORD* ) stack_buffer;

				for ( i = ( v21 ) & 3; ; *v23++ += i & 7 ) {
					v25 = v6 - 1;
					if ( !( WORD ) v6 )
						v25 = 0;
					if ( v22 >= v25 )
						break;
					i += 3;
					++v22;
				}

				std::wstring conversion( stack_buffer );
				return std::string( conversion.begin( ) , conversion.end( ) );
			}
			catch ( ... ) {
				return "BOT";
			}
		}

		std::unique_ptr< wchar_t [] > dst( new ( std::nothrow ) wchar_t [ length + 1 ] );
		if ( !dst ) return "BOT";

		if ( !communcations::read_memory( ( PVOID ) ftext , dst.get( ) , length * sizeof( wchar_t ) ) ) {
			return "BOT";
		}

		try {
			dst [ length ] = L'\0';

			auto v6 = ( std::int64_t ) length;
			char v21;
			int v22;
			int i;
			int v25;
			WORD* v23;

			v21 = v6 - 1;
			if ( !( WORD ) v6 )
				v21 = 0;
			v22 = 0;
			v23 = ( WORD* ) ( dst.get( ) );

			for ( i = ( v21 ) & 3; ; *v23++ += i & 7 ) {
				v25 = v6 - 1;
				if ( !( WORD ) v6 )
					v25 = 0;
				if ( v22 >= v25 )
					break;
				i += 3;
				++v22;
			}

			std::wstring conversion( dst.get( ) );
			return std::string( conversion.begin( ) , conversion.end( ) );
		}
		catch ( ... ) {
			return "BOT";
		}
	}
	else {
		auto fstring = communcations::read< std::int64_t >( state + 0xA08 );

		if ( !fstring )
			return std::string( "BOT" );

		length = communcations::read< int >( fstring + 0x10 );
		ftext = ( std::uintptr_t ) communcations::read< std::int64_t >( fstring + 0x8 );

		if ( !ftext || length <= 0 || length > 50 )
			return std::string( "BOT" );

		if ( length <= 32 ) {
			wchar_t stack_buffer [ 33 ] = { 0 };
			if ( !communcations::read_memory( ( PVOID ) ftext , stack_buffer , length * sizeof( wchar_t ) ) ) {
				return "BOT";
			}

			try {
				auto v6 = ( std::int64_t ) length;
				char v21;
				int v22;
				int i;
				int v25;
				WORD* v23;

				v21 = v6 - 1;
				if ( !( WORD ) v6 )
					v21 = 0;
				v22 = 0;
				v23 = ( WORD* ) stack_buffer;

				for ( i = ( v21 ) & 3; ; *v23++ += i & 7 ) {
					v25 = v6 - 1;
					if ( !( WORD ) v6 )
						v25 = 0;
					if ( v22 >= v25 )
						break;
					i += 3;
					++v22;
				}

				std::wstring conversion( stack_buffer );
				return std::string( conversion.begin( ) , conversion.end( ) );
			}
			catch ( ... ) {
				return "BOT";
			}
		}

		std::unique_ptr< wchar_t [] > dst( new ( std::nothrow ) wchar_t [ length + 1 ] );
		if ( !dst ) return "BOT";

		if ( !communcations::read_memory( ( PVOID ) ftext , dst.get( ) , length * sizeof( wchar_t ) ) ) {
			return "BOT";
		}

		try {
			dst [ length ] = L'\0';

			auto v6 = ( std::int64_t ) length;
			char v21;
			int v22;
			int i;
			int v25;
			WORD* v23;

			v21 = v6 - 1;
			if ( !( WORD ) v6 )
				v21 = 0;
			v22 = 0;
			v23 = ( WORD* ) ( dst.get( ) );

			for ( i = ( v21 ) & 3; ; *v23++ += i & 7 ) {
				v25 = v6 - 1;
				if ( !( WORD ) v6 )
					v25 = 0;
				if ( v22 >= v25 )
					break;
				i += 3;
				++v22;
			}

			std::wstring conversion( dst.get( ) );
			return std::string( conversion.begin( ) , conversion.end( ) );
		}
		catch ( ... ) {
			return "BOT";
		}
	}

	return std::string( "BOT" );
}
std::string fortnite::engine::helpers::get_platform( uint64_t player_state )
{
	std::uint64_t src;
	int size;
	const auto& state = player_state;
	if ( state ) {
		size = communcations::read<int>( state + 0x440 + 0x8 );
		if ( !size || size > 100 )
			return "";
		src = communcations::read<std::uint64_t>( state + 0x440 );
		if ( !src )
			return "";
		if ( size > 0 && size < 100 ) {
			std::unique_ptr<wchar_t []> dst( new ( std::nothrow ) wchar_t [ size + 1 ] );
			communcations::read_memory( ( PVOID ) src , dst.get( ) , size * sizeof( wchar_t ) );
			dst [ size ] = L'\0';
			std::wstring conversion( dst.get( ) );
			return std::string( conversion.begin( ) , conversion.end( ) );
		}
	}
	return "";
}



ImColor fortnite::engine::helpers::get_platform_color( const std::string& platform )
{
	if ( platform.find( encrypt( "WIN" ) ) != std::string::npos )
		return ImColor( 255 , 255 , 255 );       // White

	if ( platform.find( encrypt( "XBL" ) ) != std::string::npos ||
		platform.find( encrypt( "XSX" ) ) != std::string::npos )
		return ImColor( 34 , 139 , 34 );         // Xbox green

	if ( platform.find( encrypt( "PSN" ) ) != std::string::npos ||
		platform.find( encrypt( "PS5" ) ) != std::string::npos )
		return ImColor( 30 , 144 , 255 );        // PlayStation blue

	if ( platform.find( encrypt( "SWT" ) ) != std::string::npos )
		return ImColor( 237 , 36 , 52 );         // Switch red

	if ( platform.find( encrypt( "MAC" ) ) != std::string::npos )
		return ImColor( 100 , 149 , 237 );       // MacOS light blue

	if ( platform.find( encrypt( "LNX" ) ) != std::string::npos )
		return ImColor( 255 , 223 , 0 );         // Linux gold

	if ( platform.find( encrypt( "IOS" ) ) != std::string::npos )
		return ImColor( 0 , 255 , 255 );         // iOS cyan

	if ( platform.find( encrypt( "AND" ) ) != std::string::npos )
		return ImColor( 50 , 205 , 50 );         // Android green

	return ImColor( 128 , 128 , 128 );           // Unknown = gray
}


std::unordered_map<int32_t , ImColor> squadColorCache;

ImColor  fortnite::engine::helpers::get_squad_color( int32_t squad_id )
{
	auto it = squadColorCache.find( squad_id );
	if ( it != squadColorCache.end( ) )
		return it->second;
	static std::mt19937 rng( std::random_device {}( ) );
	std::uniform_real_distribution<float> dist( 0.0f , 1.0f );
	ImColor color( dist( rng ) , dist( rng ) , dist( rng ) , 1.0f );

	squadColorCache [ squad_id ] = color;
	return color;
}

ImColor fortnite::engine::helpers::get_team_color( int teamId )
{
	static std::unordered_map<int , ImColor> teamColors;

	auto it = teamColors.find( teamId );
	if ( it != teamColors.end( ) )
		return it->second;
	std::mt19937 rng( teamId );

	std::uniform_real_distribution<float> hueDist( 0.0f , 1.0f );
	float h = hueDist( rng );
	float s = 0.9f; 
	float v = 1.0f; 

	float r , g , b;

	int i = int( h * 6.0f );
	float f = h * 6.0f - i;
	float p = v * ( 1.0f - s );
	float q = v * ( 1.0f - f * s );
	float t = v * ( 1.0f - ( 1.0f - f ) * s );

	switch ( i % 6 )
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}

	ImColor newColor(
		int( r * 255 ) ,
		int( g * 255 ) ,
		int( b * 255 )
	);

	teamColors [ teamId ] = newColor;
	return newColor;
}

ImColor  fortnite::engine::helpers::get_weapon_color( int weapon )
{

	if ( weapon == 0 || weapon == 7 )
		return ImColor( 255 , 255 , 255 ); // NOne common
	else if ( weapon == 1 )
		return ImColor( 0 , 221 , 26 ); // Uncommon
	else if ( weapon == 2 )
		return ImColor( 0 , 112 , 221 ); // Rare
	else if ( weapon == 3 )
		return ImColor( 163 , 53 , 238 ); // Epic
	else if ( weapon == 4 )
		return ImColor( 255 , 128 , 0 ); // Legendary
	else if ( weapon == 5 )
		return ImColor( 255 , 255 , 0 ); // Mythic
	else
		return ImColor( 255 , 255 , 255 ); // none
}

ImColor  fortnite::engine::helpers::get_distance_color( int distance )
{
	if ( distance <= 20 )
	{
		return ImColor( 255 , 0 , 0 );
	}
	else if ( distance >= 20 && distance <= 40 )
	{
		return ImColor( 252 , 100 , 0 );

	}
	else if ( distance >= 40 && distance <= 60 )
	{
		return ImColor( 255 , 221 , 0 );

	}
	else if ( distance >= 60 )
	{
		return ImColor( 0 , 255 , 0 );

	}
}

ImColor  fortnite::engine::helpers::get_rank_color( std::string rank )
{
	ImVec4 color = ImVec4( 1.0f , 1.0f , 1.0f , 1.0f );

	if ( rank.find( "Bronze" ) != std::string::npos )       color = ImVec4( 0.8f , 0.6f , 0.3f , 1.0f );
	else if ( rank.find( "Silver" ) != std::string::npos )  color = ImVec4( 0.85f , 0.85f , 0.85f , 1.0f );
	else if ( rank.find( "Gold" ) != std::string::npos )    color = ImVec4( 1.0f , 0.9f , 0.3f , 1.0f );
	else if ( rank.find( "Platinum" ) != std::string::npos ) color = ImVec4( 0.5f , 1.0f , 0.5f , 1.0f );
	else if ( rank.find( "Diamond" ) != std::string::npos )  color = ImVec4( 0.3f , 0.6f , 1.0f , 1.0f );
	else if ( rank == "Elite" )      color = ImVec4( 0.7f , 0.7f , 0.7f , 1.0f );
	else if ( rank == "Champion" )   color = ImVec4( 1.0f , 0.6f , 0.2f , 1.0f );
	else if ( rank == "Unreal" )     color = ImVec4( 0.5176f , 0.0f , 0.6902f , 1.0f );

	return ImColor( color );
}
struct FInstancedStruct
{
	uintptr_t ScriptStruct;
	uintptr_t StructMemory;
}; 

EFortRarity fortnite::engine::helpers::get_rarity( uint64_t weapon_data )
{
	if ( !weapon_data )
		return EFortRarity::EFortRarity_Common;

	auto dataList = communcations::read<ueegnine::tarray<FInstancedStruct>>( weapon_data + 0x68 );

	for ( int i = 0; i < dataList.get_count( ); i++ )
	{
		auto data = dataList.get( i );
		if ( data.ScriptStruct == communcations::base_address + 0x1725C508 )
		{
			return communcations::read<EFortRarity>( data.StructMemory );
		}
	}
	return EFortRarity::EFortRarity_Uncommon;
}

bool  fortnite::engine::helpers::is_visible( uint64_t mesh ) {
	static std::unordered_map< uintptr_t , float > last_render_times;
	static std::unordered_map< uintptr_t , int > no_update_frames;
	static std::unordered_map< uintptr_t , bool > cached_visibility;
	static std::unordered_map< uintptr_t , float > last_update_time;

	auto current_render_time = communcations::read < float >( mesh + 0x330 );

	if ( current_render_time == -1000.0f ) {
		no_update_frames [ mesh ] = 0;
		cached_visibility [ mesh ] = false;
		return false;
	}

	auto it = last_render_times.find( mesh );
	float current_time = ( float ) GetTickCount64( ) / 1000.0f;

	if ( it != last_render_times.end( ) ) {
		float time_diff = current_render_time - it->second;
		float time_since_last_update = current_time - last_update_time [ mesh ];

		if ( time_diff > 0.001f ) {
			no_update_frames [ mesh ] = 0;
			last_render_times [ mesh ] = current_render_time;
			last_update_time [ mesh ] = current_time;
			cached_visibility [ mesh ] = true;
			return true;
		}
		else {
			if ( time_since_last_update > 0.1f ) {
				no_update_frames [ mesh ]++;
				if ( no_update_frames [ mesh ] > 2 ) {
					cached_visibility [ mesh ] = false;
					return false;
				}
			}

			return cached_visibility [ mesh ];
		}
	}
	else {
		last_render_times [ mesh ] = current_render_time;
		last_update_time [ mesh ] = current_time;
		no_update_frames [ mesh ] = 0;
		cached_visibility [ mesh ] = true;
		return true;
	}
}

double fortnite::engine::helpers::to_reg( double deg )
{
	return deg * ( std::numbers::pi / 180.0 );

}

