// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "drawing.hpp"
#include "../../dependenices/mouse/mouse.hpp"
#include "../game/sdk/sdk.hpp"
#include "../game/thread/entity/entity.hpp"
#include "../game/features/health/health.hpp"
#include "../settings/settings.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    // Caching for expensive operations
    struct drawing_cache {
        uint64_t last_actor = 0;
        fortnite::uemath::box3d last_bounds;
        uint64_t last_mesh = 0;
        bool bounds_valid = false;

        void clear( ) {
            last_actor = 0;
            last_mesh = 0;
            bounds_valid = false;
        }
    } cache;

    static void draw_3d_box( ImDrawList* draw_list , const ImVec2* c , ImU32 color , float thickness )
    {
        draw_list->AddLine( c [ 0 ] , c [ 1 ] , color , thickness );
        draw_list->AddLine( c [ 2 ] , c [ 3 ] , color , thickness );
        draw_list->AddLine( c [ 4 ] , c [ 5 ] , color , thickness );
        draw_list->AddLine( c [ 6 ] , c [ 7 ] , color , thickness );

        draw_list->AddLine( c [ 0 ] , c [ 2 ] , color , thickness );
        draw_list->AddLine( c [ 1 ] , c [ 3 ] , color , thickness );
        draw_list->AddLine( c [ 4 ] , c [ 6 ] , color , thickness );
        draw_list->AddLine( c [ 5 ] , c [ 7 ] , color , thickness );

        draw_list->AddLine( c [ 0 ] , c [ 4 ] , color , thickness );
        draw_list->AddLine( c [ 1 ] , c [ 5 ] , color , thickness );
        draw_list->AddLine( c [ 2 ] , c [ 6 ] , color , thickness );
        draw_list->AddLine( c [ 3 ] , c [ 7 ] , color , thickness );
    }

    static void draw_3d_corner_box( ImDrawList* draw_list , const ImVec2* c , ImU32 color , float thickness )
    {
        auto draw_line_part = [ & ]( const ImVec2& a , const ImVec2& b ) {
            ImVec2 delta = ImVec2( b.x - a.x , b.y - a.y );
            draw_list->AddLine( a , ImVec2( a.x + delta.x * 0.25f , a.y + delta.y * 0.25f ) , color , thickness );
            draw_list->AddLine( b , ImVec2( b.x - delta.x * 0.25f , b.y - delta.y * 0.25f ) , color , thickness );
            };

        draw_line_part( c [ 0 ] , c [ 1 ] );
        draw_line_part( c [ 2 ] , c [ 3 ] );
        draw_line_part( c [ 4 ] , c [ 5 ] );
        draw_line_part( c [ 6 ] , c [ 7 ] );

        draw_line_part( c [ 0 ] , c [ 2 ] );
        draw_line_part( c [ 1 ] , c [ 3 ] );
        draw_line_part( c [ 4 ] , c [ 6 ] );
        draw_line_part( c [ 5 ] , c [ 7 ] );

        draw_line_part( c [ 0 ] , c [ 4 ] );
        draw_line_part( c [ 1 ] , c [ 5 ] );
        draw_line_part( c [ 2 ] , c [ 6 ] );
        draw_line_part( c [ 3 ] , c [ 7 ] );
    }

    struct tracer_t {
        fortnite::uemath::fvector position;
        std::chrono::steady_clock::time_point timestamp;
    };

    inline std::unordered_map<uint64_t , std::vector<tracer_t>> player_traces;

    inline void add_player_trace( uint64_t actor_id , const fortnite::uemath::fvector& position ) {
        auto now = std::chrono::steady_clock::now( );
        player_traces [ actor_id ].emplace_back( tracer_t { position, now } );
    }

    inline void cleanup_traces( ) {
        auto now = std::chrono::steady_clock::now( );

        for ( auto it = player_traces.begin( ); it != player_traces.end( ); ++it ) {
            auto& traces = it->second;
            traces.erase(
                std::remove_if( traces.begin( ) , traces.end( ) ,
                    [ & ]( const tracer_t& point ) {
                        auto age = std::chrono::duration_cast< std::chrono::duration<float> >(
                            now - point.timestamp ).count( );
                        return age > fortnite::settings::players::movement_fade;
                    }
                ) ,
                traces.end( )
            );
        }

        for ( auto it = player_traces.begin( ); it != player_traces.end( ); ) {
            if ( it->second.empty( ) ) {
                it = player_traces.erase( it );
            }
            else {
                ++it;
            }
        }
    }

    struct c_bullet_tracer {
        fortnite::uemath::fvector m_start_location;
        fortnite::uemath::fvector m_end_location;
        float m_impact_angle;
        float m_lifetime;
        float m_alpha;
        float m_creation_time;
        std::vector<fortnite::uemath::fvector> m_trail_points;
        ImColor m_color;
        float m_thickness;
        c_bullet_tracer( ) :
            m_impact_angle( 0.f ) ,
            m_lifetime( 4.0f ) ,
            m_alpha( 1.0f ) ,
            m_creation_time( ImGui::GetTime( ) ) ,
            m_color( ImColor( 255 , 255 , 255 ) ) ,
            m_thickness( 1.0f )
        {}
        bool is_expired( ) const {
            return ( ImGui::GetTime( ) - m_creation_time ) > m_lifetime;
        }
        float get_age( ) const {
            return ( ImGui::GetTime( ) - m_creation_time ) / m_lifetime;
        }
    };

    std::vector<c_bullet_tracer> m_tracer_list;

    void update_and_draw_tracers( ) {
        float current_time = ImGui::GetTime( );

        for ( auto it = m_tracer_list.begin( ); it != m_tracer_list.end( ); ) {
            c_bullet_tracer& tracer = *it;

            if ( tracer.is_expired( ) ) {
                it = m_tracer_list.erase( it );
                continue;
            }

            fortnite::uemath::fvector2d start_screen = fortnite::engine::camera::world_to_screen( tracer.m_start_location );
            fortnite::uemath::fvector2d end_screen = fortnite::engine::camera::world_to_screen( tracer.m_end_location );

            if ( start_screen.x <= 0 || start_screen.y <= 0 || end_screen.x <= 0 || end_screen.y <= 0 ) {
                ++it;
                continue;
            }

            float age = tracer.get_age( );
            float life_fraction = std::clamp( age / fortnite::settings::tracers::fade , 0.0f , 1.0f );
            float alpha = tracer.m_alpha * ( 1.0f - life_fraction );

            ImColor base_color;

            base_color = ImColor( ImVec4( fortnite::settings::tracers::color [ 0 ] , fortnite::settings::tracers::color [ 1 ] , fortnite::settings::tracers::color [ 2 ] , fortnite::settings::tracers::color [ 3 ] * alpha ) );

            ImDrawList* draw = ImGui::GetBackgroundDrawList( );

            draw->AddLine( ImVec2( start_screen.x , start_screen.y ) , ImVec2( end_screen.x , end_screen.y ) , base_color , 2 );
            if ( !tracer.m_trail_points.empty( ) ) {
                fortnite::uemath::fvector2d prev_point = start_screen;
                for ( const auto& trail_point : tracer.m_trail_points ) {
                    fortnite::uemath::fvector2d trail_screen = fortnite::engine::camera::world_to_screen( trail_point );
                    if ( trail_screen.x > 0 && trail_screen.y > 0 ) {
                        draw->AddLine( ImVec2( prev_point.x , prev_point.y ) , ImVec2( trail_screen.x , trail_screen.y ) , base_color , fortnite::settings::tracers::thickness * 0.5f );
                        prev_point = trail_screen;
                    }
                }
            }

            float sphere_radius = fortnite::settings::tracers::thickness * 3.5f;

            ImColor sphere_color = ImColor( ImVec4( base_color.Value.x , base_color.Value.y , base_color.Value.z , base_color.Value.w * 0.75f ) );
            draw->AddCircleFilled( ImVec2( end_screen.x , end_screen.y ) , sphere_radius , sphere_color , 20 );

            ++it;
        }
    }

    bool is_duplicate_tracer( const fortnite::uemath::fvector& start , const fortnite::uemath::fvector& end , double threshold = 0.30 ) {
        for ( auto& tracer : m_tracer_list ) {
            if ( tracer.m_start_location.distance( start ) < threshold &&
                tracer.m_end_location.distance( end ) < threshold ) {
                return true;
            }
        }
        return false;
    }

    void add_bullet_tracer( const fortnite::uemath::fvector& start , const fortnite::uemath::fvector& end , float impact_angle ) {
        if ( m_tracer_list.size( ) >= 10 ) {
            m_tracer_list.erase( m_tracer_list.begin( ) );
        }
        c_bullet_tracer new_tracer;
        new_tracer.m_start_location = start;
        new_tracer.m_end_location = end;
        new_tracer.m_impact_angle = impact_angle;
        new_tracer.m_trail_points.push_back( end );
        new_tracer.m_thickness = fortnite::settings::tracers::thickness;
        new_tracer.m_color = ImColor( ImVec4( fortnite::settings::tracers::color [ 0 ] , fortnite::settings::tracers::color [ 1 ] , fortnite::settings::tracers::color [ 2 ] , fortnite::settings::tracers::color [ 3 ] ) );

        m_tracer_list.emplace_back( std::move( new_tracer ) );
    }

    static void draw_text_outlined( ImDrawList* draw_list , ImFont* font , float font_size , const ImVec2& pos , ImU32 text_color , const char* text )
    {
        const ImU32 outline = IM_COL32( 0 , 0 , 0 , 255 );

        ImVec2 text_size = font->CalcTextSizeA( font_size , FLT_MAX , 0.0f , text );
        ImVec2 centered_pos = ImVec2( pos.x - text_size.x / 2.0f , pos.y - text_size.y / 2.0f );

        int style = fortnite::settings::players::text_type;
        if ( style == 1 ) {
            for ( int ox = -1; ox <= 1; ++ox )
            {
                for ( int oy = -1; oy <= 1; ++oy )
                {
                    if ( ox == 0 && oy == 0 )
                        continue;
                    draw_list->AddText( font , font_size , ImVec2( centered_pos.x + ( float ) ox , centered_pos.y + ( float ) oy ) , outline , text );
                }
            }
        }
        else if ( style == 2 ) {
            draw_list->AddText( font , font_size , ImVec2( centered_pos.x + 1.0f , centered_pos.y + 1.0f ) , outline , text );
        }

        draw_list->AddText( font , font_size , centered_pos , text_color , text );
    }

    static bool on_screen( const fortnite::uemath::fvector2d& p , float margin )
    {
        return p.x >= -margin && p.y >= -margin
            && p.x <= fortnite::render::width + margin
            && p.y <= fortnite::render::height + margin;
    }

    constexpr float deg_to_rad( float deg )
    {
        return deg * ( 3.14159265358979323846f / 180.0f );
    }

    void rice_hat( const fortnite::uemath::fvector& head , float radius , float height , int segments , float tick , ImColor color ) {
        fortnite::uemath::fvector tip = { head.x, head.y, head.z + height };

        std::vector<ImVec2> basePoints2D;

        for ( int i = 0; i < segments; ++i ) {
            float angle = ( 2 * std::numbers::pi / segments ) * i;
            float x = cosf( angle ) * radius;
            float y = sinf( angle ) * radius;

            fortnite::uemath::fvector basePoint3D = { head.x + x, head.y + y, head.z + 5 };
            fortnite::uemath::fvector2d screenBase = fortnite::engine::camera::world_to_screen( basePoint3D );
            basePoints2D.push_back( ImVec2( screenBase.x , screenBase.y ) );
        }

        fortnite::uemath::fvector2d screenTip = fortnite::engine::camera::world_to_screen( tip );
        ImVec2 tip2D = ImVec2( screenTip.x , screenTip.y );

        for ( int i = 0; i < segments; ++i ) {
            ImGui::GetBackgroundDrawList( )->AddLine( tip2D , basePoints2D [ i ] , color , tick );
            ImGui::GetBackgroundDrawList( )->AddLine( basePoints2D [ i ] , basePoints2D [ ( i + 1 ) % segments ] , color , tick );
        }
    }

    EFortWeaponCoreAnimation categorize_weapon_by_name( const std::string& weaponName )
    {
        if ( weaponName.find( "Lightsaber" ) != std::string::npos || weaponName.find( "Hammer" ) != std::string::npos || weaponName.find( "Scythe" ) != std::string::npos || weaponName.find( "Blade" ) != std::string::npos || weaponName.find( "Chainsaw" ) != std::string::npos || weaponName.find( "Sword" ) != std::string::npos || weaponName.find( "Axe" ) != std::string::npos || weaponName.find( "Bat" ) != std::string::npos || weaponName.find( "Kneecapper" ) != std::string::npos || weaponName.find( "Lucille" ) != std::string::npos || weaponName.find( "Chains of Hades" ) != std::string::npos || weaponName.find( "Pickaxe" ) != std::string::npos )
        {
            return EFortWeaponCoreAnimation::Melee;
        }
        return EFortWeaponCoreAnimation::Unarmed;
    }

    static void draw_bar( ImDrawList* draw_list , const ImVec2& a , const ImVec2& b , float frac , ImU32 fill , ImU32 bg )
    {
        frac = ( std::max ) ( 0.0f , ( std::min ) ( frac , 1.0f ) );
        draw_list->AddRectFilled( a , b , bg , 2.0f );
        const float w = b.x - a.x;
        draw_list->AddRectFilled( a , ImVec2( a.x + w * frac , b.y ) , fill , 2.0f );
        draw_list->AddRect( a , b , IM_COL32( 0 , 0 , 0 , 255 ) , 2.0f );
    }

    inline int LastAmmoCount = -1;
    inline bool bInitialized = false;
    bool is_weapon_firing( uint64_t current_weapon ) {
        int CurrentAmmo = fortnite::communcations::read<int32_t>( current_weapon + 0x116c );
        if ( !bInitialized ) {
            LastAmmoCount = CurrentAmmo;
            bInitialized = true;
            return false;
        }

        bool bFired = ( CurrentAmmo < LastAmmoCount );
        LastAmmoCount = CurrentAmmo;

        return bFired;
    }
}

