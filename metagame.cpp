
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

const HierarchyNode &Shop::getStepNode(int step) const {
    CHECK(step >= 0 && step <= curloc.size());
    const HierarchyNode *cnode = &itemDbRoot();
    for(int i = 0; i < step; i++) {
        CHECK(curloc[i] >= 0 && curloc[i] < cnode->branches.size());
        cnode = &cnode->branches[curloc[i]];
    }
    return *cnode;
}

const HierarchyNode &Shop::getCurNode() const {
    return getStepNode(curloc.size());
}

const HierarchyNode &Shop::getCategoryNode() const {
    return getStepNode(curloc.size() - 1);
}

void Shop::renderNode(const HierarchyNode &node, int depth) const {
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
        {
            int dispmode = node.branches[i].displaymode;
            if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
                if(!player->hasUpgrade(node.branches[i].upgrade))
                    dispmode = HierarchyNode::HNDM_COST;
            }
            if(dispmode == HierarchyNode::HNDM_BLANK) {
            } else if(dispmode == HierarchyNode::HNDM_COST) {
                char bf[128];
                sprintf(bf, formatstring, node.branches[i].cost);
                drawText( bf, fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder );
            } else if(dispmode == HierarchyNode::HNDM_PACK) {
                char bf[128];
                sprintf(bf, "%dpk", node.branches[i].quantity);
                drawText( bf, fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder );
            } else if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
                drawText("bought", fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder);
            } else {
                CHECK(0);
            }
        }
    }
}

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
    if(keys.f.repeat && getCurNode().buyable && player->cash >= getCurNode().cost) {
        player->cash -= getCurNode().cost;
        if(getCurNode().type == HierarchyNode::HNT_DONE) {
            return true;
        } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
            bool allowbuy = true;
            if(getCurNode().displaymode == HierarchyNode::HNDM_COSTUNIQUE && player->hasUpgrade(getCurNode().upgrade))
                allowbuy = false;
            if(allowbuy) {
                player->upgrades.push_back(getCurNode().upgrade);
                player->reCalculate();
            } else {
                player->cash += getCurNode().cost;
            }
        } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
            if(player->weapon != getCurNode().weapon) {
                if(player->shotsLeft != -1)
                    player->cash += player->resellAmmoValue();
                player->weapon = getCurNode().weapon;
                player->shotsLeft = 0;
            }
            player->shotsLeft += getCurNode().quantity;
        } else {
            CHECK(0);
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
    {
        char bf[128];
        if(player->shotsLeft == -1) {
            sprintf(bf, "%10s infinite ammo", player->weapon->name.c_str());
        } else {
            sprintf(bf, "%15s %4d shots %6d resell", player->weapon->name.c_str(), player->shotsLeft, player->resellAmmoValue());
        }
        drawText(bf, 2, 1, 1);
    }
    renderNode(itemDbRoot(), 0);
}

// Not a valid state
Shop::Shop() {
    player = NULL;
}

Shop::Shop(Player *in_player) {
    player = in_player;
    curloc.push_back(0);
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
            if(currentShop != playerdata.size()) {
                shop = Shop(&playerdata[currentShop]);
            } else {
                mode = MGM_PLAY;
                game = Game(&playerdata);
            }
        }
    } else if(mode == MGM_PLAY) {
        if(game.runTick(genKeystates(keys))) {
            mode = MGM_SHOP;
            currentShop = 0;
            shop = Shop(&playerdata[currentShop]);
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
