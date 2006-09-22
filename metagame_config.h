#ifndef DNET_METAGAME_CONFIG
#define DNET_METAGAME_CONFIG

#include "input.h"
#include "itemdb.h"
#include "game_ai.h"

using namespace std;

// This is only really used for the "test" screen, and I don't want this to be considered a "full dependency" because it kind of isn't.
class GamePackage;
  
class GameAiAxisRotater : public GameAi { // todo: get this dependency out
private:
  class Config {
  public:
    int type;
  
    bool ax[2];
    int tax[2];
  };

  class Randomater {
    float current;
    int fleft;
    
  public:
    bool smooth;
  
    float next();
    Randomater();
  };
  
  vector<Randomater> rands;
  vector<float> next;
  
  Config config;

  void updateGameWork(const vector<Tank> &players, int me);
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos);
public:
  
  static Config steeringConfig(bool ax0, bool ax1);
  static Config absoluteConfig();
  static Config tankConfig(int axlsrc, int axrsrc);
  
  void updateConfig(const Config &conf);
  Float2 getControls() const;

  GameAiAxisRotater(const Config &conf);
};

class FactionState {
public:
  bool taken;
  Float4 compass_location;
  const IDBFaction *faction;
};

enum { SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_AXISCHOOSE, SETTING_TEST, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "Keys", "Mode", "Directions", "Test", "Ready" };
const char * const setting_names_detailed[] = { "Set your keys", "Control mode", "Set directions", "Test controls", "Ready" };

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
  int setting_axistype_curchoice;

  int setting_axistype_demo_curframe;
  int setting_axistype_demo_aiframe;
  smart_ptr<GamePackage> setting_axistype_demo;
  smart_ptr<GameAiAxisRotater> setting_axistype_demo_ai;
  void createNewAxistypeDemo();

  smart_ptr<GamePackage> test_game;
  
  int fireHeld;
  bool readyToPlay() const;
  
  Keystates genKeystate(const Controller &keys) const;

  PlayerMenuState();
  ~PlayerMenuState();
};

void runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions);
void runSettingRender(const PlayerMenuState &pms);

#endif
