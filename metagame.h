#ifndef DNET_METAGAME
#define DNET_METAGAME

#include "util.h"
#include "game.h"
#include "gfx.h"
#include "itemdb.h"
#include "level.h"

#include <vector>
#include <string>

using namespace std;

class Ai;

class Shop {
private:
  Player *player;

  vector<int> curloc;

  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCurNode() const;
  const HierarchyNode &getCategoryNode() const;

  void renderNode(const HierarchyNode &node, int depth) const;

public:
  bool runTick( const Keystates &keys );
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();
  Shop(Player *player);
};

// Here's some notes about the "choose your options" set of screens!
// First we prompt the player for "fire/accept" and "weapon/cancel" buttons.
// Then we prompt the player for axis type, with graphics.
// Then we prompt the player to choose which axes he wants to use.
// Then we let the player customize his system (if there is customization involved - there may not be.)
// Then we unlock the menu, and allow them to hold "ready" in much the same way as they currently do.
// I think this works.
/*
enum { SETTING_COMPASS, SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_AXISCHOOSE, SETTING_CUSTOMIZE, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "NULL", "Keys", "Mode", "Axis", "Cust", "Ready" };*/
enum { SETTING_COMPASS, SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "NULL", "Keys", "Mode", "Ready" };

const int setting_first = SETTING_BUTTONS;
const int setting_real_count = SETTING_LAST - setting_first;

enum { CHOICE_FIRSTPASS, CHOICE_ACTIVE, CHOICE_IDLE };

enum { BUTTON_ACCEPT, BUTTON_CANCEL };
const char * const button_names_a[] = { "Fire/", "Weapon/" };
const char * const button_names_b[] = { "  Accept", "  Cancel" };
const int button_count = sizeof(button_names_a) / sizeof(*button_names_a);

struct PlayerMenuState {
public:
  int settingmode;
  int choicemode;

  int setting_button_current;
  bool setting_button_reading;

  int setting_axistype;
  void traverse_axistype(int delta, int axes);

  int buttons[button_count];
  
  int firekey;
  FactionState *faction;
  Float2 compasspos;
  int axismode;

  int fireHeld;
  bool readyToPlay() const;

  PlayerMenuState();
  PlayerMenuState(Float2 cent);
};

class Metagame {
  
  int mode;
  int gameround;
  
  enum { MGM_PLAYERCHOOSE, MGM_SHOP, MGM_PLAY };
  int currentShop;
  
  vector<PlayerMenuState> pms;  // heh.
  
  vector<FactionState> factions;
  
  vector<Level> levels;
  
  Game game;
  
  Shop shop;
  
  vector<Player> playerdata;
  
  vector<vector<float> > lrCategory;
  vector<float> lrPlayer;
  vector<int> lrCash;
  vector<bool> checked;
  
  int roundsBetweenShop;

public:

  void renderToScreen() const;
  void ai(const vector<Ai *> &ai) const;
  bool runTick( const vector< Controller > &keys );

  void calculateLrStats();
  void drawMultibar(const vector<float> &sizes, const Float4 &dimensions) const;

  void findLevels(int playercount);

  Metagame();
  Metagame(int playercount, int roundsBetweenShop);

};

#endif
