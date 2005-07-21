
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
    
    assert(keys.size() >= 1);
    
    switch(interface_mode) {
        
        case IMODE_MAINMENU:
            if(keys[0].firing) {
                if(menupos == 0) {
                    interface_mode = IMODE_PLAYING;
                    game = Game();
                } else {
                    return true;
                }
            } else if(keys[0].forward || keys[0].back) {
                menupos = menupos ^ 1; // ick
            }
            break;
            
        case IMODE_PLAYING:
            if(game.runTick(keys)) {
                interface_mode = IMODE_MAINMENU;
                menupos = 0;
            }
            break;
        
        default:
            assert(0);
            break;
    }
    
    return false;
    
}
    
void interfaceRenderToScreen() {
    switch(interface_mode) {
        
        case IMODE_MAINMENU: {
            const char *menutext[] = { "New game", "Exit" };
            setZoom( 0, 0, 100 );
            for(int i = 0; i < 2; i++) {
                if(menupos == i ) {
                    setColor(1.0, 1.0, 1.0);
                } else {
                    setColor(0.5, 0.5, 0.5);
                }
                drawText(menutext[i], 5, 2, 2 + 6 * i );
            }
            break;
        }
        
        case IMODE_PLAYING:
            game.renderToScreen(RENDERTARGET_SPECTATOR);
            setColor(1.0, 1.0, 1.0);
            drawText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 5, 0, 0);
            drawText("the quick brown fox jumped over the lazy dog", 5, 0, 6);
            break;
        
        default:
            assert(0);
            break; 
        
    }
}
