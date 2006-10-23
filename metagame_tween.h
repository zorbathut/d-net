#ifndef DNET_METAGAME_TWEEN
#define DNET_METAGAME_TWEEN

#include "metagame_config.h"
#include "shop.h"

using namespace std;

class PersistentData {
public:
  // Player data and reorganization functions
  vector<Player> &players();
  const vector<Player> &players() const;

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
  void startAtNormalShop();

  // Constructor
  PersistentData(int playercount, int roundsbetweenshop);

private:
  // Persistent state
  enum { TM_PLAYERCHOOSE, TM_RESULTS, TM_SHOP };
  int mode;
  
  // Player data
  vector<Player> playerdata;
  vector<PlayerMenuState> pms;  // heh.
  vector<int> playerid; // from pms to playerdata ID
  
  vector<FactionState> factions;
  
  // Temporaries for scoring
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<Money> lrCash;
  Money lrBaseCash;
  Money lrFirepower;
  vector<bool> checked;
  
  Money newPlayerStartingCash;
  Money highestPlayerCash;
  
  // Tween layout info
  struct Slot {
    enum { CHOOSE, SHOP, RESULTS, QUITCONFIRM, SETTINGS, EMPTY };
    int type;
    int pid;
    Shop shop;
  };
  Slot slot[4];
  int slot_count; // 1 or 4
  
  // Round count data
  int roundsbetweenshop;
  int shopcycles;
  
  // Shop player state
  void reset();
    // Persistent
    vector<bool> sps_shopped;

    // State
    enum { SPS_IDLE, SPS_CHOOSING, SPS_PENDING, SPS_ACTIVE, SPS_DONE };
    vector<int> sps_playermode;
    
    // Quitconfirm cursor location
    vector<int> sps_quitconfirm;

    // Choosing only
    vector<Float2> sps_playerpos;
    
    // Pending only
    vector<pair<int, int> > sps_queue;
    
    const IDBFaction *btt_notify;
    int btt_frames_left;

  // Slot functions
  bool tickSlot(int slotid, const vector<Controller> &controllers);
  void renderSlot(int slotid) const;
  vector<const IDBFaction *> getUnfinishedFactions() const;
    
  vector<pair<int, pair<float, float> > > getRanges() const;
  Float2 targetCoords(int target) const;

  // Helper functions
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;
  
  HierarchyNode generateShopHierarchy() const;
};

#endif
