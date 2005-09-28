#ifndef DNET_AI
#define DNET_AI

#include "input.h"
#include "util.h"
#include "game.h"
#include "rng.h"

#include <deque>

using namespace std;

class Ai {
private:
    deque<Controller> shopQueue;
    Controller nextKeys;
    Rng rng;
    bool shopdone;
public:
    
    void updatePregame();
    void updateCharacterChoice(const vector<Float4> &factions, const vector<int> &playerfact, Float2 pos, int mode, int you);
    void updateShop(const Player *player);

    Controller getNextKeys() const;

    Ai();

};

#endif
