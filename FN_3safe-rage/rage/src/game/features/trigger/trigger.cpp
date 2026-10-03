// safe-rage: trigger routed through makcu, no keybd_event / SendInput.
#include "trigger.hpp"
#include "../../../../dependenices/mouse/mouse.hpp"
#include <map>
#include <unordered_set>

void fortnite::trigger::tick( )
{
    if ( !fortnite::settings::trigger::enabled )
        return;

    if ( !( GetAsyncKeyState( fortnite::settings::trigger::hotkey ) & 0x8000 ) )
        return;

    auto world_data = fortnite::world::get( );
    if ( !world_data )
        return;

    uint64_t local_player = world_data->local_players.get( 0 );
    auto player_controller = communcations::read<uint64_t>( local_player + fortnite::offsets::player_controller );
    if ( !player_controller )
        return;


    auto current_weapon = communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::current_weapon );
    bool is_shotgun = false;
    if ( current_weapon ) {
        auto weapon_type = communcations::read<EFortWeaponCoreAnimation>( current_weapon + fortnite::offsets::weapon_core_animation );
        if ( weapon_type == EFortWeaponCoreAnimation::Shotgun ) {
            is_shotgun = true;
        }
    }

    if ( fortnite::settings::trigger::shotgun_only && !is_shotgun )
        return;

    auto targetted_pawn = communcations::read<uint64_t>( player_controller + fortnite::offsets::targeted_fort_pawn );
    if ( !targetted_pawn )
        return;
    static auto last_fire = std::chrono::steady_clock::now( );
    auto now = std::chrono::steady_clock::now( );

    if ( std::chrono::duration_cast< std::chrono::milliseconds >( now - last_fire ).count( ) < fortnite::settings::trigger::trigger_delay )
        return;
    if ( fortnite::settings::trigger::randomness_factor > 0 ) {
        int random_chance = rand( ) % 100;
        if ( random_chance < fortnite::settings::trigger::randomness_factor )
            return;
    }

    last_fire = now;
    // safe-rage: hardware left click via makcu, not keybd_event.
    fortnite::mouse::click( 3 );
}
float fortnite::trigger::calculate_fov_distance( uemath::fvector2d screen_pos )
{
    float center_x = static_cast< float >( fortnite::render::width ) / 2.0f;
    float center_y = static_cast< float >( fortnite::render::height ) / 2.0f;
    float dx = static_cast< float >( screen_pos.x ) - center_x;
    float dy = static_cast< float >( screen_pos.y ) - center_y;
    return sqrtf( dx * dx + dy * dy );
}
