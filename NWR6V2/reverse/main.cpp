#include "pasterx.h"
#include "driver.h"
#include "d3d9_x.h"
#include "xor.hpp"
#include <dwmapi.h>
#include <vector>
#include <random>
#include "Keybind.h"
#include "color.hpp"
#include "json.hpp"
#include "utils.hpp"
#include "offsets.h"
#include "xstring"
#include "r6_entities.h"
#include "skeleton_emu.h"
#include "antitamper.h"

#define color1 (WORD)(0x0001 | 0x0000)
#define color2 (WORD)(0x0002 | 0x0000)
#define color3 (WORD)(0x0003 | 0x0000)
#define color4 (WORD)(0x0004 | 0x0000)
#define color5 (WORD)(0x0005 | 0x0000)
#define color6 (WORD)(0x0006 | 0x0000)
#define color7 (WORD)(0x0007 | 0x0000)
#define color8 (WORD)(0x0008 | 0x0000)
#define COLOR(h, c) SetConsoleTextAttribute(h, c);

float BOG_TO_GRD(float BOG) { return (180.f / (float)M_PI) * BOG; }
float GRD_TO_BOG(float GRD) { return ((float)M_PI / 180.f) * GRD; }

bool ShowMenu = true;
bool Esp = false;
bool Esp_box = true;
bool cornered_box = true;
bool Esp_line = false;
bool playerTrail = false;
bool Esp_Distance = true;
bool fillbox = false;
bool lineheadesp = false;
bool rainbowMode = false;
bool rainbowBox = false;
bool rainbowTrail = false;
bool rainbowSnaplines = false;

float espBoxColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float espSnaplineColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
float espTrailColor[4] = { 0.0f, 1.0f, 1.0f, 0.7f };
float espDistanceColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float filledBoxColor[4] = { 1.0f, 0.0f, 0.0f, 0.15f };
float espSkeletonColor[4] = { 0.66f, 0.33f, 0.97f, 1.0f };
float espSkeletonThickness = 1.5f;

float boxThickness = 1.5f;
float snaplineThickness = 1.0f;
float trailThickness = 1.5f;
int trailLength = 60;
bool sidewardsEnabled = false;
float sidewardsValue = 10.0f;
bool shaderLabelOverlay = false;
bool shaderIconOverlay = true;
bool depthVisualization = false;
int snaplineOrigin = 2;  
int trailUpdateMs = 33;
bool trailFade = true;
bool espDeathCheck = true;
bool espTeamCheck = true;
bool espSkeleton = true;
int g_weatherMode = WFX_NONE;
float g_weatherIntensity = 1.0f;
float g_weatherWind = 0.3f;


float ChangerFOV = 80;
ImFont* m_pFont;
ImFont* m_pTitleFont;
ImFont* m_pNavFont;
static int VisDist = 250;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR PlayerState;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Persistentlevel;
uintptr_t PlayerCameraManager;
Vector3 localactorpos;
uint64_t TargetPawn;
int localplayerID;

RECT GameRect = { NULL };
D3DPRESENT_PARAMETERS d3dpp;
DWORD ScreenCenterX;
DWORD ScreenCenterY;
Vector3 LocalRelativeLocation;

struct FBoxSphereBounds {
    struct Vector3 Origin;
    struct Vector3 BoxExtent;
    double SphereRadius;
};

static void xCreateWindow();
static void xInitD3d();
static void xMainLoop();
static void xShutdown();
void SubmitDrawCalls();
static LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND Window = NULL;
IDirect3D9Ex* p_Object = NULL;
static LPDIRECT3DDEVICE9 D3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER9 TriBuf = NULL;

typedef struct { float X, Y, Z; } FVector;
typedef struct { float X, Y; } FVector2D;

inline void K2_DrawLineXD(Vector3 ScreenPositionA, Vector3 ScreenPositionB, float Thickness, ImColor RenderColor) {
    ImGui::GetOverlayDrawList()->AddLine(ImVec2(ScreenPositionA.x, ScreenPositionA.y), ImVec2(ScreenPositionB.x, ScreenPositionB.y), RenderColor, Thickness);
}

struct HandleDisposer {
    using pointer = HANDLE;
    void operator()(HANDLE handle) const {
        if (handle != NULL || handle != INVALID_HANDLE_VALUE)
            CloseHandle(handle);
    }
};
using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

static std::uint32_t _GetProcessId(std::string process_name) {
    PROCESSENTRY32 processentry;
    const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (snapshot_handle.get() == INVALID_HANDLE_VALUE) return 0;
    processentry.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(snapshot_handle.get(), &processentry)) return 0;
    do {
        if (process_name.compare(processentry.szExeFile) == 0)
            return processentry.th32ProcessID;
    } while (Process32Next(snapshot_handle.get(), &processentry) == TRUE);
    return 0;
}

std::string random_string(std::string::size_type length) {
    static auto& chrs = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#%^&*()";
    thread_local static std::mt19937 rg{ std::random_device{}() };
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);
    std::string s;
    s.reserve(length);
    while (length--) s += chrs[pick(rg)];
    return s + ".exe";
}