void fortnite::drawing::player_esp( ImDrawList* draw_list )
{
    if ( !draw_list )
        return;

    static bool was_pressed = false;
    bool pressed = ( GetAsyncKeyState( fortnite::settings::world::battlemode ) & 0x8000 );
    if ( pressed && !was_pressed )
    {
        fortnite::settings::world::battlemode_toggle = !fortnite::settings::world::battlemode_toggle;
    }
    was_pressed = pressed;

    if ( !fortnite::settings::players::enabled )
        return;

    ImFont* font = ImGui::GetFont( );
    const float font_size = ImGui::GetFontSize( );

    auto actors = fortnite::entity::get_a( );
    if ( !actors || actors->empty( ) )
        return;

    fortnite::render::rendered_players = 0;
    fortnite::render::visible_players = 0;
    fortnite::render::players_within_20m = 0;

    auto to_lower = [ ]( std::string s ) {
        std::transform( s.begin( ) , s.end( ) , s.begin( ) , [ ]( unsigned char c ) { return std::tolower( c ); } );
        return s;
        };

    // Draw local player self rice hat
    if ( fortnite::entity::local_pawn && fortnite::settings::players::self_rice_hat ) {
        auto mesh = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::mesh );
        if ( mesh ) {
            auto head_world_self = fortnite::engine::bone::get_bone_location( mesh , 110 );
            rice_hat( head_world_self , 25.0f , 20.0f , 20 , 1.0f , ImColor( ImVec4( fortnite::settings::players::rice_hat_color [ 0 ] , fortnite::settings::players::rice_hat_color [ 1 ] , fortnite::settings::players::rice_hat_color [ 2 ] , fortnite::settings::players::rice_hat_color [ 3 ] ) ) );
        }
    }

    // Draw local player movement tracers
    if ( fortnite::settings::players::self_movement_tracers && fortnite::entity::local_pawn ) {
        auto mesh = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::mesh );
        if ( mesh ) {
            auto root_world_self = fortnite::engine::bone::get_bone_location( mesh , 0 );
            add_player_trace( fortnite::entity::local_pawn , root_world_self );
        }
    }

    cleanup_traces( );

    const float cx = fortnite::render::width / 2.0f;
    const float cy = fortnite::render::height / 2.0f;

    // Draw movement tracers for all players
    if ( fortnite::settings::players::movement_tracers || fortnite::settings::players::self_movement_tracers ) {
        ImU32 color = ImGui::GetColorU32( ImVec4( fortnite::settings::players::movement_color [ 0 ] , fortnite::settings::players::movement_color [ 1 ] , fortnite::settings::players::movement_color [ 2 ] , fortnite::settings::players::movement_color [ 3 ] ) );
        for ( auto& [actor_id , traces] : player_traces ) {
            for ( size_t i = 1; i < traces.size( ); ++i ) {
                fortnite::uemath::fvector2d prev_screen = fortnite::engine::camera::world_to_screen( traces [ i - 1 ].position );
                fortnite::uemath::fvector2d curr_screen = fortnite::engine::camera::world_to_screen( traces [ i ].position );
                draw_list->AddLine( ImVec2 { ( float ) prev_screen.x, ( float ) prev_screen.y } , ImVec2 { ( float ) curr_screen.x, ( float ) curr_screen.y } , color , fortnite::settings::players::movement_thickness );
            }
        }
    }

    // Main player ESP loop - GUARANTEED to process valid actors
    for ( const auto& actor : *actors )
    {
        // Strict validity checks - skip immediately if invalid
        if ( !actor.current || !actor.mesh )
            continue;

        // Check if dying
      /*  const uint8_t b_is_dying_flag = communcations::read<uint8_t>( actor.current + fortnite::offsets::b_is_dying );
        const bool b_is_dying = b_is_dying_flag & ( 5 << 0 );
        if ( b_is_dying )
            continue;*/

        // Get bone locations safely
        auto head_world = fortnite::engine::bone::get_bone_location( actor.mesh , 110 );
        auto root_world = fortnite::engine::bone::get_bone_location( actor.mesh , 0 );
        auto head_screen = fortnite::engine::camera::world_to_screen( head_world );

        auto root_comp = fortnite::communcations::read<uintptr_t>( actor.current + offsets::root_component );
        if ( !root_comp )
            continue;

        fortnite::uemath::frotator rotation = fortnite::communcations::read< fortnite::uemath::frotator >( root_comp + offsets::relative_rotation );
        fortnite::uemath::fvector direction = rotation.get_forward_vector( );
        fortnite::uemath::fvector view_direction = head_world + ( direction * 180.0 );
        fortnite::uemath::fvector2d view_screen = fortnite::engine::camera::world_to_screen( view_direction );

        float dist_m = fortnite::engine::camera::location.distance( head_world ) / 100.0f;
        bool is_visible = fortnite::engine::helpers::is_visible( actor.mesh );

        fortnite::radar::add_to_radar( head_world , is_visible , dist_m , rotation , actor.team_id );

        const auto bounds = fortnite::engine::bone::get_bounds( actor.mesh );

        float min_x = 99999.0f;
        float max_x = -99999.0f;
        float min_y = 99999.0f;
        float max_y = -99999.0f;

        ImVec2 corners [ 8 ];
        for ( int i = 0; i < 8; ++i ) {
            fortnite::uemath::fvector corner;
            corner.x = bounds.origin.x + ( i & 1 ? bounds.box_extent.x : -bounds.box_extent.x );
            corner.y = bounds.origin.y + ( i & 2 ? bounds.box_extent.y : -bounds.box_extent.y );
            corner.z = bounds.origin.z + ( i & 4 ? bounds.box_extent.z : -bounds.box_extent.z );

            auto screen = fortnite::engine::camera::world_to_screen( corner );
            corners [ i ] = ImVec2( static_cast< float >( screen.x ) , static_cast< float >( screen.y ) );

            min_x = ( std::min ) ( min_x , static_cast< float >( screen.x ) );
            max_x = ( std::max ) ( max_x , static_cast< float >( screen.x ) );
            min_y = ( std::min ) ( min_y , static_cast< float >( screen.y ) );
            max_y = ( std::max ) ( max_y , static_cast< float >( screen.y ) );
        }

        fortnite::render::rendered_players++;
        if ( is_visible ) fortnite::render::visible_players++;
        if ( dist_m <= 20.0f ) fortnite::render::players_within_20m++;

        if ( fortnite::settings::players::visible_check && !is_visible )
            continue;

        ImU32 box_color = is_visible ? ImGui::ColorConvertFloat4ToU32( ImVec4( fortnite::settings::players::visible_color [ 0 ] , fortnite::settings::players::visible_color [ 1 ] , fortnite::settings::players::visible_color [ 2 ] , fortnite::settings::players::visible_color [ 3 ] ) ) : ImGui::ColorConvertFloat4ToU32( ImVec4( fortnite::settings::players::invisible_color [ 0 ] , fortnite::settings::players::invisible_color [ 1 ] , fortnite::settings::players::invisible_color [ 2 ] , fortnite::settings::players::invisible_color [ 3 ] ) );

        if ( fortnite::settings::players::box )
        {
            ImVec2 top_left = ImVec2( min_x , min_y );
            ImVec2 bottom_right = ImVec2( max_x , max_y );
            float box_w = max_x - min_x;
            float box_h = max_y - min_y;

            if ( fortnite::settings::players::box_type == 0 ) {
                draw_list->AddRect( top_left , bottom_right , box_color , 0.0f , 0 , fortnite::settings::players::box_thickness );
            }
            else if ( fortnite::settings::players::box_type == 1 ) {
                float length = box_w / 4.0f;
                draw_list->AddLine( top_left , ImVec2( top_left.x + length , top_left.y ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( top_left , ImVec2( top_left.x , top_left.y + length ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( ImVec2( bottom_right.x , top_left.y ) , ImVec2( bottom_right.x - length , top_left.y ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( ImVec2( bottom_right.x , top_left.y ) , ImVec2( bottom_right.x , top_left.y + length ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( ImVec2( top_left.x , bottom_right.y ) , ImVec2( top_left.x + length , bottom_right.y ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( ImVec2( top_left.x , bottom_right.y ) , ImVec2( top_left.x , bottom_right.y - length ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( bottom_right , ImVec2( bottom_right.x - length , bottom_right.y ) , box_color , fortnite::settings::players::box_thickness );
                draw_list->AddLine( bottom_right , ImVec2( bottom_right.x , bottom_right.y - length ) , box_color , fortnite::settings::players::box_thickness );
            }
            else if ( fortnite::settings::players::box_type == 2 ) {
                draw_3d_box( draw_list , corners , box_color , fortnite::settings::players::box_thickness );
            }
            else if ( fortnite::settings::players::box_type == 3 ) {
                ImU32 filled_col = ImGui::ColorConvertFloat4ToU32( ImVec4(
                    is_visible ? fortnite::settings::players::visible_color [ 0 ] : fortnite::settings::players::invisible_color [ 0 ] ,
                    is_visible ? fortnite::settings::players::visible_color [ 1 ] : fortnite::settings::players::invisible_color [ 1 ] ,
                    is_visible ? fortnite::settings::players::visible_color [ 2 ] : fortnite::settings::players::invisible_color [ 2 ] ,
                    0.175f ) );
                draw_list->AddRectFilled( top_left , bottom_right , filled_col );
                draw_list->AddRect( top_left , bottom_right , box_color , 0.0f , 0 , fortnite::settings::players::box_thickness );
            }
            else if ( fortnite::settings::players::box_type == 4 ) {
                draw_3d_corner_box( draw_list , corners , box_color , fortnite::settings::players::box_thickness );
            }
        }

        const float center_x = ( min_x + max_x ) / 2.0f;
        const float top_y = min_y;
        const float bottom_y = max_y;

        int dist_m_int = static_cast< int >( dist_m );

        // Draw health and shield bars
        {
            const auto hb = fortnite::health::get_bar( actor.current , dist_m * 100.0f );
            const float frac = hb.max > 0.0f ? ( hb.value / hb.max ) : 0.0f;
            const float shield_frac = hb.max_shield > 0.0f ? ( hb.shield / hb.max_shield ) : 0.0f;
            const float bar_w = 6.0f;
            const float bar_gap = 2.0f;

            const ImVec2 health_bar_a( min_x - bar_w - 3.0f , top_y );
            const ImVec2 health_bar_b( min_x - 3.0f , bottom_y );

            const ImVec2 shield_bar_a( min_x - bar_w * 2.0f - bar_gap - 3.0f , top_y );
            const ImVec2 shield_bar_b( min_x - bar_w - bar_gap - 3.0f , bottom_y );

            ImU32 health_fill = IM_COL32( 70 , 220 , 90 , 255 );
            if ( hb.dbno )
                health_fill = IM_COL32( 255 , 200 , 70 , 255 );
            else if ( hb.is_healing )
                health_fill = IM_COL32( 100 , 150 , 255 , 255 );

            const ImU32 shield_fill = IM_COL32( 100 , 180 , 255 , 255 );
            const ImU32 bg = IM_COL32( 0 , 0 , 0 , 140 );

            if ( fortnite::settings::players::health_bar )
                draw_bar( draw_list , health_bar_a , health_bar_b , frac , health_fill , bg );

            if ( fortnite::settings::players::shield_bar && hb.max_shield > 0.0f )
                draw_bar( draw_list , shield_bar_a , shield_bar_b , shield_frac , shield_fill , bg );
        }

        float top_offset = top_y - font_size;

        if ( fortnite::settings::players::distance && dist_m_int > 0 )
        {
            std::string dist_str = to_lower( "[" + std::to_string( dist_m_int ) + "m]" );
            draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , top_offset ) , fortnite::engine::helpers::get_distance_color( dist_m_int ) , dist_str.c_str( ) );
            top_offset -= font_size * 1.1f;
        }

        if ( fortnite::settings::players::platform && !actor.platform.empty( ) )
        {
            std::string plat_str = to_lower( "[" + actor.platform + "]" );
            draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , top_offset ) , fortnite::engine::helpers::get_platform_color( actor.platform ) , plat_str.c_str( ) );
            top_offset -= font_size * 1.1f;
        }

        if ( fortnite::settings::players::name && !actor.name.empty( ) )
        {
            std::string name_str = to_lower( actor.name );
            draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , top_offset ) , ImColor( 255 , 255 , 255 ) , name_str.c_str( ) );
        }

        float bottom_text_offset = bottom_y + font_size * 0.8f;

        if ( fortnite::settings::players::weapon )
        {
            auto current_weapon = fortnite::communcations::read<uint64_t>( actor.current + offsets::current_weapon );
            if ( current_weapon ) {
                auto weapon_data = fortnite::communcations::read<uint64_t>( current_weapon + offsets::weapon_data );
                auto name = communcations::read<ueegnine::ftext>( weapon_data + 0x38 );
                auto real_name = name.get( );
                int ammo_count = fortnite::communcations::read<int>( current_weapon + offsets::ammo_count );
                std::string wep_str = to_lower( real_name );
                if ( fortnite::settings::players::ammo_count )
                {
                    wep_str += " [";
                    wep_str += std::to_string( ammo_count );
                    wep_str += "]";
                }
                const ImVec2 bottom_wep_pos( center_x , bottom_text_offset );
                draw_text_outlined( draw_list , font , font_size , bottom_wep_pos , fortnite::engine::helpers::get_weapon_color( 1 ) , wep_str.c_str( ) );
                bottom_text_offset += font_size * 1.1f;
            }
        }

        if ( fortnite::settings::players::rank && !actor.rank.empty( ) && actor.rank != "None" )
        {
            std::string rank_str = to_lower( actor.rank );
            const ImVec2 bottom_rank_pos( center_x , bottom_text_offset );
            draw_text_outlined( draw_list , font , font_size , bottom_rank_pos , 0 , rank_str.c_str( ) );
            bottom_text_offset += font_size * 1.1f;
        }

        if ( actor.current && fortnite::settings::players::rice_hat ) {
            rice_hat( head_world , 25.0f , 20.0f , 20 , 1.0f , ImColor( ImVec4( fortnite::settings::players::rice_hat_color [ 0 ] , fortnite::settings::players::rice_hat_color [ 1 ] , fortnite::settings::players::rice_hat_color [ 2 ] , fortnite::settings::players::rice_hat_color [ 3 ] ) ) );
        }

        if ( actor.current && ( fortnite::settings::players::velocity || fortnite::settings::players::prediction ) )
        {
            fortnite::uemath::fvector velocity = fortnite::communcations::read<fortnite::uemath::fvector>( root_comp + 0x188 );
            float speed = std::sqrt( velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z );

            if ( fortnite::settings::players::prediction )
            {
                float velocity_scale = 0.5f;
                fortnite::uemath::fvector velocity_end_3d = {
                    head_world.x + ( velocity.x * velocity_scale ),
                    head_world.y + ( velocity.y * velocity_scale ),
                    head_world.z + ( velocity.z * velocity_scale )
                };

                uemath::fvector2d velocity_screen_end = fortnite::engine::camera::world_to_screen( velocity_end_3d );
                ImVec2 start = ImVec2( head_screen.x , head_screen.y );
                uemath::fvector2d  end = velocity_screen_end;
                draw_list->AddLine( start , ImVec2( end.x , end.y ) , IM_COL32( 255 , 255 , 255 , 255 ) , 1.5f );
                float angle = atan2f( end.y - start.y , end.x - start.x );
                float head_size = 10.0f;

                ImVec2 p1 = ImVec2( end.x - head_size * cosf( angle - 0.5f ) , end.y - head_size * sinf( angle - 0.5f ) );
                ImVec2 p2 = ImVec2( end.x - head_size * cosf( angle + 0.5f ) , end.y - head_size * sinf( angle + 0.5f ) );

                draw_list->AddLine( ImVec2( end.x , end.y ) , p1 , IM_COL32( 255 , 255 , 255 , 255 ) , 1.5f );
                draw_list->AddLine( ImVec2( end.x , end.y ) , p2 , IM_COL32( 255 , 255 , 255 , 255 ) , 1.5f );
            }

            if ( fortnite::settings::players::velocity )
            {
                std::string vel_str = "[" + std::to_string( ( int ) ( speed / 100 ) ) + " m/s]";
                const ImVec2 bottom_velocity_pos( center_x , bottom_text_offset );
                draw_text_outlined( draw_list , font , font_size , bottom_velocity_pos , ImColor( 255 , 255 , 255 ) , vel_str.c_str( ) );
                bottom_text_offset += font_size * 1.1f;
            }

            if ( fortnite::settings::players::squad_size && !actor.name.empty( ) )
            {
                std::string squad_str = "unknown";
                switch ( actor.squad_size )
                {
                case 1: squad_str = "solo"; break;
                case 2: squad_str = "duo"; break;
                case 3: squad_str = "trio"; break;
                case 4: squad_str = "squad"; break;
                default:
                {
                    if ( actor.squad_size > 4 )
                        squad_str = "squad";
                    break;
                }
                }
                draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , bottom_text_offset ) , ImColor( 255 , 255 , 255 ) , squad_str.c_str( ) );
                bottom_text_offset += font_size * 1.1f;
            }
        }

        if ( fortnite::settings::players::snaplines ) {
            ImVec2 line_start;
            bool valid_start = true;

            if ( fortnite::settings::players::snaplines_start == 0 ) {
                if ( fortnite::entity::local_pawn ) {
                    auto current_weapon = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + offsets::current_weapon );
                    if ( current_weapon ) {
                        auto weapon_mesh = fortnite::communcations::read<uint64_t>( current_weapon + 0xfc0 );
                        if ( weapon_mesh ) {
                            auto weapon_muzzle = fortnite::engine::bone::get_bone_location( weapon_mesh , 3 );
                            auto weapon_muzzle_screen = fortnite::engine::camera::world_to_screen( weapon_muzzle );
                            line_start = ImVec2( weapon_muzzle_screen.x , weapon_muzzle_screen.y );
                        }
                        else {
                            valid_start = false;
                        }
                    }
                    else {
                        valid_start = false;
                    }
                }
                else {
                    valid_start = false;
                }
            }
            else if ( fortnite::settings::players::snaplines_start == 1 ) {
                line_start = ImVec2( fortnite::render::width / 2.0f , fortnite::render::height / 2.0f );
            }
            else if ( fortnite::settings::players::snaplines_start == 2 ) {
                line_start = ImVec2( fortnite::render::width / 2.0f , 0.0f );
            }
            else if ( fortnite::settings::players::snaplines_start == 3 ) {
                line_start = ImVec2( fortnite::render::width / 2.0f , ( float ) fortnite::render::height );
            }

            if ( valid_start ) {
                draw_list->AddLine( line_start , ImVec2( head_screen.x , head_screen.y ) , box_color , 1.5f );
            }
        }
    }
}

void fortnite::drawing::draw_pickups( ImDrawList* draw_list )
{
    if ( !draw_list || fortnite::settings::world::battlemode_toggle || !fortnite::settings::world::enabled || !fortnite::settings::world::pickups )
        return;

    ImFont* font = ImGui::GetFont( );
    const float font_size = ImGui::GetFontSize( );

    auto items = fortnite::entity::get_p( );
    if ( !items || items->empty( ) )
        return;

    for ( const auto& item : *items )
    {
        if ( !item.current || !item.root_comp )
            continue;

        const uint64_t root = item.root_comp;
        if ( !root )
            continue;

        auto location = communcations::read<fortnite::uemath::fvector>( root + offsets::relative_location );
        float dist_m = fortnite::engine::camera::location.distance( location ) / 100.0f;
        if ( dist_m >= fortnite::settings::world::max_distance_master || dist_m >= fortnite::settings::world::max_distance_pickups )
            continue;

        auto rotation = communcations::read<fortnite::uemath::frotator>( root + 0x158 );
        auto scale = communcations::read<fortnite::uemath::fvector>( root + 0x170 );

        fortnite::uemath::fvector extent = { 30.f, 30.f, 15.f };
        D3DMATRIX rotMatrix = fortnite::ueegnine::create_rotation_matrix( rotation );

        float min_x = 99999.0f;
        float max_x = -99999.0f;
        float min_y = 99999.0f;
        float max_y = -99999.0f;

        ImVec2 corners [ 8 ];
        for ( int i = 0; i < 8; ++i )
        {
            fortnite::uemath::fvector local;
            local.x = ( i & 1 ? extent.x : -extent.x );
            local.y = ( i & 2 ? extent.y : -extent.y );
            local.z = ( i & 4 ? extent.z : -extent.z );

            fortnite::uemath::fvector scaled;
            scaled.x = local.x * scale.x;
            scaled.y = local.y * scale.y;
            scaled.z = local.z * scale.z;

            fortnite::uemath::fvector rotated;
            rotated.x = scaled.x * rotMatrix._11 + scaled.y * rotMatrix._21 + scaled.z * rotMatrix._31;
            rotated.y = scaled.x * rotMatrix._12 + scaled.y * rotMatrix._22 + scaled.z * rotMatrix._32;
            rotated.z = scaled.x * rotMatrix._13 + scaled.y * rotMatrix._23 + scaled.z * rotMatrix._33;

            fortnite::uemath::fvector corner = location + rotated;

            auto corner_screen = fortnite::engine::camera::world_to_screen( corner );
            corners [ i ] = ImVec2( static_cast< float >( corner_screen.x ) , static_cast< float >( corner_screen.y ) );

            min_x = ( std::min ) ( min_x , static_cast< float >( corner_screen.x ) );
            max_x = ( std::max ) ( max_x , static_cast< float >( corner_screen.x ) );
            min_y = ( std::min ) ( min_y , static_cast< float >( corner_screen.y ) );
            max_y = ( std::max ) ( max_y , static_cast< float >( corner_screen.y ) );
        }

        if ( !on_screen( fortnite::uemath::fvector2d( min_x , min_y ) , 50 ) && !on_screen( fortnite::uemath::fvector2d( max_x , max_y ) , 50 ) )
            continue;

        if ( static_cast< int >( item.rarity ) < fortnite::settings::world::min_rarity_show )
            continue;

        const ImColor rarity_color = fortnite::engine::helpers::get_weapon_color( item.rarity );
        const ImU32 col32 = ImGui::ColorConvertFloat4ToU32( rarity_color );

        const float center_x = ( min_x + max_x ) / 2.0f;
        const float top_y = min_y;

        switch ( fortnite::settings::world::pickup_box_type ) {
        case 0: draw_3d_box( draw_list , corners , col32 , 1.0f ); break;
        case 1: draw_list->AddRect( ImVec2( min_x , min_y ) , ImVec2( max_x , max_y ) , col32 , 0.0f , 0 , 1.5f ); break;
        case 2: draw_3d_corner_box( draw_list , corners , col32 , 1.0f ); break;
        case 3: break;
        }

        std::string name_str = item.name;
        std::transform( name_str.begin( ) , name_str.end( ) , name_str.begin( ) , [ ]( unsigned char c ) { return ( char ) std::tolower( c ); } );

        draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , top_y - font_size * 1.5f ) , col32 , name_str.c_str( ) );
    }
}

