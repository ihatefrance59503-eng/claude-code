#include "render.hpp"
#include "../settings/settings.hpp"
#include <chrono>
#include <thread>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

// ------------------------------------------------------------------
// Overlay target discovery
//
// We paint our ImGui/DX11 output onto a trusted overlay process's HWND,
// never Fortnite's. Priority order:
//   1. Icecream Screen Recorder  — class "Qt5151QWindowIcon", exe recorder.exe
//   2. Discord Overlay           — class "Chrome_Widget_Win_1", title "Discord Overlay"
//
// Qt5151QWindowIcon is Qt5's generic main-window class; any Qt5 app
// uses it. We can't match on class alone or we'll attach to the wrong
// process (e.g. a Qt-based media player). So we EnumWindows and verify
// the owning process is recorder.exe before accepting.
// ------------------------------------------------------------------

namespace {

    bool window_process_matches(HWND hwnd, const char* exe_name) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid) return false;

        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!h) return false;

        char path[MAX_PATH] = {};
        DWORD sz = MAX_PATH;
        BOOL ok = QueryFullProcessImageNameA(h, 0, path, &sz);
        CloseHandle(h);
        if (!ok) return false;

        const char* name = strrchr(path, '\\');
        name = name ? name + 1 : path;
        return _stricmp(name, exe_name) == 0;
    }

    struct overlay_search_ctx {
        const char* target_class;
        const char* target_exe;    // optional; nullptr = don't verify process
        HWND        found;
    };

    BOOL CALLBACK overlay_search_proc(HWND hwnd, LPARAM lparam) {
        auto* ctx = reinterpret_cast<overlay_search_ctx*>(lparam);

        char cls[128] = {};
        if (!GetClassNameA(hwnd, cls, sizeof(cls)))
            return TRUE;

        if (_stricmp(cls, ctx->target_class) != 0)
            return TRUE;

        if (ctx->target_exe && !window_process_matches(hwnd, ctx->target_exe))
            return TRUE;

        // must be visible or we can't piggyback its surface
        if (!IsWindowVisible(hwnd))
            return TRUE;

        ctx->found = hwnd;
        return FALSE;  // stop enumeration
    }

}  // anonymous namespace

HWND fortnite::render::find_window()
{
    // priority 1: Icecream Screen Recorder
    {
        overlay_search_ctx ctx = {
            encrypt("Qt5151QWindowIcon"),
            encrypt("recorder.exe"),
            nullptr
        };
        EnumWindows(overlay_search_proc, reinterpret_cast<LPARAM>(&ctx));
        if (ctx.found) {
            window = ctx.found;
            return window;
        }
    }

    // priority 2: Discord Overlay
    window = FindWindowA(encrypt("Chrome_Widget_Win_1"), encrypt("Discord Overlay"));
    return window;
}

// ------------------------------------------------------------------
// Everything below this line is unchanged from the original render.cpp.
// Preserved verbatim: set_up(), tick(), get_screen().
// ------------------------------------------------------------------

bool fortnite::render::set_up(HWND window)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
            &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 6;
    style.ChildRounding = 10;
    style.PopupRounding = 5;
    yahei_f = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f);
    if (!yahei_f) {
        yahei_f = io.Fonts->AddFontDefault();
    }

    const char* font_paths[20] = {
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\cour.ttf",
        "C:\\Windows\\Fonts\\comic.ttf",
        "C:\\Windows\\Fonts\\impact.ttf",
        "C:\\Windows\\Fonts\\trebuc.ttf",
        "C:\\Windows\\Fonts\\consola.ttf",
        "C:\\Windows\\Fonts\\georgia.ttf",
        "C:\\Windows\\Fonts\\pala.ttf",
        "C:\\Windows\\Fonts\\times.ttf",
        "C:\\Windows\\Fonts\\candara.ttf",
        "C:\\Windows\\Fonts\\corbel.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "C:\\Windows\\Fonts\\cambria.ttf",
        "C:\\Windows\\Fonts\\framd.ttf",
        "C:\\Windows\\Fonts\\micross.ttf",
        "C:\\Windows\\Fonts\\lucon.ttf",
        "C:\\Windows\\Fonts\\constan.ttf"
    };

    for (int i = 0; i < 20; i++) {
        if (GetFileAttributesA(font_paths[i]) != INVALID_FILE_ATTRIBUTES) {
            window_fonts[i] = io.Fonts->AddFontFromFileTTF(font_paths[i], 16.0f);
        }
        if (!window_fonts[i]) {
            window_fonts[i] = yahei_f ? yahei_f : io.Fonts->AddFontDefault();
        }
    }

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    slate->initialize(g_pd3dDevice, g_pd3dDeviceContext, g_pSwapChain);

    return true;
}

