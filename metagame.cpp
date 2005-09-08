
#include "metagame.h"
#include "gfx.h"

#include <string>
#include <numeric>

using namespace std;

class Faction {
public:
    string filename;
    Color color;
};

const Faction factions[] = {
    { "data/faction_a.dv2", Color(1.0, 0.0, 0.0) },
    { "data/faction_b.dv2", Color(1.0, 1.0, 0.0) },
    { "data/faction_c.dv2", Color(0.0, 1.0, 1.0) },
    { "data/faction_d.dv2", Color(1.0, 0.0, 1.0) },
    { "data/faction_e.dv2", Color(0.0, 1.0, 0.0) },
    { "data/faction_f.dv2", Color(1.0, 1.0, 1.0) },
    { "data/faction_g.dv2", Color(0.7, 0.7, 1.0) },
    { "data/faction_h.dv2", Color(0.0, 0.0, 1.0) }
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
    CHECK(keys.size() == fireHeld.size());
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
        for(int i = 0; i < keys.size(); i++) {
            if(count(playersymbol.begin(), playersymbol.end(), -1) <= playersymbol.size() - 2) {
                if(playerkey[i] != -1 && keys[i].keys[playerkey[i]].down) {
                    fireHeld[i]++;
                } else {
                    fireHeld[i] = 0;
                }
            } else {
                fireHeld[i] = 0;
            }
            if(fireHeld[i] > 60)
                fireHeld[i] = 60;
        }
        if(count(fireHeld.begin(), fireHeld.end(), 60) == playerkey.size() - count(playerkey.begin(), playerkey.end(), -1) && count(fireHeld.begin(), fireHeld.end(), 60) >= 2) {
            mode = MGM_SHOP;
            currentShop = 0;
            playerdata.clear();
            playerdata.resize(count(fireHeld.begin(), fireHeld.end(), 60));
            int pid = 0;
            for(int i = 0; i < playerdata.size(); i++) {
                if(playersymbol[i] != -1) {
                    playerdata[pid++].color = factions[playersymbol[i]].color;
                }
            }
            shop = Shop(&playerdata[0]);
        }
    } else if(mode == MGM_SHOP) {
        if(currentShop == -1) {
            for(int i = 0; i < playerdata.size(); i++)
                if(genKeystates(keys)[i].f.repeat)
                    checked[i] = true;
            if(count(checked.begin(), checked.end(), false) == 0) {
                for(int i = 0; i < playerdata.size(); i++)
                    playerdata[i].cash += lrCash[i];
                currentShop = 0;
                shop = Shop(&playerdata[0]);
            }
        } else if(shop.runTick(genKeystates(keys)[currentShop])) {
            currentShop++;
            if(currentShop != playerdata.size()) {
                shop = Shop(&playerdata[currentShop]);
            } else {
                mode = MGM_PLAY;
                for(int i = 0; i < playerdata.size(); i++) {
                    playerdata[i].damageDone = 0;
                    playerdata[i].kills = 0;
                    playerdata[i].wins = 0;
                }
                game = Game(&playerdata);
            }
        }
    } else if(mode == MGM_PLAY) {
        if(game.runTick(genKeystates(keys))) {
            gameround++;
            if(gameround % 1 == 0) {
                mode = MGM_SHOP;
                currentShop = -1;
                calculateLrStats();
                checked.clear();
                checked.resize(playerdata.size());
            } else {
                float firepower = game.firepowerSpent;
                game = Game(&playerdata);
                game.firepowerSpent = firepower;
            }
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
                drawDvec2(symbols[playersymbol[i]], Float4(10, 10 + 100 * i, 90, 90 + 100 * i), 1.0);
                setColor(Color(1.0, 1.0, 1.0) / 60 * fireHeld[i]);
                drawBox(Float4(5, 5 + 100 * i, 95, 95 + 100 * i), 1);
            }
        }
        CHECK(symbols.size() == factioncount);
        for(int i = 0; i < symbols.size(); i++) {
            if(count(playersymbol.begin(), playersymbol.end(), i) == 0) {
                setColor(factions[i].color);
                drawDvec2(symbols[i], symbolpos[i], 1.0);
            }
        }
        if(count(playersymbol.begin(), playersymbol.end(), -1) <= playersymbol.size() - 2) {
            setColor(1.0, 1.0, 1.0);
            drawText("hold fire to begin", 20, 120, 560);
        }
    } else if(mode == MGM_SHOP) {
        if(currentShop == -1) {
            setZoom(0, 0, 600);
            setColor(1.0, 1.0, 1.0);
            drawText("damage", 30, 20, 20);
            drawText("kills", 30, 20, 80);
            drawText("wins", 30, 20, 140);
            drawText("base", 30, 20, 200);
            drawText("totals", 30, 20, 320);
            drawMultibar(lrCategory[0], Float4(200, 20, 700, 50));
            drawMultibar(lrCategory[1], Float4(200, 80, 700, 110));
            drawMultibar(lrCategory[2], Float4(200, 140, 700, 170));
            drawMultibar(lrCategory[3], Float4(200, 200, 700, 230));
            drawMultibar(lrPlayer, Float4(200, 320, 700, 350));
        } else {
            shop.renderToScreen();
        }
    } else if(mode == MGM_PLAY) {
        game.renderToScreen(RENDERTARGET_SPECTATOR);
    } else {
        CHECK(0);
    }
}

