// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#include "../event_manager.hpp"
#include "../../offsets/offsets.hpp"
#include "../../primitives/primitives.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/memory/memory.hpp"
#include <memory>

namespace fortnite {
	namespace world {
        struct data
        {
            uint64_t gworld {};
            uint64_t game_instance {};
            uint64_t player_controller {};
            ueegnine::tarray<uint64_t> local_players {};
            ueegnine::tarray<uint64_t> view_state {};
            ueegnine::tarray<uint64_t> levels {};
            ueegnine::tarray<uint64_t> actors {};
        };
        inline double_buffer_cache<data> cache;
        void start( );
        void start_c( );
        void stop( );
        std::shared_ptr<const data> get( );
	}
}