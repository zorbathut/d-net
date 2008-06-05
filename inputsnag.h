#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "rng.h"
#include "dumper.h"
#include "smartptr.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

struct JS; // man just don't ask

class InputSnag : boost::noncopyable {
public:
  // init/shutdown
  InputState init(Dumper *dumper, bool allow_standard, int ais);

  // the loop
  void set_ai_count(int ct);
  int get_ai_count() const;

  void key(const SDL_KeyboardEvent *key);
  void mouseclick();
  
  InputState next(Dumper *dumper);
  
  // informational
  vector<Ai *> ais();
  vector<bool> human_flags() const;
  bool users() const;
  int primary_id() const;
  pair<int, int> getType(int id) const;
  ControlConsts getcc(int cid) const;

  int getbuttoncount(int cid) const;
  int getaxiscount(int cid) const;

private:
  vector<pair<int, int> > sources;
  vector<int> prerecorded;
  vector<JS> joysticks;
  vector<smart_ptr<Ai> > ai;
  int primaryid;

  InputState last;
  InputState now;

  InputSnag(); // no soup for you
  ~InputSnag();
  void shutdown();
  friend void MainLoop();
};

#endif