void fortnite::drawing::draw_containers( ImDrawList* draw_list )
{
    if ( !draw_list || fortnite::settings::world::battlemode_toggle || !fortnite::settings::world::enabled || !fortnite::settings::world::containers )
        return;

    ImFont* font = ImGui::GetFont( );
    const float font_size = ImGui::GetFontSize( );

    auto containers = fortnite::entity::get_containers( );
    if ( !containers || containers->empty( ) )
        return;

    for ( const auto& container : *containers )
    {
     

        auto location = fortnite::communcations::read<fortnite::uemath::fvector>(
            container.root_component + offsets::relative_location );
        float dist_m = fortnite::engine::camera::location.distance( location ) / 100.0f;

        if ( dist_m >= fortnite::settings::world::max_distance_master || dist_m >= fortnite::settings::world::max_distance_containers )
            continue;

        auto rotation = fortnite::communcations::read<fortnite::uemath::frotator>(
            container.root_component + 0x158 );

        auto scale = fortnite::communcations::read<fortnite::uemath::fvector>(
            container.root_component + 0x170 );

        fortnite::uemath::fvector extent;

        switch ( container.spawn_source )
        {
        case EFortPickupSpawnSource::Chest:
            extent = { 25.f, 50.f, 24.f };
            break;
        case EFortPickupSpawnSource::AmmoBox:
            extent = { 20.f, 20.f, 18.f };
            break;
        case EFortPickupSpawnSource::SupplyDrop:
            extent = { 60.f, 60.f, 40.f };
            break;
        case EFortPickupSpawnSource::LootDrop:
            extent = { 35.f, 35.f, 25.f };
            break;
        default:
            extent = { 30.f, 30.f, 15.f };
            break;
        }

        D3DMATRIX rotMatrix = fortnite::ueegnine::create_rotation_matrix( rotation );

        float min_x = FLT_MAX;
        float max_x = -FLT_MAX;
        float min_y = FLT_MAX;
        float max_y = -FLT_MAX;

        ImVec2 corners [ 8 ];

        for ( int i = 0; i < 8; ++i )
        {
            fortnite::uemath::fvector local;
            local.x = ( i & 1 ? extent.x : -extent.x );
            local.y = ( i & 2 ? extent.y : -extent.y );
            local.z = ( i & 4 ? extent.z : -extent.z );

            fortnite::uemath::fvector scaled;
            scaled.x = local.x * scale.x;
            scaled.y = local.y * scale.y;
            scaled.z = local.z * scale.z;

            fortnite::uemath::fvector rotated;
            rotated.x = scaled.x * rotMatrix._11 + scaled.y * rotMatrix._21 + scaled.z * rotMatrix._31;
            rotated.y = scaled.x * rotMatrix._12 + scaled.y * rotMatrix._22 + scaled.z * rotMatrix._32;
            rotated.z = scaled.x * rotMatrix._13 + scaled.y * rotMatrix._23 + scaled.z * rotMatrix._33;

            fortnite::uemath::fvector world_corner = location + rotated;

            auto corner_screen = fortnite::engine::camera::world_to_screen( world_corner );

            corners [ i ] = ImVec2( ( float ) corner_screen.x , ( float ) corner_screen.y );

            min_x = min( min_x , ( float ) corner_screen.x );
            max_x = max( max_x , ( float ) corner_screen.x );
            min_y = min( min_y , ( float ) corner_screen.y );
            max_y = max( max_y , ( float ) corner_screen.y );
        }

        if ( !on_screen( fortnite::uemath::fvector2d( min_x , min_y ) , 50 ) && !on_screen( fortnite::uemath::fvector2d( max_x , max_y ) , 50 ) )
            continue;

        ImU32 box_color = IM_COL32( 100 , 200 , 100 , 255 );

        switch ( container.spawn_source )
        {
        case EFortPickupSpawnSource::Chest:
            box_color = IM_COL32( 255 , 215 , 0 , 255 );
            break;
        case EFortPickupSpawnSource::AmmoBox:
            box_color = IM_COL32( 255 , 100 , 100 , 255 );
            break;
        case EFortPickupSpawnSource::SupplyDrop:
            box_color = IM_COL32( 100 , 200 , 255 , 255 );
            break;
        case EFortPickupSpawnSource::LootDrop:
            box_color = IM_COL32( 200 , 100 , 255 , 255 );
            break;
        }

        switch ( fortnite::settings::world::containers_box_type )
        {
        case 0: draw_3d_box( draw_list , corners , box_color , 1.0f ); break;
        case 1:
            draw_list->AddRect(
                ImVec2( min_x , min_y ) ,
                ImVec2( max_x , max_y ) ,
                box_color ,
                0.0f , 0 , 1.5f
            );
            break;
        case 2: draw_3d_corner_box( draw_list , corners , box_color , 1.0f ); break;
        case 3: break;
        }

        float center_x = ( min_x + max_x ) * 0.5f;
        float top_y = min_y;

        std::string info_str = "container [";

        switch ( container.spawn_source )
        {
        case EFortPickupSpawnSource::Chest:      info_str += "chest]"; break;
        case EFortPickupSpawnSource::AmmoBox:    info_str += "ammo]"; break;
        case EFortPickupSpawnSource::SupplyDrop:  info_str += "supply]"; break;
        case EFortPickupSpawnSource::LootDrop:    info_str += "loot]"; break;
        default:                                 info_str += "other]"; break;
        }

        draw_text_outlined( draw_list , font , font_size , ImVec2( center_x , top_y - font_size * 1.5f ) , box_color , info_str.c_str( ) );
    }
}

