
#include "interface.h"
#include "game.h"

Game game;

void interfaceRunTick( const vector< Keystates > &keys ) {
    game.runTick(keys);
}
    
void interfaceRenderToScreen() {
    game.renderToScreen(RENDERTARGET_SPECTATOR);
}
