// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <memory>
#include "../../imgui.h"
#include "../../imgui_internal.h"
#include "../../imgui_impl_dx11.h"
#include "../../imgui_impl_win32.h"
#include <vector>
#include <string>
#include <d3d11.h>
#include <d3dx11tex.h>
#include <functional>
#include "unicodes.hpp"
#include <any>
#include <Windows.h>
#include <wininet.h>
#include <stdexcept>
#include "types.h"
#include "style.h"
#include "font.h"
#include "widgets.h"
#include "../../thirdparty/include/animations.hpp"
#include "draw.h"
#include <d3dcompiler.h>
#include <thread>

#pragma comment( lib, "wininet.lib" )

#define ds * g_style->dpi_scale

class c_slate {
public:
	ID3D11Device* m_device; ID3D11DeviceContext* m_ctx; IDXGISwapChain* m_swapchain;

	c_image logo;
	c_image avatar;

	bool settings_open = false;
	float settings_anim = 0;

	void initialize_fonts( );
	void initialize( ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swapchain );
	void draw( ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* swapchain );
};

inline auto slate = std::make_unique< c_slate >( );