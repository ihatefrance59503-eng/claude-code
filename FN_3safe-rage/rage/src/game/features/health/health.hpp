// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#include <cstdint>
#include "../../primitives/primitives.hpp"
#include "../../offsets/offsets.hpp"
#include "../../../../dependenices/memory/memory.hpp"
#include "../../thread/world/world.hpp"
#pragma comment(lib, "winmm.lib")
#include <algorithm>
#include <unordered_map>
#include <cctype>

namespace fortnite
{
	namespace health
	{
		struct bar
		{
			float value;
			float max;
			float shield;
			float max_shield;
			bool dbno;
			bool is_healing;
		};

		bar get_bar( std::uint64_t actor , float distance_cm );
	}
}