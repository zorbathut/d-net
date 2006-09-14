#ifndef DNET_METAGAME_TWEEN
#define DNET_METAGAME_TWEEN

#include "metagame_config.h"
#include "shop.h"

class PersistentData {
public:
  // Player data and reorganization functions
  vector<Player> &players();

  vector<Keystates> genKeystates(const vector<Controller> &keys) const;

  vector<Ai *> distillAi(const vector<Ai *> &ais) const;

  // State accessors
  bool isPlayerChoose() const;

  // Main loop
  bool tick(const vector< Controller > &keys);
  void ai(const vector<Ai *> &ais) const;
  void render() const;

  // Triggers
  void divvyCash(float firepowerSpent); // also sets to "result" mode

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

  // Slot functions
  bool tickSlot(int slotid, const vector<Controller> &controllers);
  void renderSlot(int slotid) const;

  // Helper functions
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;
};

#endif
