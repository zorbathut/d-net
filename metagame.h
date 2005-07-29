#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "util.h"
#include "game.h"
#include "gfx.h"

#include <vector>

using namespace std;

class Metagame {
    
    int mode;
    
    enum { MGM_PLAYERCHOOSE, MGM_SHOP, MGM_PLAY };
    int currentShop;

    vector< int > playerkey;
    vector< int > playersymbol;
    vector< Float2 > playerpos;
    
    vector<VectorObject> symbols;
    vector<Float4> symbolpos;
    
    Game game;
    
    vector<Player> playerdata;

public:

	void renderToScreen() const;
	bool runTick( const vector< Controller > &keys );

    vector<Keystates> genKeystates(const vector<Controller> &keys);

    Metagame();
	Metagame(int playercount);

};

#endif
