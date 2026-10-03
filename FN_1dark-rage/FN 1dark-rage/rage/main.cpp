// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#include "src/render/render.hpp"
#include "src/game/offsets/offsets.hpp"
#include "src/game/thread/entity/entity.hpp"
#include "src/game/thread/world/world.hpp"

#include "dependenices/mouse/mouse.hpp"
#include "dependenices/configs/config.hpp"

int main( INT32 ) {
	bool status;
	status = fortnite::communcations::find_driver( );
	if ( !status ) {
		MessageBoxA( GetConsoleWindow( ) , encrypt("jdhjksdhjk") , encrypt("map") , 0 );
	}
	fortnite::communcations::process_id = fortnite::communcations::get_process_id( encrypt( L"FortniteClient-Win64-Shipping.exe" ) );
	fortnite::communcations::base_address = fortnite::communcations::get_base( );
	fortnite::communcations::get_cr3( );
	fortnite::config::load( );
	fortnite::render::get_screen( );

	fortnite::render::set_up( fortnite::render::find_window( ) );
	std::cout << "hello1?\n";

	fortnite::world::start( );
	std::cout << "hello2?\n";

	fortnite::world::start_c( );
	std::cout << "hello3?\n";

	fortnite::entity::start( );
	std::cout << "hello4?\n";

	fortnite::render::tick( );
}