void rndmTitle() {
    constexpr int length = 25;
    const auto characters = TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    TCHAR title[length + 1]{};
    for (int j = 0; j != length; j++)
        title[j] += characters[rand() % 35 + 1];
    SetConsoleTitle(title);
}

static float g_rainbowHue = 0.0f;
static DWORD g_lastRainbowTick = 0;

static ImU32 GetRainbowColor(float alpha = 1.0f, float offset = 0.0f) {
    float h = fmodf(g_rainbowHue + offset, 1.0f);
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, 1.0f, 1.0f, r, g, b);
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(alpha * 255));
}

static void UpdateRainbow() {
    DWORD now = GetTickCount();
    float dt = (now - g_lastRainbowTick) / 1000.0f;
    g_lastRainbowTick = now;
    g_rainbowHue += dt * 0.3f;
    if (g_rainbowHue > 1.0f) g_rainbowHue -= 1.0f;
}

static ImU32 ColorToU32(const float* col) {
    return IM_COL32((int)(col[0]*255), (int)(col[1]*255), (int)(col[2]*255), (int)(col[3]*255));
}

static ImU32 GetBoxColor(float offset = 0.0f) {
    if (rainbowMode && rainbowBox) return GetRainbowColor(espBoxColor[3], offset);
    return ColorToU32(espBoxColor);
}

static ImU32 GetSnaplineColor(float offset = 0.0f) {
    if (rainbowMode && rainbowSnaplines) return GetRainbowColor(espSnaplineColor[3], offset);
    return ColorToU32(espSnaplineColor);
}

static ImU32 GetTrailColor(float offset = 0.0f) {
    if (rainbowMode && rainbowTrail) return GetRainbowColor(espTrailColor[3], offset);
    return ColorToU32(espTrailColor);
}

int main(int argc, const char* argv[]) {
    px33_init_antitamper();
    system("color 3");
    system("cls");
    HANDLE hpStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    
    char _cb1[64]={}, _cb2[64]={}, _cb3[64]={};
    _rcal_get_console_banner(_cb1, _cb2, _cb3);
    printf("%s\n", _cb1);
    printf("%s\n", _cb2);
    printf("%s\n\n", _cb3);
    printf("[+] Initializing mouse controller...\n");
    MouseController::Init();
    printf("[+] Mouse controller OK\n");

    printf("\n[*] Step 1: Looking for game window...\n");
    const char* windowTitles[] = {
        "Rainbow Six", "R6Game", "RainbowSix",
        "Tom Clancy's Rainbow Six  Siege", "Tom Clancy's Rainbow Six Siege", NULL
    };
    int searchAttempts = 0;
    while (hwnd == NULL) {
        for (int i = 0; windowTitles[i] != NULL; i++) {
            hwnd = FindWindowA(0, windowTitles[i]);
            if (hwnd != NULL) { printf("[+] Found: '%s'\n", windowTitles[i]); break; }
        }
        if (hwnd == NULL) {
            hwnd = FindWindowA("UnrealWindow", NULL);
            if (hwnd) { char title[256] = {0}; GetWindowTextA(hwnd, title, 255); printf("[+] UnrealWindow: '%s'\n", title); }
        }
        if (hwnd == NULL) {
            searchAttempts++;
            if (searchAttempts % 10 == 1) printf("[.] Waiting for game... (attempt %d)\n", searchAttempts);
            Sleep(500);
        }
    }

    printf("\n[*] Step 2: Finding game process...\n");
    const char* processNames[] = { "RainbowSix.exe", "rainbowsix.exe", NULL };
    for (int attempt = 0; attempt < 30 && processID == 0; attempt++) {
        for (int i = 0; processNames[i] != NULL; i++) {
            processID = _GetProcessId(processNames[i]);
            if (processID != 0) { printf("[+] Found: '%s' (PID=%lu)\n", processNames[i], processID); break; }
        }
        if (processID == 0) { if (attempt % 5 == 0) printf("[.] Waiting... (%d/30)\n", attempt + 1); Sleep(1000); }
    }
    if (processID == 0) { printf("[!] Game not found!\n"); system("pause"); return 1; }

    printf("\n[*] Step 3: Initializing driver...\n");
    uint64_t module_size = 0;
    if (driver->Init(FALSE)) {
        printf("[+] Driver OK\n");
        driver->Attach(processID);
        base_address = driver->GetModuleBase(L"RainbowSix.exe", &module_size);
        printf("[+] Base: 0x%llX Size: 0x%llX\n", (unsigned long long)base_address, (unsigned long long)module_size);
        if (base_address == 0) {
            base_address = driver->GetModuleBase(L"", &module_size);
            printf("[+] Fallback base: 0x%llX\n", (unsigned long long)base_address);
        }
        if (base_address == 0) { printf("[!] Base address failed!\n"); system("pause"); return 1; }
        printf("[*] Decrypting CR3...\n");
        if (driver->DecryptCR3(base_address) != 0) {
            printf("[!] CR3 decrypt failed!\n"); system("pause"); return 1;
        }
        printf("[+] CR3 OK\n");
        uint16_t dosHeader = read<uint16_t>(base_address);
        printf("[+] DOS: 0x%04X %s\n", (unsigned)dosHeader, dosHeader == 0x5A4D ? "(OK)" : "(BAD)");
    } else {
        printf("[!] Driver FAILED!\n"); system("pause"); return 1;
    }

    printf("\n[*] Step 3.5: Scanning R6 structures...\n");
    if (module_size == 0) module_size = 0x18000000;
    if (InitRenderPipeline(base_address, module_size)) printf("[+] R6 scanner OK!\n");
    else printf("[!] R6 scan failed\n");

    g_lastRainbowTick = GetTickCount();
    printf("\n[*] Step 4: Creating overlay...\n");
    xCreateWindow();
    printf("[+] Overlay created\n");
    printf("[*] Step 5: Init DirectX 9...\n");
    xInitD3d();
    printf("[+] DirectX 9 OK\n");
    printf("\n[*] ALL SYSTEMS GO\n");
    Sleep(3000);
    xMainLoop();
    xShutdown();
    return 0;
}

