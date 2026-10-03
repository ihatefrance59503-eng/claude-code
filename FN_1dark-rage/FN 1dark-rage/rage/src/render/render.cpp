// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "render.hpp"
#include "../settings/settings.hpp"
#include <chrono>
#include <thread>


bool fortnite::render::set_up( HWND window )
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd , sizeof( sd ) );
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = window;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray [ 2 ] = { D3D_FEATURE_LEVEL_11_0 , D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr ,
        D3D_DRIVER_TYPE_HARDWARE ,
        nullptr ,
        createDeviceFlags ,
        featureLevelArray ,
        2 ,
        D3D11_SDK_VERSION ,
        &sd ,
        &g_pSwapChain ,
        &g_pd3dDevice ,
        &featureLevel ,
        &g_pd3dDeviceContext
    );

    if ( res == DXGI_ERROR_UNSUPPORTED )
        res = D3D11CreateDeviceAndSwapChain(
            nullptr ,
            D3D_DRIVER_TYPE_WARP ,
            nullptr ,
            createDeviceFlags ,
            featureLevelArray ,
            2 ,
            D3D11_SDK_VERSION ,
            &sd ,
            &g_pSwapChain ,
            &g_pd3dDevice ,
            &featureLevel ,
            &g_pd3dDeviceContext
        );

    if ( res != S_OK )
        return false;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer( 0 , IID_PPV_ARGS( &pBackBuffer ) );
    g_pd3dDevice->CreateRenderTargetView( pBackBuffer , nullptr , &g_mainRenderTargetView );
    pBackBuffer->Release( );

    IMGUI_CHECKVERSION( );
    ImGui::CreateContext( );

    ImGuiIO& io = ImGui::GetIO( ); ( void ) io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark( );

    ImGuiStyle& style = ImGui::GetStyle( );
    style.FrameRounding = 6;
    style.ChildRounding = 10;
    style.PopupRounding = 5;
    yahei_f = io.Fonts->AddFontFromFileTTF( "C:\\Windows\\Fonts\\arial.ttf" , 16.0f );
    if (!yahei_f) {
        yahei_f = io.Fonts->AddFontDefault();
    }

    const char* font_paths[20] = {
        "C:\\Windows\\Fonts\\tahoma.ttf",  // 0
        "C:\\Windows\\Fonts\\arial.ttf",   // 1
        "C:\\Windows\\Fonts\\verdana.ttf", // 2
        "C:\\Windows\\Fonts\\segoeui.ttf", // 3
        "C:\\Windows\\Fonts\\cour.ttf",    // 4
        "C:\\Windows\\Fonts\\comic.ttf",   // 5
        "C:\\Windows\\Fonts\\impact.ttf",  // 6
        "C:\\Windows\\Fonts\\trebuc.ttf",  // 7
        "C:\\Windows\\Fonts\\consola.ttf", // 8
        "C:\\Windows\\Fonts\\georgia.ttf", // 9
        "C:\\Windows\\Fonts\\pala.ttf",    // 10
        "C:\\Windows\\Fonts\\times.ttf",   // 11
        "C:\\Windows\\Fonts\\candara.ttf", // 12
        "C:\\Windows\\Fonts\\corbel.ttf",  // 13
        "C:\\Windows\\Fonts\\calibri.ttf", // 14
        "C:\\Windows\\Fonts\\cambria.ttf", // 15
        "C:\\Windows\\Fonts\\framd.ttf",   // 16
        "C:\\Windows\\Fonts\\micross.ttf", // 17
        "C:\\Windows\\Fonts\\lucon.ttf",   // 18
        "C:\\Windows\\Fonts\\constan.ttf"  // 19
    };

    for (int i = 0; i < 20; i++) {
        if (GetFileAttributesA(font_paths[i]) != INVALID_FILE_ATTRIBUTES) {
            window_fonts[i] = io.Fonts->AddFontFromFileTTF(font_paths[i], 16.0f);
        }
        if (!window_fonts[i]) {
            window_fonts[i] = yahei_f ? yahei_f : io.Fonts->AddFontDefault();
        }
    }

    ImGui_ImplWin32_Init( window );
    ImGui_ImplDX11_Init( g_pd3dDevice , g_pd3dDeviceContext );
  
    slate->initialize( g_pd3dDevice , g_pd3dDeviceContext , g_pSwapChain );

    return true;
}

HWND fortnite::render::find_window( )
{
    window = FindWindowA( encrypt( "Chrome_WidgetWin_1" ) , encrypt( "Discord Overlay" ) );
   return window;
}