void fortnite::drawing::draw_vehicles( ImDrawList* draw_list )
{
    if ( !draw_list || fortnite::settings::world::battlemode_toggle || !fortnite::settings::world::enabled || !fortnite::settings::world::cars )
        return;

    ImFont* font = ImGui::GetFont( );
    const float font_size = ImGui::GetFontSize( );

    auto vehicles = fortnite::entity::get_vehicles( );
    if ( !vehicles || vehicles->empty( ) )
        return;

    for ( const auto& vehicle : *vehicles )
    {
        if ( !vehicle.mesh || !vehicle.root_component )
            continue;

        const auto bounds = fortnite::engine::bone::get_bounds( vehicle.mesh );
        auto location = fortnite::communcations::read<fortnite::uemath::fvector>( vehicle.mesh + offsets::relative_location );
        float dist_m = fortnite::engine::camera::location.distance( location ) / 100.0f;
        if ( dist_m >= fortnite::settings::world::max_distance_master || dist_m >= fortnite::settings::world::max_distance_cars )
            continue;

        auto screen = fortnite::engine::camera::world_to_screen( location );

        float min_x = 99999.0f;
        float max_x = -99999.0f;
        float min_y = 99999.0f;
        float max_y = -99999.0f;

        ImVec2 corners [ 8 ];
        for ( int i = 0; i < 8; ++i )
        {
            fortnite::uemath::fvector corner;
            corner.x = bounds.origin.x + ( i & 1 ? bounds.box_extent.x : -bounds.box_extent.x );
            corner.y = bounds.origin.y + ( i & 2 ? bounds.box_extent.y : -bounds.box_extent.y );
            corner.z = bounds.origin.z + ( i & 4 ? bounds.box_extent.z : -bounds.box_extent.z );

            auto corner_screen = fortnite::engine::camera::world_to_screen( corner );
            corners [ i ] = ImVec2( static_cast< float >( corner_screen.x ) , static_cast< float >( corner_screen.y ) );

            min_x = ( std::min ) ( min_x , static_cast< float >( corner_screen.x ) );
            max_x = ( std::max ) ( max_x , static_cast< float >( corner_screen.x ) );
            min_y = ( std::min ) ( min_y , static_cast< float >( corner_screen.y ) );
            max_y = ( std::max ) ( max_y , static_cast< float >( corner_screen.y ) );
        }

        if ( !on_screen( fortnite::uemath::fvector2d( min_x , min_y ) , 50 ) && !on_screen( fortnite::uemath::fvector2d( max_x , max_y ) , 50 ) )
            continue;

        ImU32 box_color = IM_COL32( 100 , 200 , 100 , 255 );

        if ( vehicle.critical_health <= 0.0f )
            box_color = IM_COL32( 100 , 100 , 100 , 255 );
        else if ( vehicle.critical_health < 33.0f )
            box_color = IM_COL32( 255 , 100 , 100 , 255 );
        else if ( vehicle.critical_health < 66.0f )
            box_color = IM_COL32( 255 , 200 , 0 , 255 );

        switch ( fortnite::settings::world::car_box_type )
        {
        case 0: draw_3d_box( draw_list , corners , box_color , 1.0f ); break;
        case 1:
            draw_list->AddRect(
                ImVec2( min_x , min_y ) ,
                ImVec2( max_x , max_y ) ,
                box_color ,
                0.0f , 0 , 1.5f
            );
            break;
        case 2: draw_3d_corner_box( draw_list , corners , box_color , 1.0f ); break;
        case 3: break;
        }

        const float center_x = ( min_x + max_x ) / 2.0f;
        const float top_y = min_y;

        if ( fortnite::settings::world::show_health )
        {
            std::string health_str = "vehicle [";
            if ( vehicle.critical_health <= 0.0f )
                health_str += "destroyed]";
            else
                health_str += std::to_string( static_cast< int >( vehicle.critical_health ) ) + "%]";
            draw_text_outlined( draw_list , font , font_size ,
                ImVec2( center_x , top_y - font_size * 1.5f ) ,
                box_color , health_str.c_str( ) );
        }
    }
}

