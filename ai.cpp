
#include "ai.h"

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
        dprintf("player %d: target %f %f, pos %f %f\n", you, targpt.x, targpt.y, pos.x, pos.y);
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

Controller Ai::getNextKeys() const {
    return nextKeys;
}

Ai::Ai() {
    nextKeys.keys.resize(1);
}
