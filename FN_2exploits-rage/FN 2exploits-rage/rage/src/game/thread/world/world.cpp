// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "world.hpp"

void fortnite::world::start( )
{
    cache.start(
        [ ]( data& out )
        {
            out.gworld = communcations::read<uint64_t>( communcations::base_address + offsets::gworld );
            out.gworld = fortnite::offsets::decrypt_uworld( out.gworld );

            if ( !out.gworld )
                return;
            out.game_instance = communcations::read<uint64_t>( out.gworld + offsets::game_instance );
            if ( !out.game_instance )
                return;

            out.local_players = communcations::read<ueegnine::tarray<uint64_t>>( out.game_instance + offsets::local_players );
            out.player_controller = communcations::read<uint64_t>( out.local_players.get( 0 ) + 0x30 );
            out.levels = communcations::read<ueegnine::tarray<uint64_t>>( out.gworld + offsets::levels );

        } ,
        20000
    );

}


void fortnite::world::start_c( )
{
    std::thread( [ ]( )
        {
            while ( true )
            {
                fortnite::engine::camera::update_camera( );
                std::this_thread::sleep_for( std::chrono::milliseconds( 2500 ) );
            }
        } ).detach( );
    std::thread( [ ]( )
        {
            while ( true )
            {
                fortnite::engine::camera::setup_camera( );
                fortnite::engine::camera::update_camera( );
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
        } ).detach( );
}

void fortnite::world::stop( )
{
    cache.stop( );
}

std::shared_ptr<const fortnite::world::data> fortnite::world::get( )
{
    return cache.get( );
}