void fortnite::drawing::draw_fov_circle( ImDrawList* draw_list )
{
    if ( !draw_list )
        return;

    const float cx = fortnite::render::width / 2.0f;
    const float cy = fortnite::render::height / 2.0f;

    const float r = fortnite::settings::aimbot::fov_size;

    const ImU32 col = ImGui::ColorConvertFloat4ToU32(
        ImVec4(
            fortnite::settings::fov::color [ 0 ] ,
            fortnite::settings::fov::color [ 1 ] ,
            fortnite::settings::fov::color [ 2 ] ,
            fortnite::settings::fov::color [ 3 ]
        )
    );

    if ( settings::weakspot::show_fov ) {
        draw_list->AddCircle( ImVec2( cx , cy ) , settings::weakspot::fov_size , ImColor( 255 , 0 , 0 , 255 ) , 50 , 1.5f );
    }

    if ( !fortnite::settings::aimbot::show_fov )
        return;

    if ( fortnite::settings::aimbot::outline )
    {
        draw_list->AddCircle( ImVec2( cx , cy ) , r , IM_COL32( 0 , 0 , 0 , 255 ) , fortnite::settings::aimbot::segments , 3.0f );
    }
    draw_list->AddCircle( ImVec2( cx , cy ) , r , col , fortnite::settings::aimbot::segments , 1.5f );
}

