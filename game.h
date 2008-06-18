#ifndef DNET_GAME
#define DNET_GAME

#include "collide.h"
#include "game_util.h"
#include "input.h"
#include "level.h"
#include "player.h"

using namespace std;

class Collider;
class GameAi;

class BombardmentState {
public:
  enum { BS_OFF, BS_SPAWNING, BS_ACTIVE, BS_FIRING, BS_COOLDOWN, BS_LAST };
  Coord2 pos;
  Coord d;
  int timer;
  int state;
  
  BombardmentState() {
    pos = Coord2(0, 0);
    d = 0;
    timer = 0;
    state = BS_OFF;
  }
};

enum {GMODE_STANDARD, GMODE_CHOICE, GMODE_TEST, GMODE_DEMO, GMODE_CENTERED_DEMO, GMODE_TITLESCREEN, GMODE_LAST};
enum {FACTION_NULL, FACTION_SMALL, FACTION_MEDIUM, FACTION_BIG, FACTION_LAST};
enum {DEMOPLAYER_QUIET, DEMOPLAYER_DPS, DEMOPLAYER_DPH, DEMOPLAYER_DPC, DEMOPLAYER_BOMBSIGHT, DEMOPLAYER_LAST};
// Damage per second, damage per hit, damage per cycle (with cycle sent to game.h manually)

class GameMetacontext {
private:
  const vector<const IDBFaction *> *wins;
  int roundsPerShop;

public:
  const vector<const IDBFaction *> &getWins() const;
  int getRoundsPerShop() const;

  bool hasMetacontext() const;

  GameMetacontext();
  GameMetacontext(const vector<const IDBFaction *> &wins, int roundsPerShop);
};

class Game : boost::noncopyable {
public:
  
  void initStandard(vector<Player> *playerdata, const Level &level, Rng *rng, bool instant_action_keys);
  void initChoice(vector<Player> *playerdata, Rng *rng);
  void initTest(Player *playerdata, const Float4 &bounds);
  void initDemo(vector<Player> *playerdata, float boxradi, const float *xps, const float *yps, const float *facing, const int *teams, const int *modes, bool blockades, Float2 hudpos, Recorder *recorder);
  void initCenteredDemo(Player *playerdata, float zoom);
  void initTitlescreen(vector<Player> *playerdata, Rng *rng);

  bool runTick(const vector<Keystates> &keys, bool confused, const vector<Player *> &players, const vector<bool> &human, Rng *rng);
  void ai(const vector<GameAi *> &ais) const;
  void renderToScreen(const vector<const Player *> &players, GameMetacontext gmc) const;

  void checksum(Adler32 *adl) const;

  int winningTeam() const;
  vector<int> teamBreakdown() const;

  int frameCount() const;

  // used for demos
  Coord2 queryPlayerLocation(int id) const;
  void kill(int id);
  void respawnPlayer(int id, Coord2 pos, Coord facing);

  void addStatCycle();  // used for damage-per-cycle calculations
  vector<pair<Float2, pair<float, string> > > getStats() const;
  void dumpMetastats(Recorder *recorder) const;
  
  void runShopcache(const IDBShopcache &cache, const vector<const Player *> &players, const Player *adjuster);

  Game();

private:
  
  void initCommon(const vector<Player*> &playerdata, const vector<Color> &in_colors, const vector<vector<Coord2> > &level, bool smashable);
  void initRandomTankPlacement(const map<int, vector<pair<Coord2, Coord> > > &player_starts, Rng *rng);

  void checkLevelSanity() const;
  
  void addTankStatusText(int tankid, const string &text, float duration);

  int frameNm;
  int frameNmToStart;
  bool freezeUntilStart;
  int framesSinceOneLeft;

  vector<Team> teams;

  vector<Tank> tanks;
  vector<BombardmentState> bombards;
  vector<ProjectilePack> projectiles;
  vector<smart_ptr<GfxEffects> > gfxeffects;
  Gamemap gamemap;

  Collider collider;

  vector<pair<vector<Coord2>, Color> > zones;

  Float2 zoom_center;
  Float2 zoom_size;

  Float2 zoom_speed;
  
  int gamemode;
  
  Float4 clear;
  
  float centereddemo_zoom;
  Float4 titlescreen_size;

  vector<int> demo_playermodes;
  float demo_boxradi; // used for font sizes
  
  int demo_cycles;
  Float2 demo_hudpos;
  Recorder *demo_recorder;
  
  Coord bombardment_tier;
  Coord getBombardmentIncreasePerSec() const;
  Coord getTimeUntilBombardmentUpgrade() const;
  
  bool instant_action_keys;
  int confused_cooldown;
  bool confused_bombing;
};

class GamePackage {
public:
  vector<Player> players;
  Game game;

  bool runTick(const vector<Keystates> &keys, Rng *rng);
  void runTickWithAi(const vector<GameAi *> &gai, Rng *rng);
  void renderToScreen() const;

  void runShopcache(const IDBShopcache &sc, int adjuster);
};

void adler(Adler32 *adler, const BombardmentState &bs);

#endif
