#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "util.h"
#include "game.h"
#include "gfx.h"

#include <vector>
#include <string>

using namespace std;

class ShopNode {
public:
    vector<ShopNode> branches;

    string name;
    int cost;
    bool showcost;
    bool choosable;

    ShopNode();
    ShopNode(const string &name, int cost, bool showcost, bool choosable);
};

class Shop {
private:
    Player *player;

    ShopNode root;
    vector<int> curloc;

    ShopNode &getCurNode();
    ShopNode &getCategoryNode();

    void renderNode(const ShopNode &node, int depth) const;

    void recreateShopNetwork();

public:
    bool runTick( const Keystates &keys );
    void renderToScreen() const;

    Shop();
    Shop(Player *player);
};

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
    
    Shop shop;
    
    vector<Player> playerdata;

public:

	void renderToScreen() const;
	bool runTick( const vector< Controller > &keys );

    vector<Keystates> genKeystates(const vector<Controller> &keys);

    Metagame();
	Metagame(int playercount);

};

#endif
