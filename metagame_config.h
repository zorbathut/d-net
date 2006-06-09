#ifndef DNET_METAGAME_CONFIG
#define DNET_METAGAME_CONFIG

#include "input.h"
#include "itemdb.h"

using namespace std;

// These are only really used for the "test" screen, and I don't want this to be considered a "full dependency" because it kind of isn't.
class Game;
class Player;

class FactionState {
public:
  bool taken;
  Float4 compass_location;
  const IDBFaction *faction;
};

enum { SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_AXISCHOOSE, SETTING_TEST, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "Keys", "Mode", "Axis", "Test", "Ready" };

enum { CHOICE_FIRSTPASS, CHOICE_ACTIVE, CHOICE_IDLE };

struct PlayerMenuState {
public:
  Float2 compasspos;
  FactionState *faction;

  int settingmode;
  int choicemode;

  int setting_button_current;
  bool setting_button_reading;
  vector<int> buttons;

  int setting_axis_current;
  bool setting_axis_reading;
  vector<int> axes;
  vector<char> axes_invert;

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
