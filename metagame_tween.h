#ifndef DNET_METAGAME_TWEEN
#define DNET_METAGAME_TWEEN

#include "metagame_config.h"
#include "shop.h"
#include "audio.h"

#include <boost/noncopyable.hpp>

using namespace std;

class PersistentData {
public:
  // Player data and reorganization functions
  vector<Player> &players();
  const vector<Player> &players() const;

  vector<Keystates> genKeystates(const vector<Controller> &keys) const;

  vector<Ai *> distillAi(const vector<Ai *> &ais) const;

  void setFactionMode(int faction_mode);

  // State accessors
  bool isPlayerChoose() const;

  // Main loop
  enum PDRTR { PDRTR_CONTINUE, PDRTR_PLAY, PDRTR_EXIT };
  PDRTR tick(const vector<Controller> &keys);
  void ai(const vector<Ai *> &ais, const vector<bool> &isHuman) const;
  bool isWaitingOnAi() const;
  void render() const;

  void checksum(Adler32 *adl) const;  

  // Triggers
  void divvyCash(int rounds = -1); // also sets to "result" mode
  void endgame(int rounds, bool playing);
  void startAtNormalShop();

  // Constructor
  PersistentData(const vector<bool> &human, Money startingcash, Coord multiple, int roundsbetweenshop, int rounds_until_end);
  void instant_action_init(const ControlConsts &ck);

private:
  // Persistent state
  enum { TM_PLAYERCHOOSE, TM_RESULTS, TM_SHOP, TM_GAMEEND };
  int mode;
  
  // Player data
  vector<Player> playerdata;
  vector<PlayerMenuState> pms;  // heh.
  vector<int> playerid; // from pms to playerdata ID. EVERYTHING ELSE is still in pms mode, where the user ID (from the Controller vector) is authoritative
  vector<bool> humans;
  
  vector<FactionState> factions;
  int faction_mode;
  
  // Temporaries for scoring
  vector<vector<Coord> > lrCategory;
  vector<Coord> lrPlayer;
  vector<Money> lrCash;
  Money lrBaseCash;
  vector<bool> checked;
  
  Money baseStartingCash;
  Coord multiplePerRound;
  
  Money newPlayerStartingCash;
  Money highestPlayerCash;
  
  Coord newPlayerDamageDone;
  Coord newPlayerKills;
  Coord newPlayerWins;
  
  // Tween layout info
  public:
  struct Slot {
    enum { CHOOSE, SHOP, RESULTS, QUITCONFIRM, SETTINGS, GAMEEND, EMPTY };
    int type;
    int pid;
    Shop shop;
  };
  private:
  Slot slot[4];
  int slot_count; // 1 or 4
  
  // Round count data
  int roundsbetweenshop;
  int shopcycles;
  int rounds_until_end;
  
  // Shop player state
  void reset();
    // Persistent
    vector<bool> sps_shopped;

    // State
    enum { SPS_IDLE, SPS_CHOOSING, SPS_PENDING, SPS_ACTIVE, SPS_DONE, SPS_END };
    vector<int> sps_playermode;
    
    // Quitconfirm cursor location
    vector<int> sps_quitconfirm;

    // Choosing only
    vector<Coord2> sps_playerpos;
    
    // Sound timeout
    vector<int> sps_soundtimeout;
    
    // Pending only
    vector<pair<int, int> > sps_queue;
    
    const IDBFaction *btt_notify;
    int btt_frames_left;

  // Slot functions
  bool tickSlot(int slotid, const vector<Controller> &controllers);
  void renderSlot(int slotid) const;
  
  bool isUnfinished(int id) const;
  vector<const IDBFaction *> getUnfinishedFactions() const;
  vector<const IDBFaction *> getUnfinishedHumanFactions() const;
  bool onlyAiUnfinished() const;
  
  vector<pair<int, pair<Coord, Coord> > > ranges;
  vector<pair<Coord, Coord> > range_targets;
  void resetRanges();
  Coord2 targetCoords(int target) const;

  // Helper functions
  void drawMultibar(const vector<Coord> &sizes, const Float4 &dimensions, const vector<int> *roundcount = NULL) const;
  
  HierarchyNode generateShopHierarchy() const;
  void attemptQueueSound(int player, const Sound *sound);
  
  void destroyPlayer(int pid); // DESTROY
  void enterGameEnd();
  
  int getExpectedPlayercount() const;
  int getHumanCount() const;
  int getAiCount() const;
};

#endif
