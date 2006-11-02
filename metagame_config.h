#ifndef DNET_METAGAME_CONFIG
#define DNET_METAGAME_CONFIG

#include "game_ai.h"
#include "itemdb.h"
#include "smartptr.h"

using namespace std;

// This is only really used for the "test" screen, and I don't want this to be considered a "full dependency" because it kind of isn't.
class GamePackage;
  
class GameAiAxisRotater : public GameAi { // todo: get this dependency out
public:
      
  class Config {
    friend class GameAiAxisRotater;
    
    int type;
  
    bool ax[2];
    int tax[2];
  };
  
  static Config steeringConfig(bool ax0, bool ax1);
  static Config absoluteConfig();
  static Config tankConfig(int axlsrc, int axrsrc);
  
  void updateConfig(const Config &conf);
  Float2 getControls() const;

  GameAiAxisRotater(const Config &conf, RngSeed seed);
  
private:
  class Randomater {
    float current;
    int fnext;
    
  public:
    bool smooth;
  
    float next(Rng *rng);
    Randomater();
  };
  
  vector<Randomater> rands;
  vector<float> next;
  
  Config config;
  
  Rng rng;

  void updateGameWork(const vector<Tank> &players, int me);
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos);
};

class FactionState {
public:
  bool taken;
  Float4 compass_location;
  const IDBFaction *faction;
};

enum { SETTING_BUTTONS, SETTING_AXISTYPE, SETTING_AXISCHOOSE, SETTING_TEST, SETTING_READY, SETTING_LAST };
const char * const setting_names[] = { "Keys", "Mode", "Directions", "Test", "Done" };

enum { CHOICE_FIRSTPASS, CHOICE_REAXIS, CHOICE_ACTIVE, CHOICE_IDLE };

enum ReadMode { RM_IDLE, RM_CHOOSING, RM_NOTRIGGER };
struct PlayerMenuState {
public:
  Float2 compasspos;
  FactionState *faction;
  int current_faction_over;
  int current_faction_over_duration;

  int settingmode;
  int choicemode;

  int setting_button_current;
  ReadMode setting_button_reading;
  vector<int> buttons;

  int setting_axis_current;
  ReadMode setting_axis_reading;
  vector<int> axes;
  vector<char> axes_invert;

  int setting_axistype;
  int setting_axistype_curchoice;
  int setting_old_axistype;

  int setting_axistype_demo_cursegment;
  int setting_axistype_demo_aiframe;
  smart_ptr<GamePackage> setting_axistype_demo;
  smart_ptr<GameAiAxisRotater> setting_axistype_demo_ai;
  void createNewAxistypeDemo(RngSeed seed);

  smart_ptr<GamePackage> test_game;
  
  Keystates genKeystate(const Controller &keys) const;

  PlayerMenuState();
  ~PlayerMenuState();
};

bool runSettingTick(const Controller &keys, PlayerMenuState *pms, vector<FactionState> &factions);
void runSettingRender(const PlayerMenuState &pms, const string &availdescr);  // kind of grim, second parameter is text description of what buttons are available

#endif
