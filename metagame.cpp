
#include "metagame.h"
#include "gfx.h"
#include "parse.h"
#include "rng.h"
#include "inputsnag.h"
#include "args.h"

#include <string>
#include <numeric>
#include <fstream>

using namespace std;

class Faction {
public:
    string filename;
    Color color;
};

const Faction factions[] = {
    { "data/faction_f.dv2", Color(1.0, 1.0, 1.0) }, // omega
    
    { "data/faction_a.dv2", Color(1.0, 0.0, 0.0) }, // pitchfork
    { "data/faction_e.dv2", Color(0.0, 1.0, 0.0) }, // serpent
    { "data/faction_h.dv2", Color(0.0, 0.0, 1.0) }, // ocean
    
    { "data/faction_b.dv2", Color(1.0, 1.0, 0.0) }, // lightning
    { "data/faction_c.dv2", Color(0.0, 1.0, 1.0) }, // H
    { "data/faction_d.dv2", Color(1.0, 0.0, 1.0) }, // compass
    { "data/faction_g.dv2", Color(0.7, 0.7, 1.0) }, // buzzsaw
    { "data/faction_i.dv2", Color(1.0, 0.7, 0.7) }, // zen
    { "data/faction_j.dv2", Color(1.0, 0.7, 0.0) }, // pincer
    { "data/faction_k.dv2", Color(0.0, 0.7, 1.0) }, // hourglass
    { "data/faction_l.dv2", Color(1.0, 0.6, 0.8) } // poison
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
    const float hoffset = 1.5;
    const float voffset = 5;

    const float fontsize = 2;
    const float boxborder = 0.5;
    const float itemheight = 4;

    const float boxwidth = 42;
    
    const float pricehpos = 29;

    const float boxthick = 0.1;
    
    float hoffbase = hoffset + ( boxwidth + hoffset ) * depth;
    
    for(int i = 0; i < node.branches.size(); i++) {
        if(depth < curloc.size() && curloc[depth] == i) {
            setColor(1.0, 1.0, 1.0);
            renderNode(node.branches[i], depth + 1);
        } else {
            setColor(0.3, 0.3, 0.3);
        }
        drawSolid( Float4( hoffbase, voffset + i * itemheight, hoffbase + boxwidth, voffset + i * itemheight + fontsize + boxborder * 2 ) );
        drawBox( Float4( hoffbase, voffset + i * itemheight, hoffbase + boxwidth, voffset + i * itemheight + fontsize + boxborder * 2 ), boxthick );
        setColor(1.0, 1.0, 1.0);
        drawText( node.branches[i].name.c_str(), fontsize, hoffbase + boxborder, voffset + i * itemheight + boxborder );
        {
            int dispmode = node.branches[i].displaymode;
            if(dispmode == HierarchyNode::HNDM_COSTUNIQUE) {
                if(node.branches[i].type == HierarchyNode::HNT_WEAPON && !player->hasUpgrade(node.branches[i].upgrade))
                    dispmode = HierarchyNode::HNDM_COST;
                if(node.branches[i].type == HierarchyNode::HNT_GLORY && player->glory != node.branches[i].glory)
                    dispmode = HierarchyNode::HNDM_COST;
            }
            if(dispmode == HierarchyNode::HNDM_BLANK) {
            } else if(dispmode == HierarchyNode::HNDM_COST) {
                drawText( StringPrintf("%6d", node.branches[i].cost), fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder );
            } else if(dispmode == HierarchyNode::HNDM_PACK) {
                drawText( StringPrintf("%dpk", node.branches[i].quantity), fontsize, hoffbase + pricehpos, voffset + i * itemheight + boxborder );
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
    
    if(keys.f.repeat && getCurNode().buyable) {
        // Player is trying to buy something!
        
        // First check to see if we're allowed to buy it
        bool canBuy = false;
        if(getCurNode().type == HierarchyNode::HNT_DONE) {
            canBuy = true;
        } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
            if(player->cash >= getCurNode().cost && !player->hasUpgrade(getCurNode().upgrade))
                canBuy = true;
        } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
            if(player->cash >= getCurNode().cost)
                canBuy = true;
            if(player->weapon != getCurNode().weapon && player->cash + player->resellAmmoValue() >= getCurNode().cost)
                canBuy = true;
        } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
            if(player->cash >= getCurNode().cost && getCurNode().glory != player->glory)
                canBuy = true;
        } else {
            CHECK(0);
        }
        
        // If so, buy it
        if(canBuy) {
            player->cash -= getCurNode().cost;
            if(getCurNode().type == HierarchyNode::HNT_DONE) {
                return true;
            } else if(getCurNode().type == HierarchyNode::HNT_UPGRADE) {
                player->upgrades.push_back(getCurNode().upgrade);
                player->reCalculate();
            } else if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
                if(player->weapon != getCurNode().weapon) {
                    if(player->shotsLeft != -1)
                        player->cash += player->resellAmmoValue();
                    player->weapon = getCurNode().weapon;
                    player->shotsLeft = 0;
                }
                player->shotsLeft += getCurNode().quantity;
                if(player->weapon == defaultWeapon())
                    player->shotsLeft = -1;
            } else if(getCurNode().type == HierarchyNode::HNT_GLORY) {
                player->glory = getCurNode().glory;
            } else {
                CHECK(0);
            }
        }
        
    }
    return false;
}