ImVec2 rotate_point( ImVec2 point , ImVec2 center , float angle ) {
    float cos_a = std::cos( angle );
    float sin_a = std::sin( angle );

    float x = point.x - center.x;
    float y = point.y - center.y;

    return ImVec2(
        center.x + x * cos_a - y * sin_a ,
        center.y + x * sin_a + y * cos_a
    );
}

void fortnite::drawing::draw_crosshair( ImDrawList* draw_list )
{
    if ( !fortnite::settings::crosshair::enabled )
        return;

    ImVec2 screen_center = ImGui::GetIO( ).DisplaySize;
    screen_center.x /= 2.f;
    screen_center.y /= 2.f;

    ImU32 color = ImGui::GetColorU32( ImVec4( fortnite::settings::crosshair::color [ 0 ] , fortnite::settings::crosshair::color [ 1 ] , fortnite::settings::crosshair::color [ 2 ] , fortnite::settings::crosshair::color [ 3 ] ) );

    float length = fortnite::settings::crosshair::length;
    float gap = fortnite::settings::crosshair::gap;
    float thickness = fortnite::settings::crosshair::thickness;
    float rotation = fortnite::settings::crosshair::rotation * 3.14159265f / 180.f;
    int style = fortnite::settings::crosshair::style;

    if ( style == 0 ) {
        ImVec2 p1 = ImVec2( screen_center.x , screen_center.y - gap - length );
        ImVec2 p2 = ImVec2( screen_center.x , screen_center.y - gap );
        ImVec2 p3 = ImVec2( screen_center.x , screen_center.y + gap );
        ImVec2 p4 = ImVec2( screen_center.x , screen_center.y + gap + length );
        ImVec2 p5 = ImVec2( screen_center.x - gap - length , screen_center.y );
        ImVec2 p6 = ImVec2( screen_center.x - gap , screen_center.y );
        ImVec2 p7 = ImVec2( screen_center.x + gap , screen_center.y );
        ImVec2 p8 = ImVec2( screen_center.x + gap + length , screen_center.y );

        if ( rotation != 0.f ) {
            p1 = rotate_point( p1 , screen_center , rotation );
            p2 = rotate_point( p2 , screen_center , rotation );
            p3 = rotate_point( p3 , screen_center , rotation );
            p4 = rotate_point( p4 , screen_center , rotation );
            p5 = rotate_point( p5 , screen_center , rotation );
            p6 = rotate_point( p6 , screen_center , rotation );
            p7 = rotate_point( p7 , screen_center , rotation );
            p8 = rotate_point( p8 , screen_center , rotation );
        }

        draw_list->AddLine( p1 , p2 , color , thickness );
        draw_list->AddLine( p3 , p4 , color , thickness );
        draw_list->AddLine( p5 , p6 , color , thickness );
        draw_list->AddLine( p7 , p8 , color , thickness );
    }
    else if ( style == 1 ) {
        float offset = ( gap + length ) / 1.414213562f;
        ImVec2 p1 = ImVec2( screen_center.x - offset , screen_center.y - offset );
        ImVec2 p2 = ImVec2( screen_center.x - gap / 1.414213562f , screen_center.y - gap / 1.414213562f );
        ImVec2 p3 = ImVec2( screen_center.x + gap / 1.414213562f , screen_center.y + gap / 1.414213562f );
        ImVec2 p4 = ImVec2( screen_center.x + offset , screen_center.y + offset );
        ImVec2 p5 = ImVec2( screen_center.x + offset , screen_center.y - offset );
        ImVec2 p6 = ImVec2( screen_center.x + gap / 1.414213562f , screen_center.y - gap / 1.414213562f );
        ImVec2 p7 = ImVec2( screen_center.x - gap / 1.414213562f , screen_center.y + gap / 1.414213562f );
        ImVec2 p8 = ImVec2( screen_center.x - offset , screen_center.y + offset );

        if ( rotation != 0.f ) {
            p1 = rotate_point( p1 , screen_center , rotation );
            p2 = rotate_point( p2 , screen_center , rotation );
            p3 = rotate_point( p3 , screen_center , rotation );
            p4 = rotate_point( p4 , screen_center , rotation );
            p5 = rotate_point( p5 , screen_center , rotation );
            p6 = rotate_point( p6 , screen_center , rotation );
            p7 = rotate_point( p7 , screen_center , rotation );
            p8 = rotate_point( p8 , screen_center , rotation );
        }

        draw_list->AddLine( p1 , p2 , color , thickness );
        draw_list->AddLine( p3 , p4 , color , thickness );
        draw_list->AddLine( p5 , p6 , color , thickness );
        draw_list->AddLine( p7 , p8 , color , thickness );
    }
    else if ( style == 2 ) {
        draw_list->AddCircle( screen_center , length + gap , color , 32 , thickness );
    }
    else if ( style == 3 ) {
        float half = length + gap;
        ImVec2 p1 = ImVec2( screen_center.x - half , screen_center.y - half );
        ImVec2 p2 = ImVec2( screen_center.x + half , screen_center.y - half );
        ImVec2 p3 = ImVec2( screen_center.x + half , screen_center.y + half );
        ImVec2 p4 = ImVec2( screen_center.x - half , screen_center.y + half );

        if ( rotation != 0.f ) {
            p1 = rotate_point( p1 , screen_center , rotation );
            p2 = rotate_point( p2 , screen_center , rotation );
            p3 = rotate_point( p3 , screen_center , rotation );
            p4 = rotate_point( p4 , screen_center , rotation );
        }

        draw_list->AddLine( p1 , p2 , color , thickness );
        draw_list->AddLine( p2 , p3 , color , thickness );
        draw_list->AddLine( p3 , p4 , color , thickness );
        draw_list->AddLine( p4 , p1 , color , thickness );
    }

    if ( fortnite::settings::crosshair::dot ) {
        draw_list->AddCircleFilled( screen_center , fortnite::settings::crosshair::dot_size , color );
    }
}

