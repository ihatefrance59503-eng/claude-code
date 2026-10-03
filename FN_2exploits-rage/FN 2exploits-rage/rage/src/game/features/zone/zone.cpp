// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "zone.hpp"
#include <cmath>


namespace fortnite::zone
{
    zone_data_t current_zone;
    bool zone_valid = false;

    void update_zone( )
    {
        auto world_data = fortnite::world::get( );
        if ( !world_data )
            return;

        uint64_t game_state = communcations::read<uint64_t>( world_data->gworld + offsets::game_state );
        if ( !game_state )
            return;

        auto map_info = communcations::read<uint64_t>( game_state + 0x2b88 );
        if ( !map_info )
        {
            zone_valid = false;
            return;
        }

        uint64_t safe_zone_definition = communcations::read<uint64_t>( map_info + 0x7d0 );
        if ( !safe_zone_definition )
            return;

        float radius = communcations::read<float>( safe_zone_definition + 0x28 );
        float min_dst = communcations::read<float>( safe_zone_definition + 0x50 );
        float max_dst = communcations::read<float>( safe_zone_definition + 0x78 );
        float reject = communcations::read<float>( safe_zone_definition + 0xa0 );
        float reject_o = communcations::read<float>( safe_zone_definition + 0xc8 );
        float wait = communcations::read<float>( safe_zone_definition + 0xf0 );
        float shrink = communcations::read<float>( safe_zone_definition + 0x118 );
        float grid = communcations::read<float>( safe_zone_definition + 0x140 );
        float solo = communcations::read<float>( safe_zone_definition + 0x168 );
        float duo = communcations::read<float>( safe_zone_definition + 0x190 );
        float squad = communcations::read<float>( safe_zone_definition + 0x1b8 );

        current_zone.radius = radius;
        current_zone.min_distance = min_dst;
        current_zone.max_distance = max_dst;
        current_zone.reject = reject;
        current_zone.reject_offset = reject_o;
        current_zone.wait_time = wait;
        current_zone.shrink_time = shrink;
        current_zone.grid_size = grid;
        current_zone.solo_radius = solo;
        current_zone.duo_radius = duo;
        current_zone.squad_radius = squad;

        zone_valid = true;
    }

    void draw_zone_esp( )
    {
        update_zone( );

        if ( !zone_valid || current_zone.radius <= 0.f )
            return;

        auto draw_list = ImGui::GetBackgroundDrawList( );
        auto local_pawn = fortnite::entity::local_pawn;

        if ( !local_pawn )
            return;

        uint64_t local_root = communcations::read<uint64_t>( local_pawn + offsets::root_component );
        fortnite::uemath::fvector local_pos = communcations::read< fortnite::uemath::fvector>( local_pawn + offsets::relative_location );
        ImVec2 screen_pos;
        fortnite::uemath::fvector2d idk = fortnite::engine::camera::world_to_screen( local_pos );
        if ( idk.x  )
        {
            // Draw zone radius as circle centered on player position
            ImU32 zone_color = ImGui::GetColorU32( ImVec4( 0.2f , 0.8f , 1.f , 0.8f ) );
            float radius_pixels = current_zone.radius / 100.f;
            draw_list->AddCircle( screen_pos , radius_pixels , zone_color , 64 , 2.f );

            // Draw next zone circle if different
            if ( current_zone.shrink_time > 0.f && current_zone.radius > current_zone.solo_radius )
            {
                ImU32 next_zone_color = ImGui::GetColorU32( ImVec4( 1.f , 0.2f , 0.2f , 0.6f ) );
                float next_radius_pixels = current_zone.solo_radius / 100.f;
                draw_list->AddCircle( screen_pos , next_radius_pixels , next_zone_color , 64 , 1.5f );
            }

            // Display zone info
            char zone_buf [ 128 ];
            sprintf_s( zone_buf , "Radius: %.0fm | Shrink: %.1fs | Next: %.0fm" ,
                current_zone.radius ,
                current_zone.shrink_time ,
                current_zone.solo_radius );

            ImVec2 text_pos = ImVec2( screen_pos.x - 80.f , screen_pos.y - 30.f );
            draw_list->AddText( text_pos , zone_color , zone_buf );
        }
    }
}