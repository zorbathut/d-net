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
    enum { AGM_APPROACH, AGM_RETREAT, AGM_WANDER, AGM_BACKUP };
    deque<Controller> shopQueue;
    Controller nextKeys;
    Rng rng;
    bool shopdone;
    
    int gamemode;
    int targetplayer;
    Float2 targetdir;
    bool firing;
    
public:
    
    void updatePregame();
    void updateCharacterChoice(const vector<Float4> &factions, const vector<int> &playerfact, Float2 pos, int mode, int me);
    void updateShop(const Player *player);
    void updateGame(const vector<Coord4> &collide, const vector<Tank> &players, int me);
    void updateWaitingForReport();

    Controller getNextKeys() const;

    Ai();

};

#endif