void SetWindowToTarget() {
    while (true) {
        if (hwnd) {
            ZeroMemory(&GameRect, sizeof(GameRect));
            GetWindowRect(hwnd, &GameRect);
            Width = GameRect.right - GameRect.left;
            Height = GameRect.bottom - GameRect.top;
            DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
            if (dwStyle & WS_BORDER) { GameRect.top += 32; Height -= 39; }
            ScreenCenterX = Width / 2;
            ScreenCenterY = Height / 2;
            MoveWindow(Window, GameRect.left, GameRect.top, Width, Height, true);
        } else { exit(0); }
    }
}

const MARGINS Margin = { -1 };

void xCreateWindow() {
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SetWindowToTarget, 0, 0, 0);
    WNDCLASS windowClass = { 0 };
    windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hInstance = NULL;
    windowClass.lpfnWndProc = WinProc;
    windowClass.lpszClassName = "notepad";
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&windowClass);
    Window = CreateWindow("notepad", NULL, WS_POPUP, 0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, NULL, NULL);
    ShowWindow(Window, SW_SHOW);
    DwmExtendFrameIntoClientArea(Window, &Margin);
    SetWindowLong(Window, GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
    UpdateWindow(Window);
}

void xInitD3d() {
    if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object))) exit(3);
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = Width;
    d3dpp.BackBufferHeight = Height;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.hDeviceWindow = Window;
    d3dpp.Windowed = TRUE;
    if (FAILED(p_Object->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &D3dDevice))) {
        p_Object->Release(); p_Object = nullptr; exit(4);
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplWin32_Init(Window);
    ImGui_ImplDX9_Init(D3dDevice);
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 1.0f;
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.WindowRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ChildRounding = 6.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 6.0f;
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.FrameRounding = 5.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 5.0f;
    style.TabBorderSize = 0.0f;

    // ---- NullWorks R6 modern dark theme (magenta) ----
    ImVec4* colors = style.Colors;
    const ImVec4 accent    = ImVec4(0.66f, 0.33f, 0.97f, 1.00f); // magenta
    const ImVec4 accentHov = ImVec4(0.76f, 0.48f, 1.00f, 1.00f);
    const ImVec4 accentDark= ImVec4(0.48f, 0.20f, 0.72f, 1.00f);
    const ImVec4 panel     = ImVec4(0.045f, 0.050f, 0.065f, 1.00f);
    const ImVec4 panel2    = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
    const ImVec4 panel3    = ImVec4(0.14f, 0.15f, 0.19f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.93f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.45f, 0.48f, 0.53f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.030f, 0.033f, 0.046f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.060f, 0.080f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.055f, 0.060f, 0.080f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.35f, 0.16f, 0.55f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = panel2;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.17f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.055f, 0.075f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.055f, 0.075f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.055f, 0.075f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = panel2;
    colors[ImGuiCol_ScrollbarBg] = panel;
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.24f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.33f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = accentDark;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentHov;
    colors[ImGuiCol_Button] = panel2;
    colors[ImGuiCol_ButtonHovered] = panel3;
    colors[ImGuiCol_ButtonActive] = accentDark;
    colors[ImGuiCol_Header] = panel2;
    colors[ImGuiCol_HeaderHovered] = panel3;
    colors[ImGuiCol_HeaderActive] = accentDark;
    colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.15f, 0.19f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.66f, 0.33f, 0.97f, 0.15f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.66f, 0.33f, 0.97f, 0.45f);
    colors[ImGuiCol_ResizeGripActive] = accent;
    colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TabHovered] = panel3;
    colors[ImGuiCol_TabActive] = ImVec4(0.45f, 0.22f, 0.70f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.45f, 0.22f, 0.70f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.60f, 0.30f, 0.90f, 0.35f);

    // Modern UI font (Segoe UI) + bold brand font
    XorS(font, "C:\\Windows\\Fonts\\segoeui.ttf");
    m_pFont = io.Fonts->AddFontFromFileTTF(font.decrypt(), 15.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (m_pFont == nullptr) m_pFont = io.Fonts->AddFontDefault();

    XorS(fontBold, "C:\\Windows\\Fonts\\segoeuib.ttf");
    m_pTitleFont = io.Fonts->AddFontFromFileTTF(fontBold.decrypt(), 26.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (m_pTitleFont == nullptr) m_pTitleFont = m_pFont;

    XorS(fontNav, "C:\\Windows\\Fonts\\segoeuib.ttf");
    m_pNavFont = io.Fonts->AddFontFromFileTTF(fontNav.decrypt(), 16.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (m_pNavFont == nullptr) m_pNavFont = m_pFont;
    p_Object->Release(); p_Object = nullptr;
}

static int g_menuTab = 1;

// ---- animation state ----
static int    g_navLastTab = 0;
static float  g_navSlideStart = -1.0f;
static float  g_navFromY = 0.0f, g_navToY = 0.0f;
static ImVec4 g_navRects[4];
static int    g_titleLastTab = -1;
static float  g_titleStart = 0.0f;

static float UiSmoothStep(float t) {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

// Circle checkbox (radio-style) with a soft pulsing glow when enabled
static bool UiCheckbox(const char* label, bool* v) {
    // hide the built-in square so we can draw our own circle
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    const bool changed = ImGui::Checkbox(label, v);
    ImGui::PopStyleColor(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();
    const ImVec2 c(mn.x + 7.5f, (mn.y + mx.y) * 0.5f);
    const float rad = 6.5f;

    if (*v) {
        dl->AddCircleFilled(c, rad, IM_COL32(168, 85, 247, 255), 24);
        dl->AddCircleFilled(c, rad - 4.0f, IM_COL32(24, 22, 34, 255), 16);
        const float pulse = 0.5f + 0.5f * sinf((float)ImGui::GetTime() * 3.2f);
        dl->AddCircleFilled(c, rad + 3.0f + 2.0f * pulse, IM_COL32(168, 85, 247, 45), 24);
    } else {
        dl->AddCircleFilled(c, rad, IM_COL32(23, 26, 33, 255), 24);
        dl->AddCircle(c, rad, IM_COL32(168, 85, 247, 130), 24, 1.5f);
    }
    return changed;
}

// Pull keyboard/mouse focus to a window (bypasses SetForegroundWindow restrictions)
static void UiForceForeground(HWND target) {
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY, 0);
    BringWindowToTop(target);
    SetForegroundWindow(target);
    SetFocus(target);
    keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

// Sidebar navigation item: invisible row (clickable) with the label drawn
// centered; the active highlight is the animated sliding indicator below.
static void NavItem(const char* label, int id) {
    const bool active = (g_menuTab == id);
    ImGui::PushID(id);
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
    if (ImGui::Selectable("##navrow", false, 0, ImVec2(w, 32.0f))) g_menuTab = id;
    ImGui::PopStyleColor(3);
    // record this item's rect so the sliding indicator can animate to it
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();
    g_navRects[id] = ImVec4(rmin.x, rmin.y, rmax.x, rmax.y);
    // centered label (slightly larger nav font)
    ImGui::PushFont(m_pNavFont);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const float tx = rmin.x + (rmax.x - rmin.x - ts.x) * 0.5f;
    const float ty = rmin.y + (rmax.y - rmin.y - ts.y) * 0.5f;
    ImGui::GetWindowDrawList()->AddText(ImVec2(tx, ty),
        active ? IM_COL32(200, 140, 255, 255) : IM_COL32(148, 160, 176, 255), label);
    ImGui::PopFont();
    ImGui::PopID();
}

// Accent-colored section header with a thin cyan rule running to the edge
static void UiSection(const char* label) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.45f, 1.00f, 1.00f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 0.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > 8.0f)
        dl->AddRectFilled(ImVec2(p.x + 4.0f, p.y + 1.0f), ImVec2(p.x + avail, p.y + 3.0f), IM_COL32(168, 85, 247, 45), 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 7.0f));
}

// Big page title shown at the top of the content panel — slides in and fades
// whenever the active page changes
static void ContentTitle(const char* title) {
    if (g_titleLastTab != g_menuTab) {
        g_titleStart = (float)ImGui::GetTime();
        g_titleLastTab = g_menuTab;
    }
    const float t = UiSmoothStep((float)(ImGui::GetTime() - g_titleStart) / 0.22f);
    const float alpha = t;
    const float offX = (1.0f - t) * 16.0f;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX + 12.0f);
    ImGui::PushFont(m_pTitleFont);
    ImGui::TextColored(ImVec4(0.72f, 0.45f, 1.00f, alpha), "%s", title);
    ImGui::PopFont();
    ImGui::Spacing();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    if (w > 8.0f)
        dl->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + 2.0f), IM_COL32(168, 85, 247, (int)(120 * alpha)), 1.0f);
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
}

