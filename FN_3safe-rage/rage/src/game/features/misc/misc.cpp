// safe-rage: misc stripped to read-only shell. no rainbow crosshair,
// no interaction scrubbing, no player size writes. all previously
// written game-state fields are detection surface — removed.
#include "misc.hpp"

void fortnite::misc::tick( )
{
	if ( !running )
		return;

	// read-only tick body goes here. anything that WRITES to game memory
	// does not belong in safe-rage. drive input through makcu, HUD through
	// the overlay (if you keep one), state readouts through communcations::read.
}

void fortnite::misc::stop( )
{
	running = false;
}
