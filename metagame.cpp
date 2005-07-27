
#include "metagame.h"
#include "gfx.h"

bool Metagame::runTick( const vector< Controller > &keys ) {
    if(mode == MGM_INIT) {
        
        playerkey.clear();
        playersymbol.clear();
        playerpos.clear();
        playerkey.resize(keys.size(), -1);
        playersymbol.resize(keys.size(), -1);
        playerpos.resize(keys.size(), Float2(400, 300));
        
        for(int i = 0; i < 6; i++) {
            char bf[128];
            sprintf(bf, "data/%c.dvec", 'a' + i);
            symbols.push_back(loadVectors(bf));
        }
        
        for(int i = 0; i < symbols.size(); i++) {
            symbolpos.push_back( boxaround( angle(PI * 2 * i / symbols.size()) * 200 + Float2( 400, 300 ), 50 ) );
        }
        
        mode = MGM_PLAYERCHOOSE;
    }
    CHECK(keys.size() == playerpos.size());
    CHECK(keys.size() == playersymbol.size());
    CHECK(keys.size() == playerkey.size());
    if(mode == MGM_PLAYERCHOOSE) {
        for(int i = 0; i < keys.size(); i++) {
            if(playersymbol[i] != -1)
                continue;
            playerpos[i].x += keys[i].x * 4;
            playerpos[i].y -= keys[i].y * 4;
            int targetInside = -1;
            for(int j = 0; j < symbolpos.size(); j++)
                if(isinside(symbolpos[j], playerpos[i]) && !count(playersymbol.begin(), playersymbol.end(), j))
                    targetInside = j;
            if(targetInside == -1)
                continue;
            for(int j = 0; j < keys[i].keys.size(); j++) {
                if(keys[i].keys[j].repeat) {
                    playersymbol[i] = targetInside;
                    playerkey[i] = j;
                }
            }
        }
    }
    return false;
}

void Metagame::renderToScreen() const {
    setZoom(0, 0, 600);
    setColor(1.0, 1.0, 1.0);
    for(int i = 0; i < playerpos.size(); i++) {
        if(playersymbol[i] == -1) {
            char bf[16];
            sprintf(bf, "p%d", i);
            drawLine(playerpos[i].x, playerpos[i].y - 15, playerpos[i].x, playerpos[i].y - 5, 1.0);
            drawLine(playerpos[i].x, playerpos[i].y + 15, playerpos[i].x, playerpos[i].y + 5, 1.0);
            drawLine(playerpos[i].x - 15, playerpos[i].y, playerpos[i].x - 5, playerpos[i].y, 1.0);
            drawLine(playerpos[i].x +15, playerpos[i].y, playerpos[i].x + 5, playerpos[i].y, 1.0);
            drawText(bf, 20, playerpos[i].x + 5, playerpos[i].y + 5);
        } else {
            dprintf("Players %d %d\n", i, playersymbol[i]);
            drawVectors(symbols[playersymbol[i]], Float4(20, 20 + 100 * i, 100, 100 + 100 * i), true, true, 1.0);
        }
    }
    for(int i = 0; i < symbols.size(); i++) {
        if(count(playersymbol.begin(), playersymbol.end(), i) == 0) {
            drawVectors(symbols[i], symbolpos[i], true, true, 1.0);
        }
    }
}

Metagame::Metagame() {
    mode = MGM_INIT;
}