void fortnite::render::tick( )
{
    ImVec4 clear_color = ImVec4( 0.0f , 0.0f , 0.0f , 0.0f );

    auto s = ImVec2 {} , p = ImVec2 {} , gs = ImVec2 { 876 , 623 };

    static auto last_time = std::chrono::high_resolution_clock::now();

    static ImVec2 window_pos = ImVec2( 400 , 200 );
    widgets->notify.add( "staring now" , "operations succesful" , notify_success );
    while ( true )
    {
        ImGuiIO& io = ImGui::GetIO( );

        MSG msg;
        while ( ::PeekMessage( &msg , nullptr , 0U , 0U , PM_REMOVE ) )
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );

            if ( msg.message == WM_QUIT )
                break;
        }

        POINT p_cursor;
        GetCursorPos( &p_cursor );
        io.MousePos.x = ( float ) p_cursor.x;
        io.MousePos.y = ( float ) p_cursor.y;

        io.MouseDown [ 0 ] = ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0;

        ImGui_ImplDX11_NewFrame( );
        ImGui_ImplWin32_NewFrame( );
        ImGui::NewFrame( );
        widgets->notify.handle( );
        {
            static bool menu_open = true;
            static bool insert_pressed_last_frame = false;

            bool insert_down = GetAsyncKeyState( VK_INSERT ) & 0x8000;

            if ( insert_down && !insert_pressed_last_frame )
                menu_open = !menu_open;

            insert_pressed_last_frame = insert_down;

            if ( menu_open )
            {
                slate->draw( g_pd3dDevice , g_pd3dDeviceContext , g_pSwapChain );

            }
        }

        ImFont* active_font = fortnite::render::window_fonts[fortnite::settings::misc::font_selection];
        if (!active_font) active_font = yahei_f;
        ImGui::PushFont( active_font );
        fortnite::visuals::tick( );
        fortnite::aimbot::tick( );
        fortnite::trigger::tick( );
        fortnite::widget::specatator_widget( );
        fortnite::widget::keybind_widget( );
        fortnite::zone::draw_zone_esp( );
        fortnite::radar::tick( );
        //fortnite::misc::tick( );
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList( );

        float fps = ImGui::GetIO( ).Framerate;

        char text [ 256 ];
        sprintf_s( text , "rage | by dx.r.k |fps: %.0f\nrendered players: %d\nvisible players: %d\nplayers within 20m: %d" ,  fps, fortnite::render::rendered_players, fortnite::render::visible_players, fortnite::render::players_within_20m );

        ImVec2 pos = ImVec2( 15 , 15 );

        ImU32 outline = IM_COL32( 0 , 0 , 0 , 255 );
        ImU32 color = IM_COL32( 255 , 255 , 255 , 255 );
        
        draw_list->AddText( ImVec2( pos.x + 1 , pos.y + 1 ) , outline , text );
        draw_list->AddText( ImVec2( pos.x - 1 , pos.y + 1 ) , outline , text );
        draw_list->AddText( ImVec2( pos.x + 1 , pos.y - 1 ) , outline , text );
        draw_list->AddText( ImVec2( pos.x - 1 , pos.y - 1 ) , outline , text );
        draw_list->AddText( pos , color , text );
        if ( fortnite::settings::world::battlemode_toggle )
        {
            const char* battle_text = "BATTLEMODE ACTIVE";
            ImVec2 battle_pos = ImVec2( 15 , 85 );
            ImU32 red = IM_COL32( 255 , 0 , 0 , 255 );
            draw_list->AddText( ImVec2( battle_pos.x + 1 , battle_pos.y + 1 ) , outline , battle_text );
            draw_list->AddText( ImVec2( battle_pos.x - 1 , battle_pos.y + 1 ) , outline , battle_text );
            draw_list->AddText( ImVec2( battle_pos.x + 1 , battle_pos.y - 1 ) , outline , battle_text );
            draw_list->AddText( ImVec2( battle_pos.x - 1 , battle_pos.y - 1 ) , outline , battle_text );
            draw_list->AddText( battle_pos , red , battle_text );
        }

        ImGui::PopFont( );


        ImGui::Render( );

        const float clear_color_with_alpha [ 4 ] =
        {
            clear_color.x * clear_color.w ,
            clear_color.y * clear_color.w ,
            clear_color.z * clear_color.w ,
            clear_color.w
        };

        g_pd3dDeviceContext->OMSetRenderTargets( 1 , &g_mainRenderTargetView , nullptr );
        g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView , clear_color_with_alpha );

        ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );

        static const int fps_limits[] = { 60, 120, 144, 165, 240, 360, 0 };
        int limit = fps_limits[ fortnite::settings::misc::fps_limit % 7 ];
        if ( limit > 0 && !fortnite::settings::misc::vsync ) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto target_time = last_time + std::chrono::microseconds( 1000000 / limit );
            while ( current_time < target_time ) {
                if ( std::chrono::duration_cast<std::chrono::microseconds>(target_time - current_time).count() > 2000 ) {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                } else {
                    std::this_thread::yield( );
                }
                current_time = std::chrono::high_resolution_clock::now();
            }
        }
        last_time = std::chrono::high_resolution_clock::now();

        HRESULT hr = g_pSwapChain->Present( fortnite::settings::misc::vsync ? 1 : 0 , 0 );
        g_SwapChainOccluded = ( hr == DXGI_STATUS_OCCLUDED );
    }
}

void fortnite::render::get_screen( )
{
    width = GetSystemMetrics( SM_CXSCREEN );
    height = GetSystemMetrics( SM_CYSCREEN );

}
