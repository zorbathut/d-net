#ifndef DNET_METAGAME_TWEEN
#define DNET_METAGAME_TWEEN

#include "metagame_config.h"
#include "shop.h"

class PersistentData {
  enum { TM_PLAYERCHOOSE, TM_SHOP };
  int mode;
  
  vector<Player> playerdata;
    
  Shop shop;
  int currentShop;
  
  vector<PlayerMenuState> pms;  // heh.
  
  vector<FactionState> factions;
  
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<Money> lrCash;
  vector<bool> checked;
  
  int roundsbetweenshop;
  int shopcycles;
  
public:
  bool isPlayerChoose() const;

  vector<Player> &players();

  vector<Keystates> genKeystates(const vector<Controller> &keys) const;

  bool tick(const vector< Controller > &keys);
  void render() const;

  void ai(const vector<Ai *> &ais) const;
  vector<Ai *> distillAi(const vector<Ai *> &ais) const;

  void divvyCash(float firepowerSpent);

  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;

  PersistentData(int playercount, int roundsbetweenshop);
};

#endif
