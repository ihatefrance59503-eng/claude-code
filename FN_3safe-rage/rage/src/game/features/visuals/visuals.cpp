// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "visuals.hpp"



void fortnite::visuals::tick( )
{
	if ( !running )
		return;
	ImDrawList* draw_list = ImGui::GetForegroundDrawList( );
	fortnite::drawing::player_esp( draw_list );

	fortnite::drawing::draw_pickups( draw_list );
	fortnite::drawing::draw_containers( draw_list );
	fortnite::drawing::draw_vehicles( draw_list );
	fortnite::drawing::draw_weakspot( draw_list );
	fortnite::drawing::draw_fov_circle( draw_list );
	fortnite::drawing::draw_crosshair( draw_list );
	fortnite::drawing::draw_bullet_tracers( draw_list );


}

void fortnite::visuals::stop( )
{
	running = false;
}