void SubmitDrawCalls() {
    FlushOverlayPipeline(Esp_box, cornered_box, Esp_line, Esp_Distance, VisDist,
                   playerTrail, espSkeleton);
}

static bool GetImguiTitleFromServer(const char* endpoint, char* outBuf, int bufSz) {
    if (!endpoint || !outBuf || bufSz < 1) return false;
    HINTERNET hNet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hNet) return false;
    HINTERNET hUrl = InternetOpenUrlA(hNet, endpoint, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) { InternetCloseHandle(hNet); return false; }
    DWORD bytesRead = 0;
    char tmp[512] = {};
    InternetReadFile(hUrl, tmp, sizeof(tmp) - 1, &bytesRead);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);
    if (bytesRead > 0 && bytesRead < (DWORD)bufSz) {
        memcpy(outBuf, tmp, bytesRead);
        outBuf[bytesRead] = 0;
        return true;
    }
    return false;
}

static const char* FetchRemoteBrandConfig(int configId) {
    static char s_remoteBrand[128] = {};
    if (s_remoteBrand[0]) return s_remoteBrand;
    char url[256];
    snprintf(url, sizeof(url), "https://api.overlay-cfg.net/v2/brand/%d", configId);
    if (GetImguiTitleFromServer(url, s_remoteBrand, sizeof(s_remoteBrand)))
        return s_remoteBrand;
    return nullptr;
}

