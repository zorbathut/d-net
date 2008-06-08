#ifndef DNET_METAGAME_CONFIG
#define DNET_METAGAME_CONFIG

#include "input.h"
#include "itemdb.h"
#include "smartptr.h"

using namespace std;

// This is only really used for the "test" screen, and I don't want this to be considered a "full dependency" because it kind of isn't.
class GamePackage;

class FactionState {
public:
  bool taken;
  Coord4 compass_location;
  const IDBFaction *faction;
};

enum { SETTING_BUTTONS, SETTING_TEST, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "Setup", "Test", "Done" };

enum { CHOICE_FIRSTPASS, CHOICE_ACTIVE, CHOICE_IDLE };

struct PlayerMenuState {
public:
  Coord2 compasspos;
  FactionState *faction;
  int current_faction_over;
  int current_faction_over_duration;

  int settingmode;
  int choicemode;

  int setting_button_current;
  vector<int> buttons;
  vector<int> axes;
  vector<char> axes_invert;

  AxisType setting_axistype;

  smart_ptr<GamePackage> test_game;
  
  Keystates genKeystate(const Controller &keys) const;

  void reset_controls();

  PlayerMenuState();
  ~PlayerMenuState();
};

bool runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions, const ControlConsts &ck);
void runSettingRender(const PlayerMenuState &pms, const ControlConsts &cc);  // kind of grim, second parameter is text description of what buttons are available

void adler(Adler32 *adl, const FactionState &pms);
void adler(Adler32 *adl, const PlayerMenuState &pms);

#endif
