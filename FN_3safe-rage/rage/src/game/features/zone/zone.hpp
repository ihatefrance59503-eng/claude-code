// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../thread/entity/entity.hpp"
#include "../../thread/world/world.hpp"
#include "../../../drawing/drawing.hpp"
#include "../../sdk/sdk.hpp"
#include "../../../../dependenices/imgui/imgui.h"
struct zone_data_t
{
    fortnite::uemath::fvector center;
    float radius;
    float min_distance;
    float max_distance;
    float reject;
    float reject_offset;
    float wait_time;
    float shrink_time;
    float grid_size;
    float solo_radius;
    float duo_radius;
    float squad_radius;
};

namespace fortnite::zone
{
    extern zone_data_t current_zone;
    extern bool zone_valid;

    void update_zone( );

    void draw_zone_circle( ImDrawList* draw_list , const fortnite::uemath::fvector& zone_center , float radius , ImU32 color , float thickness = 2.f );

    void draw_zone_esp( );
}