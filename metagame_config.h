#ifndef DNET_METAGAME_CONFIG
#define DNET_METAGAME_CONFIG

#include "float.h"
#include "itemdb.h"
#include "game.h"
#include "player.h"

class FactionState {
public:
  bool taken;
  Float4 compass_location;
  const IDBFaction *faction;
};

enum { SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_AXISCHOOSE, SETTING_TEST, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "Keys", "Mode", "Axis", "Test", "Ready" };

enum { CHOICE_FIRSTPASS, CHOICE_ACTIVE, CHOICE_IDLE };

//enum { BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_FIRE1, BUTTON_FIRE2, BUTTON_SWITCH1, BUTTON_SWITCH2, BUTTON_LAST };
//const char * const button_names[] = { "Accept", "Cancel", "Fire 1", "Fire 2", "Switch 1", "Switch 2" };

enum { BUTTON_ACCEPT, BUTTON_CANCEL, BUTTON_LAST };
const char * const button_names[] = { "Fire/", "Weapon/" };

struct PlayerMenuState {
public:
  Float2 compasspos;
  FactionState *faction;

  int settingmode;
  int choicemode;

  int setting_button_current;
  bool setting_button_reading;
  int buttons[BUTTON_LAST];

  int setting_axis_current;
  bool setting_axis_reading;
  int axes[2];  // Right now, everything uses 2 axes.
  bool axes_invert[2];

  int setting_axistype;
  void traverse_axistype(int delta, int axes);

  Game *test_game;
  Player *test_player;
  
  int fireHeld;
  bool readyToPlay() const;

  PlayerMenuState();
  PlayerMenuState(Float2 cent);
};

vector<Keystates> genKeystates(const vector<Controller> &keys, const vector<PlayerMenuState> &modes);

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions);
void runSettingRender(const PlayerMenuState &pms);

#endif
