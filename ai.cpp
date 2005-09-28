
#include "ai.h"

#include "itemdb.h"

void Ai::updatePregame() {
    nextKeys.x = nextKeys.y = 0;
    nextKeys.keys[0].down = true;
}

void Ai::updateCharacterChoice(const vector<Float4> &factions, const vector<int> &playerfact, Float2 pos, int mode, int you) {
    if(mode == -1) {
        int targfact = -1;
        if(count(playerfact.begin(), playerfact.end(), you) == 0 && factions.size() > you) {
            targfact = you;
        } else {
            for(int i = 0; i < factions.size(); i++) {
                if(count(playerfact.begin(), playerfact.end(), i) == 0) {
                    targfact = i;
                    break;
                }
            }
        }
        if(targfact == -1) {
            nextKeys.x = nextKeys.y = 0;
            nextKeys.keys[0].down = true;
            return;
        }
        Float2 targpt = Float2((factions[targfact].sx + factions[targfact].ex) / 2, (factions[targfact].sy + factions[targfact].ey) / 2);
        //dprintf("player %d: target %f %f, pos %f %f\n", you, targpt.x, targpt.y, pos.x, pos.y);
        targpt -= pos;
        if(len(targpt) != 0)
            targpt = normalize(targpt);
        nextKeys.x = targpt.x;
        nextKeys.y = -targpt.y;
        nextKeys.keys[0].down = isinside(factions[targfact], pos);
    } else if(mode != KSAX_ABSOLUTE) {
        nextKeys.x = 1.0;
        nextKeys.y = 0;
        nextKeys.keys[0].down = false;
    } else {
        CHECK(mode == KSAX_ABSOLUTE);
        nextKeys.x = 0;
        nextKeys.y = 0;
        nextKeys.keys[0].down = true;
    }
}

Controller makeController(float x, float y, bool key) {
    Controller rv;
    rv.keys.resize(1);
    rv.x = x;
    rv.y = y;
    rv.keys[0].down = key;
    return rv;
}

void doMegaEnumWorker(const HierarchyNode &rt, vector<pair<int, vector<Controller> > > *weps, vector<pair<pair<int, const Upgrade*>, vector<Controller> > > *upgs, vector<Controller> *done, vector<Controller> path) {
    if(rt.type == HierarchyNode::HNT_CATEGORY) {
        path.push_back(makeController(1, 0, 0));
        for(int i = 0; i < rt.branches.size(); i++) {
            if(i)
                path.push_back(makeController(0, -1, 0));
            doMegaEnumWorker(rt.branches[i], weps, upgs, done, path);
        }
    } else if(rt.type == HierarchyNode::HNT_WEAPON) {
        weps->push_back(make_pair(rt.cost, path));
    } else if(rt.type == HierarchyNode::HNT_UPGRADE) {
        upgs->push_back(make_pair(make_pair(rt.cost, rt.upgrade), path));
    } else if(rt.type == HierarchyNode::HNT_DONE) {
        CHECK(done->size() == 0);
        *done = path;
    }
}

void doMegaEnum(const HierarchyNode &rt, vector<pair<int, vector<Controller> > > *weps, vector<pair<pair<int, const Upgrade*>, vector<Controller> > > *upgs, vector<Controller> *done) {
    for(int i = 0; i < rt.branches.size(); i++) {
        vector<Controller> tvd;
        for(int j = 0; j < i; j++)
            tvd.push_back(makeController(0, -1, 0));
        doMegaEnumWorker(rt.branches[i], weps, upgs, done, tvd);
    }
}

vector<Controller> reversecontroller(const vector<Controller> &in) {
    vector<Controller> rv;
    for(int i = 0; i < in.size(); i++)
        rv.push_back(makeController(-in[i].x, -in[i].y, in[i].keys[0].down));
    reverse(rv.begin(), rv.end());
    return rv;
}

void appendPurchases(deque<Controller> *dest, const vector<Controller> &src, int count) {
    dest->insert(dest->end(), src.begin(), src.end());
    for(int i = 0; i < count; i++)
        dest->push_back(makeController(0, 0, 1));
    vector<Controller> reversed = reversecontroller(src);
    dest->insert(dest->end(), reversed.begin(), reversed.end());
}

void Ai::updateShop(const Player *player) {
    if(shopQueue.size()) {
        nextKeys = shopQueue[0];
        shopQueue.pop_front();
        return;
    }
    CHECK(!shopdone);
    const HierarchyNode &rt = itemDbRoot();
    vector<pair<int, vector<Controller> > > weps;
    vector<pair<pair<int, const Upgrade *>, vector<Controller> > > upgs;
    vector<Controller> done;
    doMegaEnum(rt, &weps, &upgs, &done);
    dprintf("%d weps, %d upgs, %d donesize\n", weps.size(), upgs.size(), done.size());
    CHECK(weps.size());
    CHECK(done.size());
    sort(weps.begin(), weps.end());
    sort(upgs.begin(), upgs.end());
    int upgcash = player->cash / 2;
    int weapcash = player->cash;
    while(upgcash) {
        for(int i = 0; i < upgs.size(); i++) {
            if(player->hasUpgrade(upgs[i].first.second)) {
                upgs.erase(upgs.begin() + i);
                i--;
            }
        }
        int dlim = 0;
        while(dlim < upgs.size() && upgs[dlim].first.first <= upgcash)
            dlim++;
        if(dlim == 0)
            break;
        dlim = int(dlim * rng.frand());
        upgcash -= upgs[dlim].first.first;
        weapcash -= upgs[dlim].first.first;
        appendPurchases(&shopQueue, upgs[dlim].second, 1);
        upgs.erase(upgs.begin() + dlim);
    }
    if(weapcash) {
        int dlim = 0;
        while(dlim < weps.size() && weps[dlim].first <= weapcash)
            dlim++;
        dlim = int(dlim * rng.frand());
        int amount = 1;
        if(weps[dlim].first) {
            amount = weapcash / weps[dlim].first;
        }
        appendPurchases(&shopQueue, weps[dlim].second, amount);
    }
    shopQueue.insert(shopQueue.end(), done.begin(), done.end());
    shopQueue.push_back(makeController(0, 0, 1));
    for(int i = 0; i < shopQueue.size(); i++)
        dprintf("%f %f %d\n", shopQueue[i].x, shopQueue[i].y, shopQueue[i].keys[0].down);
    for(int i = 1; i < shopQueue.size(); i += 2)
        shopQueue.insert(shopQueue.begin() + i, makeController(0, 0, 0));
    shopdone = true;
    updateShop(player);
}

Controller Ai::getNextKeys() const {
    return nextKeys;
}

Ai::Ai() {
    nextKeys.keys.resize(1);
    shopdone = false;
}
