// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <atomic>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../thread/entity/entity.hpp"
#include "../../thread/world/world.hpp"
#include "../../../drawing/drawing.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/imgui/imgui.h"

namespace fortnite {
	namespace visuals {
		inline std::atomic<bool> running = true;
		void tick( );
		void stop( );






	}
}