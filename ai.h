#ifndef DNET_AI
#define DNET_AI

#include "input.h"
#include "util.h"

class Ai {
private:
    Controller nextKeys;
public:
    
    void updatePregame();
    void updateCharacterChoice(const vector<Float4> &factions, const vector<int> &playerfact, Float2 pos, int mode, int you);

    Controller getNextKeys() const;

    Ai();

};

#endif
