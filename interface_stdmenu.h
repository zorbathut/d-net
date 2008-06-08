#ifndef DNET_INTERFACE_STDMENU
#define DNET_INTERFACE_STDMENU

#include "input.h"
#include "smartptr.h"

#include <boost/function.hpp>
#include <boost/optional.hpp>

using namespace std;

using boost::function;
using boost::optional;

enum StdMenuCommand { SMR_NOTHING = -1, SMR_ENTER = -2, SMR_RETURN = -3 };

class StdMenuItem : boost::noncopyable {
public:

  virtual pair<StdMenuCommand, int> tickEntire(const Keystates &keys);
  virtual void renderEntire(const Float4 &bounds, bool obscure) const;
  
  virtual pair<StdMenuCommand, int> tickItem(const Keystates *keys) = 0;
  virtual float renderItemHeight() const;
  virtual float renderItemWidth(float tmx) const = 0;
  virtual void renderItem(const Float4 &bounds) const = 0; // ey is ignored

  virtual void checksum(Adler32 *adl) const = 0;

  StdMenuItem();
  virtual ~StdMenuItem();
};

class StdMenu {
  
  vector<vector<smart_ptr<StdMenuItem> > > items;
  int vpos;
  int hpos;
  
  bool inside;
  
public:

  void pushMenuItem(const smart_ptr<StdMenuItem> &site);
  void pushMenuItemAdjacent(const smart_ptr<StdMenuItem> &site);

  pair<StdMenuCommand, int> tick(const Keystates &keys);
  void render(const Float4 &bounds, bool obscure) const;

  void reset();

  void checksum(Adler32 *adl) const;

  StdMenu();

};

class StdMenuItemTrigger : public StdMenuItem {
  StdMenuItemTrigger(const string &text, int trigger);
  
  string name;
  int trigger;
  
public:
  static smart_ptr<StdMenuItemTrigger> make(const string &text, int trigger);
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void checksum(Adler32 *adl) const;
};

class StdMenuItemScale : public StdMenuItem {
public:
  
  struct ScaleDisplayer {
    vector<string> labels;
    const Coord *start;
    const Coord *end;
    const bool *onstart;
    bool mini;
    
    void render(float pos) const;
    
    ScaleDisplayer(const vector<string> &labels, const Coord *start, const Coord *end, const bool *onstart, bool mini);
    ScaleDisplayer();
  };
  
private:
  
  StdMenuItemScale(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos);
  
  string name;

  Coord *position;
  function<Coord (const Coord &)> munge;
  ScaleDisplayer displayer;
  bool selected_val;
  bool *selected_pos;

public:
  
  static smart_ptr<StdMenuItemScale> make(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos);
    
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void checksum(Adler32 *adl) const;
};

/*************
 * Rounds
 */

class StdMenuItemRounds : public StdMenuItem {
  StdMenuItemRounds(const string &text, Coord *start, Coord *end, Coord *exp, int *rounds);

  string name;
  Coord *start;
  Coord *end;
  Coord *expv;
  
  int *rounds;
  
public:
  static smart_ptr<StdMenuItemRounds> make(const string &text, Coord *start, Coord *end, Coord *exp, int *rounds);
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void checksum(Adler32 *adl) const;
};

/*************
 * Chooser
 */

template<typename T> class StdMenuItemChooser : public StdMenuItem {  // I don't feel like bloating the compiletime a lot, so I've put explicit instantiations of this in interface_stdmenu.cpp
  void syncoptions();
  
  string name;
  float maxx;
  
  vector<pair<string, T> > options;
  T * storage;
  
  optional<function<void (T)> > changefunctor;
  
  int item;
  
public:
  StdMenuItemChooser(const string &text, const vector<pair<string, T> > &options, T *storage, optional<function<void (T)> > changefunctor);
  
  static smart_ptr<StdMenuItemChooser<T> > make(const string &text, const vector<pair<string, T> > &options, T *storage);
  static smart_ptr<StdMenuItemChooser<T> > make(const string &text, const vector<pair<string, T> > &options, T *storage, function<void (T)> changefunctor);
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void changeOptionDb(const vector<pair<string, T> > &newopts);
  
  void checksum(Adler32 *adl) const;
};

/*************
 * Counter
 */

class StdMenuItemCounter : public StdMenuItemChooser<int> {
  static vector<pair<string, int> > makeOpts(int low, int high);
  
public:
  StdMenuItemCounter(const string &text, int *storage, int low, int high);
  
  static smart_ptr<StdMenuItemCounter> make(const string &text, int *storage, int low, int high);
};

/*************
 * Submenu
 */

class StdMenuItemSubmenu : public StdMenuItem {
  StdMenuItemSubmenu(const string &text, StdMenu menu, int signal);
  StdMenuItemSubmenu(const string &text, StdMenu *menu, int signal);
  
  string name;
  
  StdMenu submenu;
  StdMenu *submenu_ptr;
  
  StdMenu &gsm();  
  const StdMenu &gsm() const;
  
  int signal;
  
public:
  static smart_ptr<StdMenuItemSubmenu> make(const string &text, StdMenu menu, int signal = SMR_NOTHING);
  static smart_ptr<StdMenuItemSubmenu> make(const string &text, StdMenu *menu, int signal = SMR_NOTHING);
  
  pair<StdMenuCommand, int> tickEntire(const Keystates &keys);
  void renderEntire(const Float4 &bounds, bool obscure) const;

  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void checksum(Adler32 *adl) const;
};

/*************
 * Back
 */

class StdMenuItemBack : public StdMenuItem {
  StdMenuItemBack(const string &text, int signal);
  
  string name;
  int signal;
  
public:
  static smart_ptr<StdMenuItemBack> make(const string &text, int signal = SMR_NOTHING);
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys);
  float renderItemWidth(float tmx) const;
  void renderItem(const Float4 &bounds) const;
  
  void checksum(Adler32 *adl) const;
};

#endif