void fortnite::render::tick()
{
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 876, 623 };
    static auto last_time = std::chrono::high_resolution_clock::now();
    static ImVec2 window_pos = ImVec2(400, 200);
    widgets->notify.add("starting now", "operations successful", notify_success);

    while (true) {
        ImGuiIO& io = ImGui::GetIO();

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }

        POINT p_cursor;
        GetCursorPos(&p_cursor);
        io.MousePos.x = (float)p_cursor.x;
        io.MousePos.y = (float)p_cursor.y;
        io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        widgets->notify.handle();
        {
            static bool menu_open = true;
            static bool insert_pressed_last_frame = false;
            bool insert_down = GetAsyncKeyState(VK_INSERT) & 0x8000;
            if (insert_down && !insert_pressed_last_frame)
                menu_open = !menu_open;
            insert_pressed_last_frame = insert_down;
            if (menu_open)
                slate->draw(g_pd3dDevice, g_pd3dDeviceContext, g_pSwapChain);
        }

        ImFont* active_font = fortnite::render::window_fonts[fortnite::settings::misc::font_selection];
        if (!active_font) active_font = yahei_f;
        ImGui::PushFont(active_font);
        fortnite::visuals::tick();
        fortnite::aimbot::tick();
        fortnite::trigger::tick();
        fortnite::widget::specatator_widget();
        fortnite::widget::keybind_widget();
        fortnite::zone::draw_zone_esp();
        fortnite::radar::tick();

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        float fps = ImGui::GetIO().Framerate;
        char text[256];
        sprintf_s(text, "rage | by dx.r.k |fps: %.0f\nrendered players: %d\nvisible players: %d\nplayers within 20m: %d",
            fps, fortnite::render::rendered_players, fortnite::render::visible_players, fortnite::render::players_within_20m);

        ImVec2 pos = ImVec2(15, 15);
        ImU32 outline = IM_COL32(0, 0, 0, 255);
        ImU32 color = IM_COL32(255, 255, 255, 255);
        draw_list->AddText(ImVec2(pos.x + 1, pos.y + 1), outline, text);
        draw_list->AddText(ImVec2(pos.x - 1, pos.y + 1), outline, text);
        draw_list->AddText(ImVec2(pos.x + 1, pos.y - 1), outline, text);
        draw_list->AddText(ImVec2(pos.x - 1, pos.y - 1), outline, text);
        draw_list->AddText(pos, color, text);
        if (fortnite::settings::world::battlemode_toggle) {
            const char* battle_text = "BATTLEMODE ACTIVE";
            ImVec2 battle_pos = ImVec2(15, 85);
            ImU32 red = IM_COL32(255, 0, 0, 255);
            draw_list->AddText(ImVec2(battle_pos.x + 1, battle_pos.y + 1), outline, battle_text);
            draw_list->AddText(ImVec2(battle_pos.x - 1, battle_pos.y + 1), outline, battle_text);
            draw_list->AddText(ImVec2(battle_pos.x + 1, battle_pos.y - 1), outline, battle_text);
            draw_list->AddText(ImVec2(battle_pos.x - 1, battle_pos.y - 1), outline, battle_text);
            draw_list->AddText(battle_pos, red, battle_text);
        }

        ImGui::PopFont();
        ImGui::Render();

        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        static const int fps_limits[] = { 60, 120, 144, 165, 240, 360, 0 };
        int limit = fps_limits[fortnite::settings::misc::fps_limit % 7];
        if (limit > 0 && !fortnite::settings::misc::vsync) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto target_time = last_time + std::chrono::microseconds(1000000 / limit);
            while (current_time < target_time) {
                if (std::chrono::duration_cast<std::chrono::microseconds>(target_time - current_time).count() > 2000) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } else {
                    std::this_thread::yield();
                }
                current_time = std::chrono::high_resolution_clock::now();
            }
        }
        last_time = std::chrono::high_resolution_clock::now();

        HRESULT hr = g_pSwapChain->Present(fortnite::settings::misc::vsync ? 1 : 0, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
}

void fortnite::render::get_screen()
{
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
}