void fortnite::drawing::draw_bullet_tracers( ImDrawList* draw_list )
{
    uint64_t local_weapon = communcations::read<uint64_t>( fortnite::entity::local_pawn + offsets::current_weapon );

    if ( local_weapon && is_weapon_firing( local_weapon ) ) {

        fortnite::uemath::fvector cameraForward;
        float cp = cosf( deg_to_rad( fortnite::engine::camera::rotation.pitch ) );
        float sp = sinf( deg_to_rad( fortnite::engine::camera::rotation.pitch ) );
        float cy = cosf( deg_to_rad( fortnite::engine::camera::rotation.yaw ) );
        float sy = sinf( deg_to_rad( fortnite::engine::camera::rotation.yaw ) );

        cameraForward.x = cp * cy;
        cameraForward.y = cp * sy;
        cameraForward.z = sp;
        cameraForward.normalize( );

        fortnite::uemath::fvector direction = cameraForward;
        auto impact_direction = communcations::read<fortnite::uemath::fvector>( fortnite::entity::local_pawn + offsets::last_fired_direction );

        auto impact_angle = acos( direction.dot( impact_direction ) );
        auto start_location = communcations::read<fortnite::uemath::fvector>( fortnite::entity::local_pawn + offsets::last_fired_location );
        auto end_location = start_location + ( impact_direction * communcations::read<float>( local_weapon + offsets::current_projected_impact_distance ) );
        if ( !is_duplicate_tracer( start_location , end_location ) ) {
            add_bullet_tracer( start_location , end_location , impact_angle );
        }
    }

    update_and_draw_tracers( );
}


