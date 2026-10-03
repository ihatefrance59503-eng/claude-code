// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "widgets.hpp"

inline ImVec2 spectator_pos = ImVec2( 20 , 200 );
inline ImVec2 keybind_pos = ImVec2( 20 , 350 );

inline bool dragging_spectators = false;
inline bool dragging_keybinds = false;

inline ImVec2 spectator_drag_offset;
inline ImVec2 keybind_drag_offset;

void fortnite::widget::handle_drag( ImVec2& pos , ImVec2 size , bool& dragging , ImVec2& offset )
{
    auto& io = ImGui::GetIO( );
    ImVec2 mouse = io.MousePos;

    bool hovering = mouse.x >= pos.x && mouse.y >= pos.y && mouse.x <= pos.x + size.x && mouse.y <= pos.y + 42.f;

    if ( hovering && ImGui::IsMouseClicked( 0 ) )
    {
        dragging = true;
        offset = mouse - pos;
    }

    if ( !ImGui::IsMouseDown( 0 ) )
        dragging = false;

    if ( dragging )
        pos = mouse - offset;
}
struct spectator_t
{
    std::string name;
    float watch_time = 0.f;
    float last_seen = 0.f;
};

inline std::vector< spectator_t > spectators_list;
inline std::mutex spectators_mutex;