void Shop::ai(Ai *ais) const {
    if(ais)
        ais->updateShop(player);
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
    setColor(player->color * 0.5);
    {
        const float ofs = 8;
        drawDvec2(player->faction_symb, Float4(ofs, ofs, 125 - ofs, 100 - ofs), 0.5);
    }
    renderNode(itemDbRoot(), 0);
    if(getCurNode().type == HierarchyNode::HNT_WEAPON) {
        drawText("damage per second", 2, 1.5, 17.5);
        drawText(StringPrintf("%20.4f", getCurNode().weapon->getDamagePerSecond()), 2, 1.5, 20.5);
        drawText("cost per damage", 2, 1.5, 23.5);
        drawText(StringPrintf("%20.4f", getCurNode().weapon->getCostPerDamage()), 2, 1.5, 26.5);
    }
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
            if(playersymbol[i] == -1) { // if player hasn't chosen symbol yet
                fireHeld[i] = 0;
                playerpos[i].x += deadzone(keys[i].x, keys[i].y, 0, 0.2) * 4;
                playerpos[i].y -= deadzone(keys[i].y, keys[i].x, 0, 0.2) * 4;
                int targetInside = -1;
                for(int j = 0; j < symbolpos.size(); j++)
                    if(isinside(symbolpos[j], playerpos[i]) && !count(playersymbol.begin(), playersymbol.end(), j))
                        targetInside = j;
                if(targetInside != -1) {                        
                    for(int j = 0; j < keys[i].keys.size(); j++) {
                        if(keys[i].keys[j].repeat) {
                            playersymbol[i] = targetInside;
                            playerkey[i] = j;
                        }
                    }
                }
            } else {    // if player has chosen symbol
                {
                    int opm = playermode[i];
                    if(keys[i].l.repeat) {
                        do {
                            playermode[i]--;
                            playermode[i] += KSAX_END;
                            playermode[i] %= KSAX_END;
                        } while(ksax_minaxis[playermode[i]] > keys[i].axes.size());
                    }
                    if(keys[i].r.repeat) {
                        do {
                            playermode[i]++;
                            playermode[i] += KSAX_END;
                            playermode[i] %= KSAX_END;
                        } while(ksax_minaxis[playermode[i]] > keys[i].axes.size());
                    }
                    if(playermode[i] != opm)
                        fireHeld[i] = 0;    // just for a bit of added safety
                }
                if(playerkey[i] != -1 && keys[i].keys[playerkey[i]].down) {
                    fireHeld[i]++;
                } else {
                    fireHeld[i] = 0;
                }
                if(fireHeld[i] > 60)
                    fireHeld[i] = 60;
            }
        }
        if(count(fireHeld.begin(), fireHeld.end(), 60) == playerkey.size() - count(playerkey.begin(), playerkey.end(), -1) && count(fireHeld.begin(), fireHeld.end(), 60) >= 2) {
            mode = MGM_SHOP;
            currentShop = 0;
            playerdata.clear();
            playerdata.resize(count(fireHeld.begin(), fireHeld.end(), 60));
            int pid = 0;
            for(int i = 0; i < playersymbol.size(); i++) {
                if(playersymbol[i] != -1) {
                    playerdata[pid].color = factions[playersymbol[i]].color;
                    playerdata[pid].faction_symb = symbols[playersymbol[i]];
                    pid++;
                }
            }
            CHECK(pid == playerdata.size());
            shop = Shop(&playerdata[0]);
        }
    } else if(mode == MGM_SHOP) {
        if(currentShop == -1) {
            for(int i = 0; i < playerdata.size(); i++)
                if(genKeystates(keys, playermode)[i].f.repeat)
                    checked[i] = true;
            if(count(checked.begin(), checked.end(), false) == 0) {
                for(int i = 0; i < playerdata.size(); i++)
                    playerdata[i].cash += lrCash[i];
                currentShop = 0;
                shop = Shop(&playerdata[0]);
            }
        } else if(shop.runTick(genKeystates(keys, playermode)[currentShop])) {
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
                findLevels(playerdata.size());
                game = Game(&playerdata, levels[int(frand() * levels.size())]);
            }
        }
    } else if(mode == MGM_PLAY) {
        if(game.runTick(genKeystates(keys, playermode))) {
            gameround++;
            if(gameround % roundsBetweenShop == 0) {
                mode = MGM_SHOP;
                currentShop = -1;
                calculateLrStats();
                checked.clear();
                checked.resize(playerdata.size());
            } else {
                float firepower = game.firepowerSpent;
                game = Game(&playerdata, levels[int(frand() * levels.size())]);
                game.firepowerSpent = firepower;
            }
        }
    } else {
        CHECK(0);
    }
    return false;
}