namespace fortnite::drawing
{
    struct AimbotState
    {
        float smooth_x = 0.0f;
        float smooth_y = 0.0f;
        float last_move_x = 0.0f;
        float last_move_y = 0.0f;
    };

    static AimbotState aimbot_state;

    static bool switched_to_pickaxe = false;
    static ULONGLONG swing_time = 0;
    static bool was_switched = false;
    static ULONGLONG switch_time = 0;

    void draw_weakspot( ImDrawList* draw_list )
    {
        if ( fortnite::settings::world::battlemode_toggle )
            return;
        if ( !fortnite::settings::world::enabled )
            return;
        if ( !fortnite::settings::world::weakspots )
            return;
        if ( !draw_list )
            return;
        ImFont* font = ImGui::GetFont( );
        const float font_size = ImGui::GetFontSize( );

        auto weakspots = fortnite::entity::get_weakspots( );
        auto world_data = fortnite::world::get( );

        static bool was_breaking = false;

        bool has_weakspots = weakspots && !weakspots->empty( );

        if ( !has_weakspots )
        {

            if ( was_breaking )
            {
                if ( fortnite::settings::weakspot::auto_takewall ) {
                    widgets->notify.add( "weakspot ended" , "auto break finished" , notify_info );
                    keybd_event( fortnite::settings::binds::wall , 0 , 0 , 0 );
                    Sleep( 1 );
                    keybd_event( fortnite::settings::binds::wall , 0 , KEYEVENTF_KEYUP , 0 );
                    Sleep( 10 );
                    keybd_event( VK_LBUTTON , 0 , 0 , 0 );
                    Sleep( 1 );
                    keybd_event( VK_LBUTTON , 0 , KEYEVENTF_KEYUP , 0 );
                    Sleep( 1 );

                    keybd_event( fortnite::settings::binds::shotgun_slot , 0 , 0 , 0 );

                    keybd_event( fortnite::settings::binds::shotgun_slot , 0 , KEYEVENTF_KEYUP , 0 );
                    widgets->notify.add( "wall placed" , "auto take wall finsihed" , notify_info );
                }
                was_breaking = false;
            }

            return;
        }

        const float center_x = fortnite::render::width * 0.5f;
        const float center_y = fortnite::render::height * 0.5f;

        struct BestTarget
        {
            fortnite::uemath::fvector location;
            float distance = FLT_MAX;
            bool found = false;
        } best_target;

        for ( const auto& weakspot : *weakspots )
        {
            if ( !weakspot.root_component )
                continue;

            auto location = fortnite::communcations::read<fortnite::uemath::fvector>( weakspot.root_component + fortnite::offsets::relative_location );
            float dist_m = fortnite::engine::camera::location.distance( location ) / 100.0f;
            if ( dist_m >= fortnite::settings::world::max_distance_master ) continue;
            if ( dist_m >= fortnite::settings::world::max_distance_weakspots ) continue;
            auto screen = fortnite::engine::camera::world_to_screen( location );

            if ( !on_screen( fortnite::uemath::fvector2d( screen.x , screen.y ) , 150 ) )
                continue;
            if ( fortnite::settings::weakspot::show_weakspot ) {
                draw_text_outlined( draw_list , font , font_size , ImVec2( screen.x , screen.y ) , ImColor( 255 , 0 , 0 , 255 ) , "weakspot" );
            }

            float dx = screen.x - center_x;
            float dy = screen.y - center_y;
            float dist = std::sqrt( ( dx * dx ) + ( dy * dy ) );
            if ( dist > fortnite::settings::weakspot::fov_size )
                continue;

            if ( fortnite::settings::weakspot::aimbot &&
                dist < best_target.distance )
            {
                best_target.distance = dist;
                best_target.location = location;
                best_target.found = true;
            }
        }

        static bool is_on_pickaxe = false;
        static ULONGLONG pickaxe_switch_time = 0;
        static ULONGLONG last_swing = 0;

        auto check_holding_pickaxe = [ world_data ]( ) -> bool
            {
                auto current_weapon = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + fortnite::offsets::current_weapon );
                auto weapon_data = fortnite::communcations::read<uint64_t>( current_weapon + fortnite::offsets::weapon_data );
                auto name = fortnite::communcations::read<fortnite::ueegnine::ftext>( weapon_data + 0x38 );
                return name.get( ).find( "Pickaxe" ) != std::string::npos;
            };

        if ( fortnite::settings::weakspot::aimbot &&
            best_target.found )
        {
            if ( !was_breaking )
            {
                widgets->notify.add(
                    "found weakspot" ,
                    "valid weakspot found" ,
                    notify_info
                );

                was_breaking = true;
            }

            auto final_screen = fortnite::engine::camera::world_to_screen( best_target.location );
            float delta_x = final_screen.x - center_x;
            float delta_y = final_screen.y - center_y;

            float current_smooth_x = fortnite::settings::aimbot::smooth_x;
            float current_smooth_y = fortnite::settings::aimbot::smooth_y;

            float factor_x = std::clamp( 1.0f - current_smooth_x , 0.01f , 1.0f );
            float factor_y = std::clamp( 1.0f - current_smooth_y , 0.01f , 1.0f );

            float move_x = delta_x * factor_x;
            float move_y = delta_y * factor_y;


            if ( std::abs( move_x ) > 0.001f || std::abs( move_y ) > 0.001f )
            {
                auto world_data = fortnite::world::get( );
                if ( world_data )
                {
                    uint64_t local_player = world_data->local_players.get( 0 );
                    if ( local_player )
                    {
                        auto player_controller = fortnite::communcations::read<uint64_t>( local_player + fortnite::offsets::player_controller );
                        if ( player_controller )
                        {
                            auto current_rotation = fortnite::communcations::read<fortnite::uemath::frotator>( player_controller + 0x26f0 );
                            fortnite::uemath::frotator new_rotation {};
                            new_rotation.pitch = current_rotation.pitch - ( move_y * 0.01f );
                            new_rotation.yaw = current_rotation.yaw + ( move_x * 0.01f );
                            new_rotation.roll = 0.0f;
                            fortnite::communcations::write<fortnite::uemath::frotator>( player_controller + 0x26f0 , new_rotation );
                        }
                    }
                }
            }
            if ( fortnite::settings::weakspot::auto_hit )
            {
                float crosshair_distance = std::sqrt( ( delta_x * delta_x ) + ( delta_y * delta_y ) );

                bool already_holding_pickaxe = check_holding_pickaxe( );

                if ( !is_on_pickaxe && !already_holding_pickaxe )
                {
                    std::cout << "sending\n";
                    keybd_event( fortnite::settings::binds::pickaxe , 0 , 0 , 0 );
                    keybd_event( fortnite::settings::binds::pickaxe , 0 , KEYEVENTF_KEYUP , 0 );
                    is_on_pickaxe = true;
                    pickaxe_switch_time = GetTickCount64( );
                    last_swing = 0;
                }

                ULONGLONG now = GetTickCount64( );
                already_holding_pickaxe = check_holding_pickaxe( );

                int switch_delay = already_holding_pickaxe ? 0 : 200;

                if ( ( now - pickaxe_switch_time >= switch_delay ) &&
                    crosshair_distance <= 30.0f &&
                    ( now - last_swing > 450 ) )
                {
                    keybd_event( VK_LBUTTON , 0 , 0 , 0 );
                    last_swing = now;
                }
            }
        }
        else
        {
            was_breaking = false;
        }
    }
}
