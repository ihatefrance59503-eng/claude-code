// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#include "../../primitives/primitives.hpp"
#include "../../../../dependenices/memory/memory.hpp"
#include "../../thread/entity/entity.hpp"

namespace fortnite {
	namespace aimbot {
		//fortnite::entity::actor_data* best_target;
		//float best_fov_distance = 100;

		void tick( );
		float calculate_fov_distance( uemath::fvector2d screen_pos );



	}
}