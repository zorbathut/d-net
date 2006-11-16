#ifndef DNET_GAME
#define DNET_GAME

#include "collide.h"
#include "game_projectile.h"
#include "input.h"
#include "noncopyable.h"
#include "rng.h"

using namespace std;

class Collider;
class GameAi;

class BombardmentState {
public:
  enum { BS_OFF, BS_SPAWNING, BS_ACTIVE, BS_FIRING, BS_COOLDOWN, BS_LAST };
  Coord2 pos;
  int timer;
  int state;
  
  int framesSinceLastSwitch;
  
  BombardmentState() {
    pos = Coord2(0, 0);
    timer = 0;
    framesSinceLastSwitch = 0;
    state = BS_OFF;
  }
};

enum {GMODE_STANDARD, GMODE_CHOICE, GMODE_TEST, GMODE_DEMO, GMODE_CENTERED_DEMO, GMODE_LAST};
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
  
  void initStandard(vector<Player> *playerdata, const Level &level, Rng *rng);
  void initChoice(vector<Player> *playerdata, Rng *rng);
  void initTest(Player *playerdata, const Float4 &bounds);
  void initDemo(vector<Player> *playerdata, float boxradi, const float *xps, const float *yps, const float *facing, const int *modes, bool blockades, Float2 hudpos, Recorder *recorder);
  void initCenteredDemo(Player *playerdata, float zoom);

  bool runTick(const vector<Keystates> &keys, const vector<Player *> &players, Rng *rng);
  void ai(const vector<GameAi *> &ais) const;
  void renderToScreen(const vector<const Player *> &players, GameMetacontext gmc) const;

  int winningTeam() const;
  vector<int> teamBreakdown() const;

  int frameCount() const;

  // used for demos
  Coord2 queryPlayerLocation(int id) const;
  void kill(int id);
  void respawnPlayer(int id, Coord2 pos, float facing);

  void addStatCycle();  // used for damage-per-cycle calculations

  float firepowerSpent;

  Game();

private:
  
  void initCommon(const vector<Player*> &playerdata, const vector<Color> &in_colors, const vector<vector<Coord2> > &level, bool smashable);
  void initRandomTankPlacement(const map<int, vector<pair<Coord2, float> > > &player_starts, Rng *rng);
  
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

  vector<int> demo_playermodes;
  float demo_boxradi; // used for font sizes
  
  int demo_hits;
  Float2 demo_hudpos;
  Recorder *demo_recorder;
  
  float bombardment_tier;
  float getBombardmentIncreasePerSec() const;
  float getTimeUntilBombardmentUpgrade() const;

};

class GamePackage {
public:
  vector<Player> players;
  Game game;

  bool runTick(const vector<Keystates> &keys, Rng *rng);
  void renderToScreen() const;
};

#endif