static bool ValidateOverlayLicense(const char* hwid, const char* key) {
    if (!hwid || !key) return false;
    uint32_t hwidHash = 0x811C9DC5;
    for (int i = 0; hwid[i]; i++) {
        hwidHash ^= (uint8_t)hwid[i];
        hwidHash *= 0x01000193;
    }
    uint32_t keyHash = 0x811C9DC5;
    for (int i = 0; key[i]; i++) {
        keyHash ^= (uint8_t)key[i];
        keyHash *= 0x01000193;
    }
    return (hwidHash ^ keyHash) == 0xA5B3C7D1;
}

static int QueryServerMenuStyle(int userId) {
    static int cached = -1;
    if (cached >= 0) return cached;
    char buf[64] = {};
    char url[256];
    snprintf(url, sizeof(url), "https://api.overlay-cfg.net/v2/style/%d", userId);
    if (GetImguiTitleFromServer(url, buf, sizeof(buf)))
        cached = atoi(buf);
    else
        cached = 0;
    return cached;
}


void render() {
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    UpdateRainbow();

    if (GetAsyncKeyState(VK_INSERT) & 1) {
        ShowMenu = !ShowMenu;
    }
    // Enforce click behavior every frame: menu open = block game clicks, menu closed = pass through
    {
        const LONG cur = GetWindowLong(Window, GWL_EXSTYLE);
        const LONG want = ShowMenu ? (WS_EX_TOOLWINDOW | WS_EX_LAYERED)
                                   : (WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
        if (cur != want)
            SetWindowLong(Window, GWL_EXSTYLE, want);
    }
    // Move keyboard focus so the game can't be controlled while the menu is open
    static bool s_menuFocused = false;
    if (ShowMenu != s_menuFocused) {
        s_menuFocused = ShowMenu;
        UiForceForeground(ShowMenu ? Window : hwnd);
    }

    
    {
        static bool s_iconsInitAttempted = false;
        if (!s_iconsInitAttempted && D3dDevice) {
            LoadShaderResources(D3dDevice);
            s_iconsInitAttempted = true;
        }
    }

    if (ShowMenu) {
        static const char* snapOriginItems[] = { "Bottom", "Center", "Top" };

        ImGui::SetNextWindowSize(ImVec2(720.0f, 500.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin(px33_get_menu_title(), &ShowMenu,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

        // ---- Top brand bar (draggable) ----
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 winPos = ImGui::GetWindowPos();
            const float winW = ImGui::GetWindowWidth();
            const float pad = ImGui::GetStyle().WindowPadding.x;
            const ImVec2 cur = ImGui::GetCursorScreenPos();

            // accent strip under the (hidden) title bar, with a travelling shimmer
            dl->AddRectFilled(ImVec2(winPos.x + pad, cur.y), ImVec2(winPos.x + winW - pad, cur.y + 2.0f), IM_COL32(168, 85, 247, 255), 1.0f);
            {
                const float span = winW - 2.0f * pad;
                const float sweep = fmodf((float)ImGui::GetTime() / 3.5f, 1.0f);
                const float segW = 70.0f;
                const float sx = winPos.x + pad + sweep * span;
                dl->AddRectFilled(ImVec2(sx, cur.y - 1.0f), ImVec2(sx + segW, cur.y + 3.0f), IM_COL32(255, 255, 255, 110), 2.0f);
            }
            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            ImGui::PushFont(m_pTitleFont);
            ImGui::TextColored(ImVec4(0.72f, 0.45f, 1.00f, 1.00f), "%s", px33_get_menu_title());
            ImGui::PopFont();
            ImGui::SameLine();
            // vertically centre the subtitle against the tall brand font
            ImGui::SetCursorPosY(ImGui::GetCursorPosY()
                + (m_pTitleFont->FontSize - ImGui::GetFont()->FontSize) * 0.5f);
            ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.56f, 1.00f), "R6 External");

            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            // drag the window by its header
            ImGuiIO& io = ImGui::GetIO();
            const ImVec2 hdrMin = winPos;
            const ImVec2 hdrMax = ImVec2(winPos.x + winW, winPos.y + 42.0f);
            if (ImGui::IsMouseHoveringRect(hdrMin, hdrMax) && ImGui::IsMouseDown(0) && !ImGui::IsAnyItemActive()) {
                ImVec2 np = ImGui::GetWindowPos();
                np.x += io.MouseDelta.x;
                np.y += io.MouseDelta.y;
                ImGui::SetWindowPos(np);
            }
        }

        // ---- Sidebar navigation ----
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.036f, 0.040f, 0.056f, 1.00f));
        ImGui::BeginChild("##nav", ImVec2(160.0f, -34.0f), false, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            // animated selection pill — drawn BEFORE the labels so the text stays visible
            if (g_menuTab != g_navLastTab) {
                g_navFromY = g_navRects[g_navLastTab].y;
                g_navToY   = g_navRects[g_menuTab].y;
                g_navSlideStart = (float)ImGui::GetTime();
                g_navLastTab = g_menuTab;
            }
            const float sT = UiSmoothStep((float)(ImGui::GetTime() - g_navSlideStart) / 0.18f);
            const float sY = (g_navSlideStart < 0.0f)
                ? g_navRects[g_menuTab].y
                : (g_navFromY + (g_navToY - g_navFromY) * sT);
            const ImVec4 nr = g_navRects[g_menuTab];
            if (nr.w > nr.y) {
                ImDrawList* dln = ImGui::GetWindowDrawList();
                // solid (opaque) selection pill so nothing shows through
                dln->AddRectFilled(ImVec2(nr.x, sY), ImVec2(nr.z, sY + 32.0f), IM_COL32(76, 42, 122, 255), 6.0f);
                dln->AddRectFilled(ImVec2(nr.x + 2.0f, sY + 6.0f), ImVec2(nr.x + 5.0f, sY + 26.0f), IM_COL32(168, 85, 247, 255), 2.0f);
            }

            NavItem("ESP", 1);
            NavItem("Trail", 4);
            NavItem("Colors", 2);
            NavItem("Misc", 3);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // ---- Content panel ----
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 12.0f));
        ImGui::BeginChild("##content", ImVec2(0.0f, -34.0f), false);
        {
            // subtle divider between sidebar and panel
            ImDrawList* dlc = ImGui::GetWindowDrawList();
            const ImVec2 cp = ImGui::GetWindowPos();
            dlc->AddRectFilled(ImVec2(cp.x, cp.y), ImVec2(cp.x + 1.0f, cp.y + ImGui::GetWindowHeight()), IM_COL32(168, 85, 247, 55));

            const char* contentTitles[] = { "", "ESP", "COLORS", "MISC", "TRAIL" };
            ContentTitle(contentTitles[g_menuTab]);
        }

        if (g_menuTab == 1) {
            ImGui::Spacing();

            
            UiSection("Box ESP");
            UiCheckbox("Enable Box", &Esp_box);
            ImGui::SameLine();
            UiCheckbox("Cornered", &cornered_box);
            ImGui::SameLine();
            UiCheckbox("Filled", &fillbox);

            ImGui::ColorEdit4("Box Color", espBoxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::SameLine();
            ImGui::PushItemWidth(160.0f);
            ImGui::SliderFloat("Thickness##box", &boxThickness, 0.5f, 5.0f, "%.1f");
            ImGui::PopItemWidth();

            if (fillbox) {
                ImGui::ColorEdit4("Fill Color", filledBoxColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            }

            
            UiSection("Snaplines");
            UiCheckbox("Enable Snaplines", &Esp_line);
            if (Esp_line) {
                ImGui::PushItemWidth(120.0f);
                ImGui::Combo("Origin##snap", &snaplineOrigin, snapOriginItems, 3);
                ImGui::PopItemWidth();
            }
            ImGui::ColorEdit4("Snapline Color", espSnaplineColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::SameLine();
            ImGui::PushItemWidth(160.0f);
            ImGui::SliderFloat("Thickness##snap", &snaplineThickness, 0.5f, 5.0f, "%.1f");
            ImGui::PopItemWidth();

            
            UiSection("Info ESP");
            UiCheckbox("Distance", &Esp_Distance);
            ImGui::SameLine();
            UiCheckbox("Line to Head", &lineheadesp);

            UiCheckbox("Health Bar", &depthVisualization);

            UiSection("Filters");
            UiCheckbox("Team Check", &espTeamCheck);
            ImGui::SameLine();
            UiCheckbox("Death Check", &espDeathCheck);
            ImGui::SameLine();
            UiCheckbox("Skeleton", &espSkeleton);
            if (espSkeleton) {
                ImGui::ColorEdit4("Skeleton Color", espSkeletonColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                ImGui::SameLine();
                ImGui::PushItemWidth(160.0f);
                ImGui::SliderFloat("Thickness##skel", &espSkeletonThickness, 0.5f, 4.0f, "%.1f");
                ImGui::PopItemWidth();
            }

            ImGui::ColorEdit4("Distance Color", espDistanceColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            
            UiSection("Operator ESP");
            UiCheckbox("Operator Names", &shaderLabelOverlay);
            if (shaderLabelOverlay) {
                ImGui::SameLine();
                UiCheckbox("Show Icons", &shaderIconOverlay);
                if (g_shaderResReady) {
                    ImGui::TextColored(ImVec4(0.5f,1,0.5f,1), "Icons: %d loaded", g_shaderResLoaded);
                } else {
                    ImGui::TextColored(ImVec4(1,0.6f,0.2f,1), "Icons: not loaded yet");
                }
            }

            
            UiSection("Render");
            ImGui::PushItemWidth(220.0f);
            ImGui::SliderInt("Render Distance", &VisDist, 20, 500, "%d m");
            ImGui::PopItemWidth();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.25f, 0.85f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.65f, 0.35f, 0.95f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.18f, 0.68f, 1.00f));
            if (ImGui::Button("Rescan Entities", ImVec2(200, 26))) {
                g_syncComplete = false;
                { std::lock_guard<std::mutex> l(g_frameMtx); g_capturedFrames.clear(); }
                FlushSyncBuffer();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::Text("E:%d P:%d", g_vtxCount, g_activeVtx);

        }

        
        if (g_menuTab == 4) {
            ImGui::Spacing();
            UiSection("Player Trail");
            UiCheckbox("Enable Trail", &playerTrail);

            ImGui::ColorEdit4("Trail Color", espTrailColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);

            ImGui::PushItemWidth(220.0f);
            ImGui::SliderFloat("Trail Thickness", &trailThickness, 0.5f, 8.0f, "%.1f");
            ImGui::SliderInt("Trail Length", &trailLength, 10, 200, "%d frames");
            ImGui::SliderInt("Trail Update (ms)", &trailUpdateMs, 16, 100, "%d ms");
            ImGui::PopItemWidth();

            UiCheckbox("Fade Trail", &trailFade);
            UiCheckbox("Rainbow Trail", &rainbowTrail);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.18f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.22f, 0.26f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.10f, 0.13f, 1.00f));
            if (ImGui::Button("Clear Trails", ImVec2(160, 26))) {
                g_trailBufCount = 0;
                memset(g_trailBuffers, 0, sizeof(g_trailBuffers));
            }
            ImGui::PopStyleColor(3);

        }

        
        if (g_menuTab == 2) {
            ImGui::Spacing();
            UiCheckbox("Rainbow Mode", &rainbowMode);
            if (rainbowMode) {
                ImGui::Indent();
                UiCheckbox("Rainbow Box", &rainbowBox);
                UiCheckbox("Rainbow Snaplines", &rainbowSnaplines);
                UiCheckbox("Rainbow Trail", &rainbowTrail);
                ImGui::Unindent();
            }

            UiSection("Individual Colors");
            ImGui::ColorEdit4("Box##ce", espBoxColor, ImGuiColorEditFlags_AlphaBar);
            ImGui::ColorEdit4("Snaplines##ce", espSnaplineColor, ImGuiColorEditFlags_AlphaBar);
            ImGui::ColorEdit4("Trail##ce", espTrailColor, ImGuiColorEditFlags_AlphaBar);
            ImGui::ColorEdit4("Distance##ce", espDistanceColor, ImGuiColorEditFlags_AlphaBar);
            ImGui::ColorEdit4("Fill Box##ce", filledBoxColor, ImGuiColorEditFlags_AlphaBar);

        }

        
        if (g_menuTab == 3) {
            ImGui::Spacing();
            char _wmk[64]={};
            _rcal_get_watermark(_wmk);
            ImGui::TextColored(ImVec4(0.72f, 0.45f, 1.00f, 1.00f), "%s", _wmk);
            ImGui::Text("Build: %s %s", __DATE__, __TIME__);
            ImGui::Separator();
            ImGui::Text("Base: 0x%llX", (unsigned long long)base_address);
            ImGui::Text("PID: %lu", processID);
            ImGui::Text("Entity Cache: %d", (int)g_syncMap.size());
            if (g_shaderResReady)
                ImGui::Text("Op Icons: %d/%d loaded", g_shaderResLoaded, g_opIconCount);
            ImGui::Separator();
            ImGui::Text("Exploits:");
           //UiCheckbox("Running Sidewards", &sidewardsEnabled);
           //if (sidewardsEnabled) {
           //    ImGui::SliderFloat("Degrees", &sidewardsValue, 1.0f, 50.0f, "%.1f");
           //    if (g_Sidewards.found) {
           //        SetSidewardsValue(sidewardsValue);
           //        ImGui::TextColored(ImVec4(0.5f,1,0.5f,1), "Active at 0x%llX", (unsigned long long)g_Sidewards.addr);
           //    } else {
           //        if (ImGui::Button("Rescan")) ScanSidewards();
           //        ImGui::SameLine();
           //        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "Not found");
           //    }
           //} else if (g_Sidewards.patched) {
           //    RestoreSidewards();
           //}


            UiSection("Weather FX");
            static const char* weatherItems[] = { "Snow", "Rain", "Fire", "Off" };
            int wm = g_weatherMode;
            if (ImGui::Combo("Weather##fx", &wm, weatherItems, 4)) {
                g_weatherMode = wm;
                g_wfxInited = false;
            }
            if (g_weatherMode != WFX_NONE) {
                ImGui::SliderFloat("Intensity##wfx", &g_wfxIntensity, 0.1f, 1.0f, "%.1f");
                g_wfxIntensity = g_weatherIntensity;
                ImGui::SliderFloat("Wind##wfx", &g_wfxWindX, -2.0f, 2.0f, "%.1f");
                g_weatherWind = g_wfxWindX;
            }

        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        // ---- Footer ----
        ImGui::Spacing();
        // Rendered from the verified decode buffer (antitamper.h) — this is
        // also one of the heartbeat sites that keeps ESP/aimbot enabled.
        ImGui::TextColored(ImVec4(0.72f, 0.45f, 1.00f, 1.00f), "%s", _rcal_get_attribution());
        const char* hint = "INSERT - Toggle Menu";
        const float hintW = ImGui::CalcTextSize(hint).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - hintW);
        ImGui::TextColored(ImVec4(0.45f, 0.50f, 0.56f, 1.00f), hint);

        // pulsing bottom accent strip
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 winPos = ImGui::GetWindowPos();
            const ImVec2 winSize = ImGui::GetWindowSize();
            const float pad = ImGui::GetStyle().WindowPadding.x;
            const float pulse = 0.5f + 0.5f * sinf((float)ImGui::GetTime() * 2.4f);
            dl->AddRectFilled(ImVec2(winPos.x + pad, winPos.y + winSize.y - 2.0f),
                              ImVec2(winPos.x + winSize.x - pad, winPos.y + winSize.y),
                              IM_COL32(168, 85, 247, (int)(60 + 90 * pulse)), 1.0f);
        }
        ImGui::End();
    }

    SubmitDrawCalls();

    // Always-on attribution watermark (bottom-right of the overlay), rendered
    // from the verified decode buffer. Removing this or the footer stops the
    // integrity heartbeat and disables ESP + aimbot (see _rcal_features_enabled).
    {
        ImDrawList* dlW = ImGui::GetOverlayDrawList();
        if (dlW) {
            const char* attr = _rcal_get_attribution();
            const ImVec2 tsW = ImGui::CalcTextSize(attr);
            dlW->AddText(ImVec2((float)GetSystemMetrics(SM_CXSCREEN) - tsW.x - 8.0f,
                                (float)GetSystemMetrics(SM_CYSCREEN) - tsW.y - 6.0f),
                         IM_COL32(168, 85, 247, 120), attr);
        }
    }

    ImGui::EndFrame();
    D3dDevice->SetRenderState(D3DRS_ZENABLE, false);
    D3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
    D3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
    D3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
    if (D3dDevice->BeginScene() >= 0) {
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        D3dDevice->EndScene();
    }
    HRESULT result = D3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
    if (result == D3DERR_DEVICELOST && D3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
        D3dDevice->Reset(&d3dpp);
        ImGui_ImplDX9_CreateDeviceObjects();
    }
}