void fortnite::widget::find_spectators( )
{
    static auto last_update = std::chrono::steady_clock::now( );

    auto now = std::chrono::steady_clock::now( );
    if ( std::chrono::duration_cast< std::chrono::seconds >( now - last_update ).count( ) < 1 )
        return;

    last_update = now;

    auto world_data = fortnite::world::get( );
    if ( !world_data )
        return;

    auto player_state = fortnite::communcations::read<uint64_t>( fortnite::entity::local_pawn + offsets::player_state );

    if ( !player_state )
        return;

    auto spectators = fortnite::communcations::read<ESpectatorArray>( player_state + 0xab0 + 0x108 );

    std::set< std::string > current_names;

    for ( uint32_t i = 0; i < spectators.Count; ++i )
    {
        uintptr_t item_address = spectators.Data + ( i * 0x18 );
        uintptr_t player_state = fortnite::communcations::read<uintptr_t>( item_address + 0x10 );

        if ( !player_state )
            continue;

        std::string name =
            fortnite::engine::helpers::decrypt_name( player_state , 0 );

        if ( name.empty( ) )
            continue;

        current_names.insert( name );
    }

    std::lock_guard lock( spectators_mutex );
    for ( auto& spectator : spectators_list )
    {
        if ( current_names.contains( spectator.name ) )
        {
            spectator.watch_time += 1.f;
            spectator.last_seen = ImGui::GetTime( );
        }
    }

    for ( auto& name : current_names )
    {
        bool exists = false;

        for ( auto& spectator : spectators_list )
        {
            if ( spectator.name == name )
            {
                exists = true;
                break;
            }
        }

        if ( !exists )
        {
            spectator_t spec;
            spec.name = name;
            spec.watch_time = 0.f;
            spec.last_seen = ImGui::GetTime( );

            spectators_list.push_back( spec );
        }
    }

    spectators_list.erase( std::remove_if( spectators_list.begin( ) , spectators_list.end( ) , [ & ]( const spectator_t& spectator ) { return !current_names.contains( spectator.name ); } ) , spectators_list.end( ) );
}
void fortnite::widget::specatator_widget( )
{
    using namespace ImGui;

    find_spectators( );

    auto draw_list = GetBackgroundDrawList( );

    float width = 260.f;
    float line_height = 20.f;

    std::lock_guard lock( spectators_mutex );

    float height = 45.f + ( spectators_list.size( ) * line_height );

    ImVec2 size = ImVec2( width , height );

    handle_drag( spectator_pos , size , dragging_spectators , spectator_drag_offset );
    ImVec2 pos = spectator_pos;
    pcolor color = pcolor( 255 , 255 , 255 , 255 );

    draw_list->AddRectFilled( pos , pos + size , g_style->col( pcol_bg ) , 4 ds );
    draw_list->AddRectFilled( pos + vec2 { 40, 6 } , pos + size - vec2 { 6, 6 } , g_style->col( pcol_bg2 ) , 3 ds );
    draw_list->AddRectFilled( { pos.x, pos.y + size.y - 2 ds } , { pos.x + size.x, pos.y + size.y } , color );
    float header_h = 42.f;

    g_draw->text( icons , 16 , ImVec2( pos.x + 12 , pos.y + ( header_h * 0.5f ) - 8.f ) , color , I_EYE , false , draw_list );
    g_draw->text( font , 16 , pos + vec2 { 50, 14 } , color , std::string( "Spectators [" ) + std::to_string( spectators_list.size( ) ) + "]" , false , draw_list );

    float y = pos.y + 40.f;

    for ( auto& spectator : spectators_list )
    {
        int total_seconds = ( int ) spectator.watch_time;

        int minutes = total_seconds / 60;
        int seconds = total_seconds % 60;

        char time_buf [ 32 ];
        sprintf_s( time_buf , "%02i:%02i" , minutes , seconds );

        g_draw->text( font , 14 , ImVec2( pos.x + 12 , y ) , color , spectator.name , false , draw_list );
        g_draw->text( font , 14 , ImVec2( pos.x + width - 55 , y ) , color , time_buf , false , draw_list );

        y += line_height;
    }
}
std::string fortnite::widget::get_key_name( int vk )
{
    switch ( vk )
    {
    case VK_LBUTTON: return "M1";
    case VK_RBUTTON: return "M2";
    case VK_MBUTTON: return "M3";
    case VK_XBUTTON1: return "M4";
    case VK_XBUTTON2: return "M5";

    case VK_SHIFT: return "SHIFT";
    case VK_CONTROL: return "CTRL";
    case VK_MENU: return "ALT";

    default:
    {
        char name [ 64 ] = { 0 };

        UINT scan = MapVirtualKeyA( vk , MAPVK_VK_TO_VSC );

        GetKeyNameTextA( scan << 16 , name , sizeof( name ) );

        return name;
    }
    }
}
bool fortnite::widget::is_key_down( int vk )
{
    return ( GetAsyncKeyState( vk ) & 0x8000 );
}
void fortnite::widget::keybind_widget( )
{
    using namespace ImGui;

    struct bind_t
    {
        std::string name;
        int key;
    };

    std::vector< bind_t > active_binds;

    if ( fortnite::settings::aimbot::aimbot && is_key_down( fortnite::settings::aimbot::hotkey ) )
        active_binds.push_back( { "Aimbot", fortnite::settings::aimbot::hotkey } );

    if ( fortnite::settings::trigger::enabled && is_key_down( fortnite::settings::trigger::hotkey ) )
        active_binds.push_back( { "Triggerbot", fortnite::settings::trigger::hotkey } );

    if ( fortnite::settings::weakspot::aimbot && is_key_down( fortnite::settings::weakspot::hotkey ) )
        active_binds.push_back( { "Weakspot", fortnite::settings::weakspot::hotkey } );
    if ( fortnite::settings::world::battlemode && is_key_down( fortnite::settings::world::battlemode ) )
        active_binds.push_back( { "Battlemode", fortnite::settings::world::battlemode } );
    auto draw_list = GetBackgroundDrawList( );

    float width = 260.f;
    float line_height = 20.f;

    float height = 45.f + ( active_binds.size( ) * line_height );

    ImVec2 size = ImVec2( width , height );

    handle_drag( keybind_pos , size , dragging_keybinds , keybind_drag_offset );

    ImVec2 pos = keybind_pos;

    pcolor color = pcolor( 255 , 255 , 255 , 255 );

    draw_list->AddRectFilled( pos , pos + size , g_style->col( pcol_bg ) , 4 ds );

    draw_list->AddRectFilled( pos + vec2 { 40, 6 } , pos + size - vec2 { 6, 6 } , g_style->col( pcol_bg2 ) , 3 ds );

    draw_list->AddRectFilled( { pos.x, pos.y + size.y - 2 ds } , { pos.x + size.x, pos.y + size.y } , color );

    float header_h = 42.f;

    g_draw->text( icons , 16 , ImVec2( pos.x + 12 , pos.y + ( header_h * 0.5f ) - 8.f ) , color , I_KEYBOARD , false , draw_list );
    g_draw->text( font , 16 , pos + vec2 { 50, 14 } , color , std::string( "Keybinds [" ) + std::to_string( active_binds.size( ) ) + "]" , false , draw_list );

    float y = pos.y + 40.f;

    for ( auto& bind : active_binds )
    {
        std::string key_name = get_key_name( bind.key );
        std::string text = bind.name + " [" + key_name + "]";
        g_draw->text( font , 14 , ImVec2( pos.x + 12 , y ) , color , text , false , draw_list );

        y += line_height;
    }
}