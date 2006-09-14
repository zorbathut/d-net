#ifndef DNET_METAGAME_TWEEN
#define DNET_METAGAME_TWEEN

#include "metagame_config.h"
#include "shop.h"

class PersistentData {
public:
  // Player data and reorganization functions
  vector<Player> &players();

  vector<Keystates> genKeystates(const vector<Controller> &keys) const;

  void ai(const vector<Ai *> &ais) const;
  vector<Ai *> distillAi(const vector<Ai *> &ais) const;

  // State accessors
  bool isPlayerChoose() const;

  // Main loop
  bool tick(const vector< Controller > &keys);
  void render() const;

  // Slot functions
  void renderSlot(int slotid) const;

  // Triggers
  void divvyCash(float firepowerSpent);

  // Constructor
  PersistentData(int playercount, int roundsbetweenshop);

private:
  // Persistent state (used for recognizing PLAYERCHOOSE mode only)
  enum { TM_PLAYERCHOOSE, TM_SHOP };
  int mode;
  
  // Player data
  vector<Player> playerdata;
  vector<PlayerMenuState> pms;  // heh.
  
  vector<FactionState> factions;
  
  // Temporaries for scoring
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<Money> lrCash;
  vector<bool> checked;
  
  // Tween layout info
  struct Slot {
    enum { CHOOSE, SHOP, RESULTS, EMPTY };
    int type;
    int pid;
    Shop shop;
  };
  Slot slot[4];
  int slot_count; // 1 or 4
  
  // Round count data
  int roundsbetweenshop;
  int shopcycles;

  // Zone functions
  void chooseInit(int pid, int loc);
  bool chooseTick(const vector<Controller> &keys);
  void chooseRender();

  // Helper functions
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;
};

#endif