vector<Keystates> Metagame::genKeystates(const vector<Controller> &keys) {
    vector<Keystates> kst(playerdata.size());
    int pid = 0;
    for(int i = 0; i < playerkey.size(); i++) {
        if(playerkey[i] != -1) {
            kst[pid].u = keys[i].u;
            kst[pid].d = keys[i].d;
            kst[pid].l = keys[i].l;
            kst[pid].r = keys[i].r;
            kst[pid].f = keys[i].keys[playerkey[i]];
            pid++;
        }
    }
    return kst;
}

void Metagame::calculateLrStats() {
    vector<vector<float> > values(4);
    for(int i = 0; i < playerdata.size(); i++) {
        values[0].push_back(playerdata[i].damageDone);
        values[1].push_back(playerdata[i].kills);
        values[2].push_back(playerdata[i].wins);
        values[3].push_back(1);
        dprintf("%d: %f %d %d", i, playerdata[i].damageDone, playerdata[i].kills, playerdata[i].wins);
    }
    vector<float> totals(values.size());
    for(int j = 0; j < totals.size(); j++) {
        totals[j] = accumulate(values[j].begin(), values[j].end(), 0.0);
    }
    int chunkTotal = 0;
    for(int i = 0; i < totals.size(); i++) {
        if(totals[i] > 1e-6)
            chunkTotal++;
    }
    dprintf("%d, %f\n", gameround, game.firepowerSpent);
    float totalReturn = 100 * pow(1.03, gameround) + game.firepowerSpent * 0.8;
    dprintf("Total cash is %f", totalReturn);
    
    for(int i = 0; i < playerdata.size(); i++) {
        for(int j = 0; j < totals.size(); j++) {
            if(totals[j] > 1e-6)
                values[j][i] /= totals[j];
        }
    }
    // values now stores percentages for each category
    
    vector<float> playercash(playerdata.size());
    for(int i = 0; i < playercash.size(); i++) {
        for(int j = 0; j < totals.size(); j++) {
            playercash[i] += values[j][i];
        }
        playercash[i] /= chunkTotal;
    }
    // playercash now stores percentages for players
    
    vector<int> playercashresult(playerdata.size());
    for(int i = 0; i < playercash.size(); i++) {
        playercashresult[i] = int(playercash[i] * totalReturn);
    }
    // playercashresult now stores cashola for players
    
    lrCategory = values;
    lrPlayer = playercash;
    lrCash = playercashresult;
    
}

void Metagame::drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const {
    float total = accumulate(sizes.begin(), sizes.end(), 0.0);
    if(total < 1e-6) {
        dprintf("multibar failed, total is %f\n", total);
        return;
    }
    float width = dimensions.ex - dimensions.sx;
    float per = width / total;
    float cpos = dimensions.sx;
    for(int i = 0; i < sizes.size(); i++) {
        setColor(playerdata[i].color);
        float epos = cpos + sizes[i] * per;
        drawShadedBox(Float4(cpos, dimensions.sy, epos, dimensions.ey), 1, 6);
        cpos = epos;
    }
}

// not a valid state
Metagame::Metagame() {
}

Metagame::Metagame(int playercount) {

    playerkey.clear();
    playersymbol.clear();
    playerpos.clear();
    playerkey.resize(playercount, -1);
    playersymbol.resize(playercount, -1);
    playerpos.resize(playercount, Float2(400, 300));
    fireHeld.resize(playercount);
    
    for(int i = 0; i < factioncount; i++) {
        symbols.push_back(loadDvec2(factions[i].filename.c_str()));
    }
    
    for(int i = 0; i < symbols.size(); i++) {
        symbolpos.push_back( boxaround( angle(PI * 2 * i / symbols.size()) * 200 + Float2( 400, 300 ), 50 ) );
    }
    
    mode = MGM_PLAYERCHOOSE;
    
    gameround = 0;

}
