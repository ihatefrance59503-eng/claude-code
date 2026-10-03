// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../dependenices/imgui/imgui.h"

namespace fortnite
{
	namespace drawing
	{
		void player_esp( ImDrawList* draw_list );
		void draw_pickups( ImDrawList* draw_list );
		void draw_containers( ImDrawList* draw_list );
		void draw_vehicles( ImDrawList* draw_list );
		void draw_weakspot( ImDrawList* draw_list );
		void draw_fov_circle( ImDrawList* draw_list );
		void draw_crosshair( ImDrawList* draw_list );
		void draw_bullet_tracers( ImDrawList* draw_list );
	}
}