vector<Ai *> distillAi(const vector<Ai *> &ai, const vector<int> &playersymbol) {
    CHECK(ai.size() == playersymbol.size());
    vector<Ai *> rv;
    for(int i = 0; i < playersymbol.size(); i++)
        if(playersymbol[i] != -1)
            rv.push_back(ai[i]);
    return rv;
}
    
void Metagame::ai(const vector<Ai *> &ai) const {
    CHECK(ai.size() == playerpos.size());
    if(mode == MGM_PLAYERCHOOSE) {
        for(int i = 0; i < ai.size(); i++) {
            if(ai[i]) {
                int mode = -1;
                if(playersymbol[i] != -1)
                    mode = playermode[i];
                ai[i]->updateCharacterChoice(symbolpos, playersymbol, playerpos[i], mode, i);
            }
        }
    } else if(mode == MGM_SHOP) {
        if(currentShop == -1) {
            for(int i = 0; i < ai.size(); i++)
                if(ai[i])
                    ai[i]->updateWaitingForReport();
        } else {
            CHECK(currentShop >= 0 && currentShop < playerdata.size());
            shop.ai(distillAi(ai, playersymbol)[currentShop]);
        }
    } else if(mode == MGM_PLAY) {
        game.ai(distillAi(ai, playersymbol));
    } else {
        CHECK(0);
    }
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
                float ye = min(600. / playersymbol.size(), 100.);
                Float4 box( 0, ye * i, ye, ye + ye * i );
                drawDvec2(symbols[playersymbol[i]], Float4(box.sx + ye / 10, box.sy + ye / 10, box.ex - ye / 10, box.ey - ye / 10), 1.0);
                setColor(Color(1.0, 1.0, 1.0) / 60 * fireHeld[i]);
                drawBox(Float4(box.sx + ye / 20, box.sy + ye / 20, box.ex - ye / 20, box.ey - ye / 20), 1);
                setColor(Color(0.8, 0.8, 0.8));
                drawText(ksax_names[playermode[i]], 20, ye, ye * (i + 1. / 20));
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
            setColor(1.0, 1.0, 1.0);
            drawJustifiedText("waiting", 30, 400, 400, TEXT_CENTER, TEXT_MIN);
            int notdone = count(checked.begin(), checked.end(), false);
            CHECK(notdone);
            int cpos = 0;
            float increment = 800.0 / notdone;
            for(int i = 0; i < checked.size(); i++) {
                if(!checked[i]) {
                    setColor(playerdata[i].color);
                    drawDvec2(playerdata[i].faction_symb, Float4(cpos * increment, 440, (cpos + 1) * increment, 580), 1);
                    cpos++;
                }
            }
        } else {
            shop.renderToScreen();
        }
    } else if(mode == MGM_PLAY) {
        game.renderToScreen();
        if(!controls_users()) {        
            setColor(1.0, 1.0, 1.0);
            setZoom(0, 0, 100);
            drawText(StringPrintf("round %d", gameround), 2, 5, 82);
        }
    } else {
        CHECK(0);
    }
}

