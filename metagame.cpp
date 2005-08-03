
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

ShopNode::ShopNode() {
    name = "null";
    cost = 0;
    showcost = false;
    choosable = false;
}
ShopNode::ShopNode(const string &in_name, int in_cost, bool in_showcost, bool in_choosable) {
    name = in_name;
    cost = in_cost;
    showcost = in_showcost;
    choosable = in_choosable;
}

ShopNode &Shop::getCurNode() {
    ShopNode *cnode = &root;
    for(int i = 0; i < curloc.size(); i++) {
        CHECK(curloc[i] >= 0 && curloc[i] < cnode->branches.size());
        cnode = &cnode->branches[curloc[i]];
    }
    return *cnode;
}

ShopNode &Shop::getCategoryNode() {
    ShopNode *cnode = &root;
    for(int i = 0; i < curloc.size() - 1; i++) {
        CHECK(curloc[i] >= 0 && curloc[i] < cnode->branches.size());
        cnode = &cnode->branches[curloc[i]];
    }
    return *cnode;
}

void Shop::renderNode(const ShopNode &node, int depth) const {
    CHECK(depth < 3 || node.branches.size() == 0);
    const float hoffset = 1;
    const float voffset = 5;

    const float fontsize = 2;
    const float boxborder = 0.5;
    const float itemheight = 4;

    const float boxwidth = 40;
    
    const float pricehpos = 29;

    const float boxthick = 0.1;
    
    const char formatstring[] = "%6d";
    
    float hoffbase = hoffset + ( boxwidth + hoffset ) * depth;
    
    for(int i = 0; i < node.branches.size(); i++) {
        if(depth < curloc.size() && curloc[depth] == i) {
            setColor(1.0, 1.0, 1.0);
            renderNode(node.branches[i], depth + 1);
        } else {
            setColor(0.3, 0.3, 0.3);
        }
        drawBox( Float4( hoffbase, voffset + i * itemheight, hoffbase + boxwidth, voffset + i * itemheight + fontsize + boxborder * 2 ), boxthick );
        setColor(1.0, 1.0, 1.0);
        drawText( node.branches[i].name.c_str(), fontsize, hoffbase + boxborder, voffset + i * itemheight + boxborder );
        if(node.branches[i].showcost) {
            char bf[128];
            sprintf(bf, formatstring, node.branches[i].cost);
            drawText( bf, fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder );
        }
    }
}

// Shop::recreateShopNetwork() has been moved to const.cpp because it compiles so painfully slowly.
// I suspect that's thanks to all the constants. It'll be moved back once it's sane.

bool Shop::runTick(const Keystates &keys) {
    if(keys.l.repeat && curloc.size() > 1)
        curloc.pop_back();
    if(keys.r.repeat && getCurNode().branches.size() != 0)
        curloc.push_back(0);
    if(keys.u.repeat)
        curloc.back()--;
    if(keys.d.repeat)
        curloc.back()++;
    curloc.back() += getCategoryNode().branches.size();
    curloc.back() %= getCategoryNode().branches.size();
    if(keys.f.repeat && getCurNode().choosable && player->cash >= getCurNode().cost) {
        player->cash -= getCurNode().cost;
        if(getCurNode().name == "done") {
            return true;
        } else if(getCurNode().name == "hull boost") {
            player->maxHealth += 5;
        } else {
            dprintf("Bought a %s\n", getCurNode().name.c_str());
        }
    }
    return false;
}

void Shop::renderToScreen() const {
    CHECK(player);
    clearFrame(player->color * 0.05 + Color(0.05, 0.05, 0.05));
    setColor(1.0, 1.0, 1.0);
    setZoom(0, 0, 100);
    {
        char bf[128];
        sprintf(bf, "cash onhand %6d", player->cash);
        drawText(bf, 2, 80, 1);
    }
    renderNode(root, 0);
}

// Not a valid state
Shop::Shop() {
    player = NULL;
}

Shop::Shop(Player *in_player) {
    player = in_player;
    recreateShopNetwork();
}

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
            shop = Shop(&playerdata[0]);
        }
    } else if(mode == MGM_SHOP) {
        if(shop.runTick(genKeystates(keys)[currentShop])) {
            currentShop++;
            if(currentShop == playerdata.size()) {
                mode = MGM_PLAY;
                game = Game(&playerdata);
            } else {
                shop = Shop(&playerdata[currentShop]);
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
                setColor(1.0, 1.0, 1.0);
                char bf[16];
                sprintf(bf, "p%d", i);
                drawLine(playerpos[i].x, playerpos[i].y - 15, playerpos[i].x, playerpos[i].y - 5, 1.0);
                drawLine(playerpos[i].x, playerpos[i].y + 15, playerpos[i].x, playerpos[i].y + 5, 1.0);
                drawLine(playerpos[i].x - 15, playerpos[i].y, playerpos[i].x - 5, playerpos[i].y, 1.0);
                drawLine(playerpos[i].x +15, playerpos[i].y, playerpos[i].x + 5, playerpos[i].y, 1.0);
                drawText(bf, 20, playerpos[i].x + 5, playerpos[i].y + 5);
            } else {
                setColor(factions[playersymbol[i]].color);
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
        shop.renderToScreen();
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
