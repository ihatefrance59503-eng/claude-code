// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once 
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../dependenices/impl.hpp"
#include "../../dependenices/imgui/imgui.h"
#include "../../dependenices/imgui/imgui_impl_dx11.h"
#include "../../dependenices/imgui/imgui_impl_win32.h"
#include "../game/features/visuals/visuals.hpp"
#include "../game/features/radar/radar.hpp"
#include "../../dependenices/imgui/slate/include/slate.h"
#include "../../dependenices/extras/extras.hpp"
#include "../game/features/aimbot/aimbot.hpp"
#include "../game/features/trigger/trigger.hpp"
#include "../game/features/widgets/widgets.hpp"
#include "../game/features/zone/zone.hpp"
#include "../game/features/misc/misc.hpp"
#include <psapi.h>
#pragma comment(lib, "Psapi.lib")
#include <d3dX11tex.h>

namespace fortnite {
	namespace render {
		inline HWND window;
		inline ID3D11Device* g_pd3dDevice = nullptr;
		inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
		inline IDXGISwapChain* g_pSwapChain = nullptr;
		inline bool                     g_SwapChainOccluded = false;
		inline UINT                     g_ResizeWidth = 0 , g_ResizeHeight = 0;
		inline ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
		inline int height , width;
		inline ImFont* yahei_f = nullptr;
		inline ImFont* window_fonts [ 20 ] = { nullptr };

		inline int rendered_players = 0;
		inline int visible_players = 0;
		inline int players_within_20m = 0;
		//

		bool set_up( HWND window );
		HWND find_window( );
		void tick( );
		void get_screen( );
	}
}