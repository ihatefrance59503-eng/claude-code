// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <set>
#include <mutex>
#include <algorithm>
#include <chrono>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../thread/entity/entity.hpp"
#include "../../thread/world/world.hpp"
#include "../../../drawing/drawing.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/imgui/imgui.h"

namespace fortnite {
	namespace widget {
		struct ESpectatorArray
		{
			uintptr_t Data;
			uint32_t Count;
			uint32_t Max;
		};

		inline std::atomic<bool> running = true;
		inline std::unordered_set<std::string> UpdatedNames;
		void find_spectators( );
		void specatator_widget( );
		void keybind_widget( );
		std::string get_key_name( int vk );
		void stop( );
		bool is_key_down( int vk );
		void handle_drag( ImVec2& pos , ImVec2 size , bool& dragging , ImVec2& offset );




	}
}