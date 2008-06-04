#ifndef DNET_INPUTSNAG
#define DNET_INPUTSNAG

#include "input.h"
#include "rng.h"
#include "dumper.h"

using namespace std;

class Ai;
class SDL_KeyboardEvent;

class InputSnag : boost::noncopyable {
public:
  // init/shutdown
  InputState init(Dumper *dumper, bool allow_standard, int ais);
  void shutdown();

  // the loop
  void set_ai_count(int ct);
  int get_ai_count() const;

  void key(const SDL_KeyboardEvent *key);
  void mouseclick();
  
  InputState next(Dumper *dumper);
  
  // informational
  vector<Ai *> ais() const;
  vector<bool> human_flags() const;
  bool users() const;
  int primary_id() const;
  pair<int, int> getType(int id) const;
  ControlConsts getcc(int cid) const;

private:
  InputSnag(); // no soup for you
  ~InputSnag();
  friend InputSnag &isnag();
};

// singleton
InputSnag &isnag();
  
class ControlShutdown {
public:
  ControlShutdown() { } // this makes gcc shut up about unused variables
  ~ControlShutdown() {
    isnag().shutdown();
  }
};

#endif
