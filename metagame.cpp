
#include "metagame.h"
#include "gfx.h"

#include <string>

using namespace std;

class Faction {
public:
    string filename;
    Color color;
};

const Faction factions[] = {
    { "data/a.dvec", Color(1.0, 0.0, 0.0) },
    { "data/b.dvec", Color(1.0, 1.0, 0.0) },
    { "data/c.dvec", Color(0.0, 1.0, 1.0) },
    { "data/d.dvec", Color(1.0, 0.0, 1.0) },
    { "data/e.dvec", Color(0.0, 1.0, 0.0) },
    { "data/f.dvec", Color(1.0, 1.0, 1.0) }
};

const int factioncount = sizeof(factions) / sizeof(Faction);

bool Metagame::runTick( const vector< Controller > &keys ) {
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
        if(count(playerkey.begin(), playerkey.end(), -1) == 0) {
            mode = MGM_SHOP;
            currentShop = 0;
            playerdata.clear();
            playerdata.resize(playerkey.size());
            for(int i = 0; i < playerdata.size(); i++)
                playerdata[i].color = factions[playersymbol[i]].color;
        }
    } else if(mode == MGM_SHOP) {
        Keystates target = genKeystates(keys)[currentShop];
        if(target.f.repeat) {
            currentShop++;
            if(currentShop == playerdata.size()) {
                mode = MGM_PLAY;
                game = Game(&playerdata);
            }
        }
    } else if(mode == MGM_PLAY) {
        if(game.runTick(genKeystates(keys))) {
            mode = MGM_SHOP;
            currentShop = 0;
        }
    } else {
        CHECK(0);
    }
    return false;
}

void Metagame::renderToScreen() const {
    if(mode == MGM_PLAYERCHOOSE) {
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
        CHECK(symbols.size() == factioncount);
        for(int i = 0; i < symbols.size(); i++) {
            if(count(playersymbol.begin(), playersymbol.end(), i) == 0) {
                setColor(factions[i].color);
                drawVectors(symbols[i], symbolpos[i], true, true, 1.0);
            }
        }
    } else if(mode == MGM_SHOP) {
        clearFrame(factions[playersymbol[currentShop]].color * 0.05 + Color(0.05, 0.05, 0.05));
        setColor(1.0, 1.0, 1.0);
        setZoom(0, 0, 100);
        char shopText[128];
        sprintf(shopText, "shop %d", currentShop);
        drawText(shopText, 5, 5, 5);
    } else if(mode == MGM_PLAY) {
        game.renderToScreen(RENDERTARGET_SPECTATOR);
    } else {
        CHECK(0);
    }
}

vector<Keystates> Metagame::genKeystates(const vector<Controller> &keys) {
    vector<Keystates> kst(playerpos.size());
    for(int i = 0; i < playerpos.size(); i++) {
        kst[i].u = keys[i].u;
        kst[i].d = keys[i].d;
        kst[i].l = keys[i].l;
        kst[i].r = keys[i].r;
        kst[i].f = keys[i].keys[playerkey[i]];
    }
    return kst;
}

// not a valid state
Metagame::Metagame() { }

Metagame::Metagame(int playercount) {

    playerkey.clear();
    playersymbol.clear();
    playerpos.clear();
    playerkey.resize(playercount, -1);
    playersymbol.resize(playercount, -1);
    playerpos.resize(playercount, Float2(400, 300));
    
    for(int i = 0; i < factioncount; i++) {
        symbols.push_back(loadVectors(factions[i].filename.c_str()));
    }
    
    for(int i = 0; i < symbols.size(); i++) {
        symbolpos.push_back( boxaround( angle(PI * 2 * i / symbols.size()) * 200 + Float2( 400, 300 ), 50 ) );
    }
    
    mode = MGM_PLAYERCHOOSE;

}
