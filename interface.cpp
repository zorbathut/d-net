
#include "interface.h"
#include "game.h"
#include "gfx.h"

Game game;

int interface_mode;
int menupos;

enum { IMODE_MAINMENU, IMODE_PLAYING };

void interfaceInit() {
    interface_mode = IMODE_MAINMENU;
    menupos = 0;
}

bool interfaceRunTick( const vector< Keystates > &keys ) {
    game.runTick(keys);
    return false;
}
    
void interfaceRenderToScreen() {
    game.renderToScreen(RENDERTARGET_SPECTATOR);
    setColor(1.0, 1.0, 1.0);
    drawText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, 0, 0);
    drawText("the quick brown fox jumped over the lazy dog", 5, 0, 6);
}
