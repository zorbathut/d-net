
#include "interface.h"
#include "game.h"
#include "gfx.h"

Game game;

void interfaceRunTick( const vector< Keystates > &keys ) {
    game.runTick(keys);
}
    
void interfaceRenderToScreen() {
    game.renderToScreen(RENDERTARGET_SPECTATOR);
    setColor(1.0, 1.0, 1.0);
    drawText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, 0, 0);
    drawText("the quick brown fox jumped over the lazy dog", 5, 0, 6);
}
