// safe-rage: forked from dark-rage. read-only posture, no rotation writes,
// no exploit soup, makcu input layer.
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
		MessageBoxA( GetConsoleWindow( ) , encrypt( "driver not loaded" ) , encrypt( "map" ) , 0 );
		return 1;
	}
	Sleep( 200 );
	fortnite::communcations::process_id = fortnite::communcations::get_process_id( encrypt( L"FortniteClient-Win64-Shipping.exe" ) );
	Sleep( 200 );
	fortnite::communcations::base_address = fortnite::communcations::get_base( );
	Sleep( 200 );
	fortnite::communcations::get_cr3( );
	Sleep( 200 );
	fortnite::mouse::open( );
	Sleep( 200 );
	fortnite::config::load( );
	Sleep( 200 );
	fortnite::render::get_screen( );
	Sleep( 200 );
	fortnite::render::set_up( fortnite::render::find_window( ) );
	Sleep( 200 );
	fortnite::world::start( );
	Sleep( 200 );
	fortnite::world::start_c( );
	Sleep( 200 );
	fortnite::entity::start( );
	Sleep( 200 );
	fortnite::render::tick( );
}
