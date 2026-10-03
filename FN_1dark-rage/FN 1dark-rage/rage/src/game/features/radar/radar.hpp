// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once 
#include "../../primitives/primitives.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/imgui/imgui.h"
#include "../../../settings/settings.hpp"
#include <thread>
#include <vector>


struct RadarEntry {
    fortnite::uemath::fvector2d pos;
    bool bIsVisible;
    fortnite::uemath::frotator rot;
    int distance;
};
namespace fortnite {
    namespace radar {
        inline std::vector<RadarEntry> entries;
        uemath::fvector2d rotate_point( uemath::fvector2d radarPos , uemath::fvector2d radarSize , uemath::fvector localPos , uemath::fvector targetPos );
        void clamp_to_radar( float* x , float* y , float range );
        void world_to_radar( const uemath::fvector& TargetPos , int& outX , int& outY );
        void add_to_radar( uemath::fvector WorldLocation , bool bIsVisible , int Distance , uemath::frotator RelativeRotation, int32_t team_id );
        void add_pickup_to_radar( uemath::fvector WorldLocation, uint8_t rarity  );
        void radar_range( float* x , float* y , float range );
        void calculate_point_on_radar( uemath::fvector vOrigin , int& screenx , int& screeny );
        void calc_range( float* x , float* y , float range );
        void tick( );
    };
}