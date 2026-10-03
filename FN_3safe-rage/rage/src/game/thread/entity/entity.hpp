// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once

#include "../event_manager.hpp"
#include "../../offsets/offsets.hpp"
#include "../../primitives/primitives.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/memory/memory.hpp"
#include "../world/world.hpp"

#include <vector>
#include <memory>
#include <mutex>

namespace fortnite
{
    namespace entity
    {
        struct client_quick_bars_t
        {
            int current_slot; // 0x0
        };

        struct actor_data
        {
            uint64_t current {};
            uint64_t mesh {};
            uint64_t player_state {};
            uint64_t root_component {};
            int32_t squad_size {};
            int32_t team_id {};
            int32_t distance {};
            std::string name {};
            std::string platform {};
            std::string weapon_name {};
            std::string rank {};
            int32_t rarity {};
        };

        struct pickup_data
        {
            uint64_t current {};
            uint64_t root_comp {};
            std::string name {};
            int32_t rarity {};
        };

        struct container_data {
            uint64_t current {};
            uint64_t root_component {};
            uint64_t mesh_component {};
            int32_t spawn_source {};
        };

        struct projectile_data {
            uint64_t current {};
            uint64_t root_component {};
        };

        struct vehicle_data {
            uint64_t current {};
            uint64_t root_component {};
            uint64_t mesh {};
            float critical_health {};
        };

        struct weakspot_data {
            uint64_t current {};
            uint64_t root_component {};
        };

        struct supply_drop_data {
            uint64_t current {};
            uint64_t root_component {};
            uemath::fvector world_location {};
            int32_t distance {};
            int32_t players_interacting {};
        };

        struct building_data {
            uint64_t current {};
            uint64_t root_component {};
            uint64_t mesh_component {};
            uint8_t building_type {};
        };

        // Internal bulk data structure for master cache
        struct processed_actor_data
        {
            uint64_t actor;
            float revive_time;
            float lifespan_after_death;
            float simulating_too_long_length;
            uint64_t search_text_ptr;
            uint8_t container_flags;

            uint64_t root_component;
            uint64_t player_state;
            uint64_t mesh;
            uint64_t current_weapon;

            uint64_t container_mesh_component;
            int32_t container_spawn_source;
            int32_t max_level;

            float vehicle_critical_health;
            uint64_t vehicle_mesh;
        };

        // Global shared bulk data (updated by master_cache)
        inline std::vector<processed_actor_data> g_shared_bulk_data;
        inline std::mutex g_bulk_mutex;

        inline uint64_t local_pawn;
        inline double_buffer_cache<std::vector<actor_data>> actor_cache;
        inline double_buffer_cache<std::vector<pickup_data>> pickup_cache;
        inline double_buffer_cache<std::vector<uint8_t>> master_cache;
        inline double_buffer_cache<std::vector<container_data>> container_cache;
        inline double_buffer_cache<std::vector<projectile_data>> projectile_cache;
        inline double_buffer_cache<std::vector<vehicle_data>> vehicle_cache;
        inline double_buffer_cache<std::vector<weakspot_data>> weakspot_cache;
        inline double_buffer_cache<std::vector<supply_drop_data>> supply_drop_cache;
        inline double_buffer_cache<std::vector<building_data>> building_cache;

        void start( );
        void stop( );

        std::shared_ptr<const std::vector<actor_data>> get_a( );
        std::shared_ptr<const std::vector<pickup_data>> get_p( );
        std::shared_ptr<const std::vector<container_data>> get_containers( );
        std::shared_ptr<const std::vector<projectile_data>> get_projectiles( );
        std::shared_ptr<const std::vector<vehicle_data>> get_vehicles( );
        std::shared_ptr<const std::vector<weakspot_data>> get_weakspots( );
        std::shared_ptr<const std::vector<supply_drop_data>> get_supply_drops( );
        std::shared_ptr<const std::vector<building_data>> get_buildings( );
    }
}