// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "radar.hpp"

void fortnite::radar::tick( )
{
    using namespace fortnite::settings::radar;

    if ( !enabled )
        return;

    ImVec2 radarPos = position;

    if ( movable )
    {
        ImGui::SetNextWindowPos( radarPos , ImGuiCond_Once );
        ImGui::SetNextWindowSize( { size * 2.f, size * 2.f } );

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |  ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus |  ImGuiWindowFlags_NoBackground;

        ImGui::Begin( "##radar" , nullptr , flags );
        position = ImGui::GetWindowPos( );
        ImGui::End( );
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList( );
    if ( !drawList )
        return;

    const float radius = size;
    const float center_x = radarPos.x + radius;
    const float center_y = radarPos.y + radius;

    drawList->AddCircleFilled(ImVec2( center_x , center_y ) ,  radius , ImColor( 0 , 0 , 0 , ( int ) opacity ) );

    if ( show_grid )
    {
        for ( int i = 1; i <= 4; i++ )
        {
            float ring = ( radius * i ) / 4.0f;
            drawList->AddCircle( ImVec2( center_x , center_y ) , ring ,  ImColor( 100 , 100 , 100 , 150 ) ,  0 , 1.0f );
        }
    }

    drawList->AddCircleFilled(
        ImVec2( center_x , center_y ) ,
        3.5f ,
        ImColor( 255 , 255 , 255 , 255 )
    );

    if ( show_fov )
    {
        float fov = fortnite::engine::camera::fov;
        float half_fov = ( fov * 0.5f ) * ( std::numbers::pi / 180.0f );

        float length = radius * 0.9f;
        float forward = -std::numbers::pi / 2.0f;

        ImVec2 left( center_x + cosf( forward - half_fov ) * length , center_y + sinf( forward - half_fov ) * length);

        ImVec2 right( center_x + cosf( forward + half_fov ) * length , center_y + sinf( forward + half_fov ) * length);

        drawList->AddTriangleFilled( ImVec2( center_x , center_y ) , left , right , ImColor( 128 , 128 , 128 , 100 ));
    }
}
fortnite::uemath::fvector2d fortnite::radar::rotate_point( uemath::fvector2d radarPos , uemath::fvector2d radarSize , uemath::fvector localPos , uemath::fvector targetPos )
{
    float radarRange = static_cast< float >( 20.f ) * 100.0f;

    float dx = targetPos.x - localPos.x;
    float dy = targetPos.y - localPos.y;
    float yaw = fortnite::engine::camera::rotation.yaw * ( float ) ( std::numbers::pi / 180.f );

    float sinYaw = sinf( yaw );
    float cosYaw = cosf( yaw );

    float x = dx * cosYaw - dy * sinYaw;
    float y = dx * sinYaw + dy * cosYaw;
    float dist = sqrtf( x * x + y * y );
    if ( dist > radarRange )
    {
        x *= radarRange / dist;
        y *= radarRange / dist;
    }

    uemath::fvector2d  radarCenter = uemath::fvector2d(
        radarPos.x + radarSize.x * 0.5f ,
        radarPos.y + radarSize.y * 0.5f
    );

    x = x / radarRange * ( radarSize.x * 0.5f );
    y = y / radarRange * ( radarSize.y * 0.5f );

    return uemath::fvector2d( radarCenter.x + x , radarCenter.y + y );
}

void fortnite::radar::clamp_to_radar( float* x , float* y , float range )
{
    float absX = fabsf( *x );
    float absY = fabsf( *y );

    if ( absX > range || absY > range )
    {
        float scale = absX > absY ? ( range / absX ) : ( range / absY );
        *x *= scale;
        *y *= scale;
    }
}
void fortnite::radar::world_to_radar( const uemath::fvector& TargetPos ,int& outX ,int& outY )
{
    using namespace fortnite::settings::radar;

    const uemath::fvector& CamLoc = engine::camera::location;
    const uemath::frotator& CamRot = engine::camera::rotation;

    float radarRange = range;
    float radarRadius = size;

    float dx = TargetPos.x - CamLoc.x;
    float dy = TargetPos.y - CamLoc.y;

    float rad = -( CamRot.yaw + 90.0f ) * ( std::numbers::pi / 180.0f );

    float cosYaw = cosf( rad );
    float sinYaw = sinf( rad );

    float rotatedX = dx * cosYaw - dy * sinYaw;
    float rotatedY = dx * sinYaw + dy * cosYaw;

    float dist = sqrtf( rotatedX * rotatedX + rotatedY * rotatedY );

    if ( dist > radarRange )
    {
        float scale = radarRange / dist;
        rotatedX *= scale;
        rotatedY *= scale;
    }

    ImVec2 center = {
        position.x + radarRadius,
        position.y + radarRadius
    };

    float normX = ( rotatedX / radarRange ) * radarRadius;
    float normY = ( rotatedY / radarRange ) * radarRadius;

    outX = ( int ) ( center.x + normX );
    outY = ( int ) ( center.y + normY );
}
void fortnite::radar::add_to_radar(uemath::fvector WorldLocation , bool bIsVisible , int Distance , uemath::frotator RelativeRotation , int32_t team_id )
{
    using namespace fortnite::settings::radar;

    int ScreenX = 0 , ScreenY = 0;
    ImDrawList* drawList = ImGui::GetForegroundDrawList( );

    world_to_radar( WorldLocation , ScreenX , ScreenY );

    ImVec2 pos( ScreenX , ScreenY );
    float playerYaw = fortnite::engine::camera::rotation.yaw;
    float radarYaw = -( playerYaw + 90.0f );
    float finalYaw = radarYaw + RelativeRotation.yaw;
    float yawRad = fortnite::engine::helpers::to_reg( finalYaw );
    float length = 6.25f;
    float width = 5.0f;
    float angle = fortnite::engine::helpers::to_reg( 130.0f );

    ImVec2 tip(pos.x + cosf( yawRad ) * length ,pos.y + sinf( yawRad ) * length);
    ImVec2 left( pos.x + cosf( yawRad + angle ) * width ,pos.y + sinf( yawRad + angle ) * width);
    ImVec2 right( pos.x + cosf( yawRad - angle ) * width ,pos.y + sinf( yawRad - angle ) * width);

    ImColor color;
    auto to_color = [ ]( const float c [ 4 ] )
    {
      return ImColor( c [ 0 ] , c [ 1 ] , c [ 2 ] , c [ 3 ] );
    };
    if ( color_by_team )
        color = fortnite::engine::helpers::get_team_color( team_id );
    else if ( color_by_visibility )
        color = bIsVisible ? to_color( visible_color ) : to_color( hidden_color );
    else
        color = to_color( enemy_color );

    drawList->AddTriangleFilled( tip , left , right , color );

    if ( show_distance )
    {
        char buf [ 32 ];
        sprintf_s( buf , "%dm" , Distance );

        ImVec2 pos_main( pos.x + 6 , pos.y );

        ImU32 color_main = IM_COL32( 255 , 255 , 255 , 200 );
        ImU32 color_outline = IM_COL32( 0 , 0 , 0 , 255 );

        drawList->AddText( ImVec2( pos_main.x - 1 , pos_main.y ) , color_outline , buf );
        drawList->AddText( ImVec2( pos_main.x + 1 , pos_main.y ) , color_outline , buf );
        drawList->AddText( ImVec2( pos_main.x , pos_main.y - 1 ) , color_outline , buf );
        drawList->AddText( ImVec2( pos_main.x , pos_main.y + 1 ) , color_outline , buf );

        drawList->AddText( pos_main , color_main , buf );
    }
}

void fortnite::radar::add_pickup_to_radar( uemath::fvector WorldLocation , uint8_t rarity )
{
    int ScreenX = 0 , ScreenY = 0;
    ImDrawList* m_DrawList = ImGui::GetForegroundDrawList( );

    world_to_radar( WorldLocation , ScreenX , ScreenY );

    ImVec2 pos( ScreenX , ScreenY );

    ImColor color = fortnite::engine::helpers::get_weapon_color( rarity );

    float radius = 3.5f;

    m_DrawList->AddCircleFilled( pos , radius , color );
}

void fortnite::radar::radar_range( float* x , float* y , float range )
{
    clamp_to_radar( x , y , range );
}

void fortnite::radar::calculate_point_on_radar( uemath::fvector vOrigin , int& screenx , int& screeny )
{
    return;
}

void fortnite::radar::calc_range( float* x , float* y , float range )
{
    clamp_to_radar( x , y , range );
}
