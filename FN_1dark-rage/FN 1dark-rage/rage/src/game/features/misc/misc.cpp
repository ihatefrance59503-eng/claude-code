// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "misc.hpp"


inline bool is_valid( uintptr_t address )
{
    if ( address == 0 )
        return false;

    if ( address < 0x10000 || address > 0x7FFFFFFFFFFF ) 
        return false;
    if ( address % sizeof( uintptr_t ) != 0 )
        return false;

    return true;
}
void fortnite::misc::tick( )
{
	if ( !running )
		return;
	ImDrawList* draw_list = ImGui::GetForegroundDrawList( );
	ImFont* font = ImGui::GetFont( );
	const float font_size = ImGui::GetFontSize( );
    auto current_weapon = communcations::read<uint64_t>(fortnite::entity::local_pawn + offsets::current_weapon );
    static float hue = 0.0f;
    static float color_transition_speed = 0.5f; 
    auto world_data1 = fortnite::world::get( );

    if ( current_weapon )
    {
        hue += color_transition_speed;
        if ( hue > 360.0f )
            hue -= 360.0f;
        float h = hue / 60.0f;
        float c = 1.0f;  
        float x = c * ( 1.0f - fabs( fmod( h , 2.0f ) - 1.0f ) );

        float r = 0.0f , g = 0.0f , b = 0.0f;

        if ( h < 1.0f ) { r = c; g = x; b = 0.0f; }
        else if ( h < 2.0f ) { r = x; g = c; b = 0.0f; }
        else if ( h < 3.0f ) { r = 0.0f; g = c; b = x; }
        else if ( h < 4.0f ) { r = 0.0f; g = x; b = c; }
        else if ( h < 5.0f ) { r = x; g = 0.0f; b = c; }
        else { r = c; g = 0.0f; b = x; }
        uint8_t color_r = ( uint8_t ) ( r * 255.0f );
        uint8_t color_g = ( uint8_t ) ( g * 255.0f );
        uint8_t color_b = ( uint8_t ) ( b * 255.0f );
        uint8_t color_a = 255;

        uint32_t smooth_color = ( color_a << 24 ) | ( color_b << 16 ) | ( color_g << 8 ) | color_r;
        communcations::write<uint32_t>( current_weapon + 0x928 , smooth_color );
    }

    auto world_data = fortnite::world::get( );
    if ( !world_data )
        return;
    uint64_t local_player = world_data->local_players.get( 0 );
    auto player_controller = communcations::read<uint64_t>( local_player + fortnite::offsets::player_controller );
    if ( !player_controller )
        return;

    auto interaction_component = communcations::read<uint64_t>( player_controller + 0x2b10 );

    if ( interaction_component )
    {
        communcations::write<float>( interaction_component + 0x300 , 0.0f );
        communcations::write<float>( interaction_component + 0x304 , 0.0f );
        communcations::write<uint8_t>( interaction_component + 0x32d , 0 );  
        communcations::write<uint8_t>( interaction_component + 0x32e , 0 );  
        communcations::write<uint8_t>( interaction_component + 0x330 , 0 );  
        communcations::write<uint8_t>( interaction_component + 0x331 , 0 );  
    }

	auto actors = fortnite::entity::get_a( );
	if ( !actors )
		return;
    for ( const auto& actor : *actors ) {
        if ( fortnite::settings::exploits::player_size ) {
            auto UCapsuleComponent = communcations::read<uint64_t>( actor.current + 0x340 );
            communcations::write<uemath::fvector>( UCapsuleComponent + 0x170 , uemath::fvector( fortnite::settings::exploits::player_size_value , fortnite::settings::exploits::player_size_value , fortnite::settings::exploits::player_size_value ) );
        }

    }





}

void fortnite::misc::stop( )
{
	running = false;
}










/*EXPLOIT TO FAST BREAK WALLS BUT IN RETURN YOUR BULLETS DO NOTHING LMAO
auto weapon_data = communcations::read<uint64_t>(
    current_weapon + offsets::weapon_data );

auto weapon_stats = communcations::read<uint64_t>(
    weapon_data + 0x130 );

const auto row_name = communcations::read<uint32_t>(
    weapon_data + 0x138 );

auto scan_row_map = [ & ]( std::uintptr_t row_map_base ) -> std::uintptr_t
    {
        if ( !row_map_base || !is_valid( row_map_base ) )
            return 0;

        const auto row_map_data = communcations::read<std::uintptr_t>( row_map_base + 0x0 );

        const auto row_map_count = communcations::read<int32_t>( row_map_base + 0x8 );

        if ( !row_map_data || row_map_count <= 0 || row_map_count > 4096 )
            return 0;

        for ( int i = 0; i < row_map_count; i++ )
        {
            const auto pair_base =
                row_map_data + ( static_cast< std::uintptr_t >( i ) * 0x18 );

            const auto key = communcations::read<uint32_t>( pair_base + 0x0 );

            if ( key != row_name )
                continue;

            const auto row_ptr = communcations::read<std::uintptr_t>( pair_base + 0x8 );

            if ( !row_ptr || !is_valid( row_ptr ) )
                return 0;

            return row_ptr;
        }

        return 0;
    };

std::uintptr_t idk = 0;

if ( auto row_ptr = scan_row_map( weapon_stats + 0x30 ); row_ptr )
idk = row_ptr;

if ( !idk )
idk = scan_row_map( weapon_stats + 0x58 );

if ( idk && is_valid( idk ) )
{
    auto check_holding_pickaxe = [ ]( ) -> bool
        {
            auto current_weapon = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::current_weapon );
            auto weapon_data = fortnite::communcations::read<uint64_t>( current_weapon + fortnite::offsets::weapon_data );
            auto name = fortnite::communcations::read<fortnite::ueegnine::ftext>( weapon_data + 0x38 );
            return name.get( ).find( "Pickaxe" ) != std::string::npos;
        };

    bool already_holding_pickaxe = check_holding_pickaxe( );

    static float ORIGINAL_PICKAXE_DAMAGE = -1.0f;
    if ( ORIGINAL_PICKAXE_DAMAGE < 0.0f && idk && is_valid( idk ) )
    {
        ORIGINAL_PICKAXE_DAMAGE = communcations::read<float>( idk + 0x244 );
    }

    if ( idk && is_valid( idk ) )
    {
        bool already_holding_pickaxe = check_holding_pickaxe( );

        if ( already_holding_pickaxe )
        {
            communcations::write<float>( idk + 0x244 , 1.25f );
        }
        else
        {
            communcations::write<float>( idk + 0x244 , ORIGINAL_PICKAXE_DAMAGE );
        }
    }
}*/