vector<Keystates> Metagame::genKeystates(const vector<Controller> &keys, const vector<int> &modes) {
    vector<Keystates> kst(playerdata.size());
    int pid = 0;
    for(int i = 0; i < playerkey.size(); i++) {
        if(playerkey[i] != -1) {
            kst[pid].u = keys[i].u;
            kst[pid].d = keys[i].d;
            kst[pid].l = keys[i].l;
            kst[pid].r = keys[i].r;
            kst[pid].f = keys[i].keys[playerkey[i]];
            kst[pid].axmode = modes[i];
            if(kst[pid].axmode == KSAX_UDLR || kst[pid].axmode == KSAX_ABSOLUTE) {
                kst[pid].ax[0] = keys[i].x;
                kst[pid].ax[1] = keys[i].y;
            } else if(kst[pid].axmode == KSAX_TANK) {
                CHECK(keys[i].axes.size() >= 4);
                kst[pid].ax[0] = keys[i].axes[1];
                kst[pid].ax[1] = keys[i].axes[2];
            } else {
                CHECK(0);
            }
            CHECK(keys[i].x >= -1 && keys[i].x <= 1);
            CHECK(keys[i].y >= -1 && keys[i].y <= 1);
            pid++;
        }
    }
    CHECK(pid == kst.size());
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
    float totalReturn = 75 * pow(1.08, gameround) * playerdata.size() * roundsBetweenShop + game.firepowerSpent * 0.8;
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

void Metagame::findLevels(int playercount) {
    if(!levels.size()) {
        ifstream ifs("data/levels/levellist.txt");
        string line;
        while(getLineStripped(ifs, line)) {
            Level lev = loadLevel("data/levels/" + line);
            if(lev.playersValid.count(playercount))
                levels.push_back(lev);
        }
        dprintf("Got %d usable levels\n", levels.size());
        CHECK(levels.size());
    }
}

// not a valid state
Metagame::Metagame() {
}

Metagame::Metagame(int playercount, int in_roundsBetweenShop) {

    const Float2 cent(450, 300);
    
    roundsBetweenShop = in_roundsBetweenShop;
    CHECK(roundsBetweenShop >= 1);
    
    playerkey.clear();
    playersymbol.clear();
    playerpos.clear();
    playermode.clear();
    playerkey.resize(playercount, -1);
    playersymbol.resize(playercount, -1);
    playerpos.resize(playercount, cent);
    playermode.resize(playercount, KSAX_UDLR);
    fireHeld.resize(playercount);
    
    for(int i = 0; i < factioncount; i++) {
        symbols.push_back(loadDvec2(factions[i].filename));
    }
    
    for(int i = 0; i < 4; i++) {
        symbolpos.push_back( boxaround( makeAngle(PI * 2 * i / 4) * 100 + cent, 50 ) );
    }
    
    for(int i = 4; i < symbols.size(); i++) {
        symbolpos.push_back( boxaround( makeAngle(PI * 2 * ( i - 4 ) / ( symbols.size() - 4 )) * 225 + cent, 50 ) );
    }
    
    mode = MGM_PLAYERCHOOSE;
    
    gameround = 0;

}