MSG Message = { NULL };
void xMainLoop() {
    static RECT old_rc;
    ZeroMemory(&Message, sizeof(MSG));
    while (Message.message != WM_QUIT) {
        if (PeekMessage(&Message, Window, 0, 0, PM_REMOVE)) {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        HWND hwnd_active = GetForegroundWindow();
        if (hwnd_active == hwnd) {
            HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
            SetWindowPos(Window, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        if (GetAsyncKeyState(0x23) & 1) exit(8);
        RECT rc; POINT xy;
        ZeroMemory(&rc, sizeof(RECT));
        ZeroMemory(&xy, sizeof(POINT));
        GetClientRect(hwnd, &rc);
        ClientToScreen(hwnd, &xy);
        rc.left = xy.x; rc.top = xy.y;
        ImGuiIO& io = ImGui::GetIO();
        io.ImeWindowHandle = hwnd;
        io.DeltaTime = 1.0f / 60.0f;
        POINT p; GetCursorPos(&p);
        io.MousePos.x = p.x - xy.x;
        io.MousePos.y = p.y - xy.y;
        if (GetAsyncKeyState(VK_LBUTTON)) {
            io.MouseDown[0] = true; io.MouseClicked[0] = true;
            io.MouseClickedPos[0].x = io.MousePos.x;
            io.MouseClickedPos[0].y = io.MousePos.y;
        } else io.MouseDown[0] = false;
        if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom) {
            old_rc = rc;
            Width = rc.right; Height = rc.bottom;
            d3dpp.BackBufferWidth = Width; d3dpp.BackBufferHeight = Height;
            SetWindowPos(Window, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
            D3dDevice->Reset(&d3dpp);
        }
        render();
    }
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    DestroyWindow(Window);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam)) return true;
    switch (Message) {
    case WM_DESTROY: xShutdown(); PostQuitMessage(0); exit(4); break;
    case WM_SIZE:
        if (D3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            d3dpp.BackBufferWidth = LOWORD(lParam); d3dpp.BackBufferHeight = HIWORD(lParam);
            HRESULT hr = D3dDevice->Reset(&d3dpp);
            if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
            ImGui_ImplDX9_CreateDeviceObjects();
        } break;
    default: return DefWindowProc(hWnd, Message, wParam, lParam); break;
    }
    return 0;
}

void xShutdown() {
    ShutdownRenderPipeline();
    if (TriBuf) TriBuf->Release();
    if (D3dDevice) D3dDevice->Release();
    if (p_Object) p_Object->Release();
    DestroyWindow(Window);
    UnregisterClass("notepad", NULL);
}
