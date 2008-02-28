
#include "interface.h"

#include "ai.h"
#include "args.h"
#include "debug.h"
#include "game_tank.h"
#include "gfx.h"
#include "inputsnag.h"
#include "metagame.h"
#include "os.h"
#include "player.h"
#include "audio.h"
#include "adler32.h"
#include "game_ai.h"
#include "res_interface.h"
#include "adler32_util.h"

#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/function.hpp>

using namespace std;
using boost::bind;
using boost::function;
using boost::optional;

DEFINE_int(rounds_per_shop, 6, "How many rounds between each buying-things opportunity");
DEFINE_bool(auto_newgame, false, "Automatically enter New Game");
DEFINE_float(startingPhase, -1, "Starting phase override");
DEFINE_bool(showtanks, false, "Show-tank mode");
DEFINE_bool(dumpTanks, false, "Dump-tank mode");
DEFINE_bool(showGlobalErrors, true, "Display global errors");

DEFINE_int(factionMode, 3, "Faction mode to skip faction choice battle, -1 for normal faction mode");

/*************
 * Std
 */
 
pair<StdMenuCommand, int> StdMenuItem::tickEntire(const Keystates &keys) {
  CHECK(0);
}
void StdMenuItem::renderEntire(const Float4 &bounds, bool obscure) const {
  CHECK(0);
}

float StdMenuItem::renderItemHeight() const {
  return 4;
}

StdMenuItem::StdMenuItem() { };
StdMenuItem::~StdMenuItem() { };

/*************
 * Trigger
 */

class StdMenuItemTrigger : public StdMenuItem {
  StdMenuItemTrigger(const string &text, int trigger) : name(text), trigger(trigger) { };
  
  string name;
  int trigger;
  
public:
  static smart_ptr<StdMenuItemTrigger> make(const string &text, int trigger) { return smart_ptr<StdMenuItemTrigger>(new StdMenuItemTrigger(text, trigger)); }
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    if(keys && keys->accept.push) {
      queueSound(S::accept);
      reg_adler_ul(__LINE__);
      return make_pair(SMR_NOTHING, trigger);
    }
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  
  float renderItemWidth(float tmx) const {
    return getTextWidth(name, 4);
  }
  
  void renderItem(const Float4 &bounds) const {
    drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "trigger");
    adler(adl, name);
    adler(adl, trigger);
  }
};

/*************
 * Scale
 */

class StdMenuItemScale : public StdMenuItem {
public:
  
  struct ScaleDisplayer {
    vector<string> labels;
    const Coord *start;
    const Coord *end;
    const bool *onstart;
    bool mini;
    
    void render(float pos) const;
    
    ScaleDisplayer(const vector<string> &labels, const Coord *start, const Coord *end, const bool *onstart, bool mini) : labels(labels), start(start), end(end), onstart(onstart), mini(mini) { };
    ScaleDisplayer() { };
  };
  
private:
  
  StdMenuItemScale(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos) : name(text), position(position), munge(munge), displayer(sds), selected_val(selected_val), selected_pos(selected_pos) { };
  
  string name;

  Coord *position;
  function<Coord (const Coord &)> munge;
  ScaleDisplayer displayer;
  bool selected_val;
  bool *selected_pos;

public:
  
  static smart_ptr<StdMenuItemScale> make(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos) { return smart_ptr<StdMenuItemScale>(new StdMenuItemScale(text, position, munge, sds, selected_val, selected_pos)); }
    
  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    if(keys) {
      if(keys->l.down)
        *position -= Coord(1) / 16;
      if(keys->r.down)
        *position += Coord(1) / 16;
      if(keys->l.down || keys->r.down)
        *position = munge(*position);
    }
    
    if(keys && selected_pos)
      *selected_pos = selected_val;
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  
  float renderItemWidth(float tmx) const {
    return getTextWidth(name, 4);
  }
  
  void renderItem(const Float4 &bounds) const {
    drawText(name.c_str(), 4, bounds.s());
    
    Float4 boundy = Float4(bounds.sx + 35, bounds.sy, bounds.ex, bounds.sy + 4);
    GfxWindow gfxw(boundy, 1.0);
    
    displayer.render(position->toFloat());
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "scale");
    adler(adl, name);
    CHECK(position);
    adler(adl, *position);
    adler(adl, selected_val);
    CHECK(selected_pos);
    adler(adl, *selected_pos);
  }
};

void StdMenuItemScale::ScaleDisplayer::render(float pos) const {
  float cent;
  if(!onstart) {
    cent = 0.5;
  } else if(*onstart) {
    cent = start->toFloat();
  } else {
    cent = end->toFloat();
  }
  
  if(mini) {
    setZoomAround(Float4(-0.5, -0.1, labels.size() + 0.5 - 1, 0.06));
  } else {
    setZoomAround(Float4(cent, -0.09, cent, 0.08));
  }
  
  const float height = mini ? 0.27 : 0.19;
  
  for(int i = 0; i < labels.size(); i++) {
    setColor(C::active_text);
    drawJustifiedText(labels[i], height / 2 * 0.9, Float2(i, 0), TEXT_CENTER, TEXT_MAX);
    setColor(C::inactive_text);
    drawLine(Float4(i, height / 2 - height / 6, i, height / 6), height / 20);
  }
  
  setColor(C::inactive_text);
  drawLine(Float4(0, height / 4, labels.size() - 1, height / 4), height / 20);
  
  setColor(C::active_text);
  if(start && end)
    drawLine(Float4(start->toFloat(), height / 4, end->toFloat(), height / 4), height / 10);
  
  if(start) {
    vector<Float2> path;
    path.push_back(Float2(start->toFloat(), height / 8));
    path.push_back(Float2(start->toFloat() + height / 16, height / 4));
    path.push_back(Float2(start->toFloat(), height / 2 - height / 8));
    path.push_back(Float2(start->toFloat() - height / 16, height / 4));
    drawLineLoop(path, height / 20);
  }
  
  if(end) {
    vector<Float2> path;
    path.push_back(Float2(end->toFloat(), height / 8));
    path.push_back(Float2(end->toFloat() + height / 16, height / 4));
    path.push_back(Float2(end->toFloat(), height / 2 - height / 8));
    path.push_back(Float2(end->toFloat() - height / 16, height / 4));
    drawLineLoop(path, height / 20);
  }
  
  if(!start && !end) {
    vector<Float2> path;
    path.push_back(Float2(pos, height / 8));
    path.push_back(Float2(pos + height / 16, height / 4));
    path.push_back(Float2(pos, height / 2 - height / 8));
    path.push_back(Float2(pos - height / 16, height / 4));
    drawLineLoop(path, height / 20);
  }
}

/*************
 * Rounds
 */

int calculateRounds(Coord start, Coord end, Coord exp) {
  return floor(ceil((end - start) * log(30) / exp / 6)).toInt() * 6;
}

class StdMenuItemRounds : public StdMenuItem {
  StdMenuItemRounds(const string &text, Coord *start, Coord *end, Coord *exp) : name(text), start(start), end(end), expv(exp) { };

  string name;
  Coord *start;
  Coord *end;
  Coord *expv;
  
public:
  static smart_ptr<StdMenuItemRounds> make(const string &text, Coord *start, Coord *end, Coord *exp) { return smart_ptr<StdMenuItemRounds>(new StdMenuItemRounds(text, start, end, exp)); }
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    if(keys && keys->l.down)
      *expv *= Coord(101) / 100;
    if(keys && keys->r.down)
      *expv /= Coord(101) / 100;
    *expv = clamp(*expv, Coord(0.001), 2);
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  float renderItemWidth(float tmx) const {
    return tmx;
  }
  void renderItem(const Float4 &bounds) const {
    drawText(name.c_str(), 4, bounds.s());
    float percentage = (exp(expv->toFloat()) - 1) * 100;
    drawJustifiedText(StringPrintf("%d (+%.2f%% cash/round)", calculateRounds(*start, *end, *expv), percentage), 4, Float2(bounds.ex, bounds.sy), TEXT_MAX, TEXT_MIN);
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "rounds");
    adler(adl, name);
    adler(adl, *start);
    adler(adl, *end);
    adler(adl, *expv);
  }
};

/*************
 * Chooser
 */

template<typename T> class StdMenuItemChooser : public StdMenuItem {
  StdMenuItemChooser(const string &text, const vector<pair<string, T> > &options, T *storage, optional<function<void (T)> > changefunctor) : name(text), options(options), storage(storage), changefunctor(changefunctor) {
    CHECK(storage);
    syncoptions();
  }
  
  void syncoptions() {
    maxx = 0;
    item = -1;
    
    for(int i = 0; i < options.size(); i++) {
      if(*storage == options[i].second) {
        CHECK(item == -1);
        item = i;
      }
      maxx = max(maxx, getTextWidth(options[i].first, 4));
    }
    
    if(item == -1) {
      item = 0;
      *storage = options[item].second;
    }
    
    maxx += getTextWidth(name, 4);
    maxx += 8;
  }
  
  string name;
  float maxx;
  
  vector<pair<string, T> > options;
  T * storage;
  
  optional<function<void (T)> > changefunctor;
  
  int item;
  
public:
  static smart_ptr<StdMenuItemChooser<T> > make(const string &text, const vector<pair<string, T> > &options, T *storage) { return smart_ptr<StdMenuItemChooser<T> >(new StdMenuItemChooser<T>(text, options, storage, optional<function<void (T)> >())); }
  static smart_ptr<StdMenuItemChooser<T> > make(const string &text, const vector<pair<string, T> > &options, T *storage, function<void (T)> changefunctor) { return smart_ptr<StdMenuItemChooser<T> >(new StdMenuItemChooser<T>(text, options, storage, changefunctor)); }
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    if(keys && keys->l.push)
      item--;
    if(keys && keys->r.push)
      item++;
    
    item = modurot(item, options.size());
    
    *storage = options[item].second;
    
    if(changefunctor)
      (*changefunctor)(*storage);
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  float renderItemWidth(float tmx) const {
    return maxx;
  }
  void renderItem(const Float4 &bounds) const {
    drawText(name, 4, Float2(bounds.sx, bounds.sy));
    drawJustifiedText(options[item].first, 4, Float2(bounds.ex, bounds.sy), TEXT_MAX, TEXT_MIN);
  }
  
  void changeOptionDb(const vector<pair<string, T> > &newopts) {
    options = newopts;
    
    syncoptions();
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "chooser");
    adler(adl, name);
    for(int i = 0; i < options.size(); i++) {
      adler(adl, options[i].first);
    }
    adler(adl, item);
  }
};

/*************
 * Submenu
 */

class StdMenuItemSubmenu : public StdMenuItem {
  StdMenuItemSubmenu(const string &text, StdMenu menu, int signal) : name(text), submenu(menu), submenu_ptr(NULL), signal(signal) { };
  StdMenuItemSubmenu(const string &text, StdMenu *menu, int signal) : name(text), submenu_ptr(menu), signal(signal) { };
  
  string name;
  
  StdMenu submenu;
  StdMenu *submenu_ptr;
  
  StdMenu &gsm() {
    if(submenu_ptr) return *submenu_ptr;
    return submenu;
  }
  
  const StdMenu &gsm() const {
    if(submenu_ptr) return *submenu_ptr;
    return submenu;
  }
  
  int signal;
  
public:
  static smart_ptr<StdMenuItemSubmenu> make(const string &text, StdMenu menu, int signal = SMR_NOTHING) { return smart_ptr<StdMenuItemSubmenu>(new StdMenuItemSubmenu(text, menu, signal)); }
  static smart_ptr<StdMenuItemSubmenu> make(const string &text, StdMenu *menu, int signal = SMR_NOTHING) { return smart_ptr<StdMenuItemSubmenu>(new StdMenuItemSubmenu(text, menu, signal)); }
  
  pair<StdMenuCommand, int> tickEntire(const Keystates &keys) {
    return gsm().tick(keys);
  }
  void renderEntire(const Float4 &bounds, bool obscure) const {
    gsm().render(bounds, obscure);
  }

  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    reg_adler_ul((bool)keys);
    reg_adler_ul(keys->accept.push);
    if(keys && keys->accept.push) {
      reg_adler_ul(__LINE__);
      queueSound(S::accept);
      gsm().reset();
      reg_adler_ul(__LINE__);
      return make_pair(SMR_ENTER, signal);
    }
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  float renderItemWidth(float tmx) const {
    return getTextWidth(name, 4);
  }
  void renderItem(const Float4 &bounds) const {
    drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "submenu");
    adler(adl, name);
    submenu.checksum(adl);
    if(submenu_ptr)
      submenu_ptr->checksum(adl);
  }
};

/*************
 * Back
 */

class StdMenuItemBack : public StdMenuItem {
  StdMenuItemBack(const string &text, int signal) : name(text), signal(signal) { };
  
  string name;
  int signal;
  
public:
  static smart_ptr<StdMenuItemBack> make(const string &text, int signal = SMR_NOTHING) { return smart_ptr<StdMenuItemBack>(new StdMenuItemBack(text, signal)); }
  
  pair<StdMenuCommand, int> tickItem(const Keystates *keys) {
    reg_adler_ul(__LINE__);
    if(keys && keys->accept.push) {
      queueSound(S::choose);
      return make_pair(SMR_RETURN, signal);
    }
    
    reg_adler_ul(__LINE__);
    return make_pair(SMR_NOTHING, SMR_NOTHING);
  }
  float renderItemWidth(float tmx) const {
    return getTextWidth(name, 4);
  }
  void renderItem(const Float4 &bounds) const {
    drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
  }
  
  void checksum(Adler32 *adl) const {
    adler(adl, "back");
    adler(adl, name);
    adler(adl, signal);
  }
};

/*************
 * StdMenu
 */
 
void StdMenu::pushMenuItem(const smart_ptr<StdMenuItem> &site) {
  StackString stp("pushmenuitem");
  items.push_back(vector<smart_ptr<StdMenuItem> >(1, site));
}

void StdMenu::pushMenuItemAdjacent(const smart_ptr<StdMenuItem> &site) {
  CHECK(items.size());
  items.back().push_back(site);
}

pair<StdMenuCommand, int> StdMenu::tick(const Keystates &keys) {
  StackString stp("StdMenu ticking");
  reg_adler_ul(keys.accept.push);
  reg_adler_ul(__LINE__);
  if(!inside) {
    int pvpos = vpos;
    int phpos = hpos;
    
    if(keys.u.repeat)
      vpos--;
    if(keys.d.repeat)
      vpos++;
    vpos = modurot(vpos, items.size());
    
    if(keys.r.repeat)
      hpos++;
    if(keys.l.repeat)
      hpos--;
    hpos = modurot(hpos, items[vpos].size());
    
    if(pvpos != vpos || phpos != hpos)
      queueSound(S::select);
  }
  reg_adler_ul(__LINE__);
  for(int i = 0; i < items.size(); i++)
    for(int j = 0; j < items[i].size(); j++)
      if(i != vpos && j != hpos)
        items[i][j]->tickItem(NULL);
  reg_adler_ul(__LINE__);
  {
    pair<StdMenuCommand, int> rv;
    if(inside) {
      reg_adler_ul(__LINE__);
      rv = items[vpos][hpos]->tickEntire(keys);
      reg_adler_ul(__LINE__);
    } else {
      reg_adler_ul(__LINE__);
      rv = items[vpos][hpos]->tickItem(&keys);
      reg_adler_ul(__LINE__);
    }
    
    reg_adler_ul(__LINE__);
    if(rv.first == SMR_NOTHING) {
      reg_adler_ul(__LINE__);
      return rv;
    } else if(rv.first == SMR_ENTER) {
      inside = true;
      reg_adler_ul(0x12341237);
      reg_adler_ul(inside);
      reg_adler_ul(__LINE__);
      return make_pair(SMR_NOTHING, rv.second);
    } else if(rv.first == SMR_RETURN && !inside) {
      reg_adler_ul(__LINE__);
      return make_pair(SMR_RETURN, rv.second);
    } else if(rv.first == SMR_RETURN && inside) {
      inside = false;
      reg_adler_ul(0x12341236);
      reg_adler_ul(inside);
      reg_adler_ul(__LINE__);
      return make_pair(SMR_NOTHING, rv.second);
    } else {
      CHECK(0);
    }
  }
  reg_adler_ul(__LINE__);
}

void StdMenu::render(const Float4 &bounds, bool obscure) const {
  StackString stp("StdMenu rendering");
  
  if(inside) {
    items[vpos][hpos]->renderEntire(bounds, obscure);
  } else {
    GfxWindow gfxw(bounds, 1.0);
    setZoomCenter(0, 0, getZoom().span_y() / 2);
    
    const float tween_items = 3;
    const float border = 4;
    
    float totheight = 0;
    float maxwidth = 0;
    for(int i = 0; i < items.size(); i++) {
      float theight = 0;
      float twidth = 0;
      for(int j = 0; j < items[i].size(); j++) {
        theight = max(theight, items[i][j]->renderItemHeight());
        twidth += items[i][j]->renderItemWidth((getZoom().span_x() - 4) / items[i].size());
      }
      
      totheight += theight;
      maxwidth = max(maxwidth, twidth);
    }
    
    totheight += tween_items * (items.size() - 1);
    
    if(obscure) {
      Float2 upleft = Float2(maxwidth / 2 + border, totheight / 2 + border);
      setColor(C::box_border);
      drawSolid(Float4(-upleft, upleft));
      drawRect(Float4(-upleft, upleft), 0.5);
    }
    
    float curpos = (getZoom().span_y() - totheight) / 2;
    for(int i = 0; i < items.size(); i++) {
      float theight = 0;
      float tx = 0;
      
      for(int j = 0; j < items[i].size(); j++) {
        if(i == vpos && j == hpos) {
          setColor(C::active_text);
        } else {
          setColor(C::inactive_text);
        }
        
        items[i][j]->renderItem(Float4(-maxwidth / 2 + tx, getZoom().sy + curpos, -maxwidth / 2 + tx + maxwidth / items[i].size(), -1));
        
        theight = max(theight, items[i][j]->renderItemHeight());
        tx += maxwidth / items[i].size();
      }
      curpos += theight + tween_items;
    }
  }
}

void StdMenu::reset() {
  vpos = 0;
  hpos = 0;
  inside = false;
  //reg_adler_ul(0x12341235);
  reg_adler_ul(inside);
}

StdMenu::StdMenu() {
  vpos = 0;
  hpos = 0;
  inside = false;
  //reg_adler_ul(0x12341234);
  reg_adler_ul(inside);
}

void StdMenu::checksum(Adler32 *adl) const {vector<vector<smart_ptr<StdMenuItem> > > items;
  StackString sstr("WE CHECKSUM MENUS");
  reg_adler_ul(0x47283183);
  reg_adler_intermed(*adl);
  for(int i = 0; i < items.size(); i++) {
    for(int j = 0; j < items[i].size(); j++) {
      reg_adler_intermed(*adl);
      items[i][j]->checksum(adl);
      reg_adler_intermed(*adl);
    }
  }
  reg_adler_intermed(*adl);
  adler(adl, vpos);
  reg_adler_intermed(*adl);
  adler(adl, hpos);
  reg_adler_intermed(*adl);
  reg_adler_ul(inside);
  adler(adl, inside);
  reg_adler_intermed(*adl);
  reg_adler_ul(0x47283184);
}

/*************
 * GameAiIntro
 */

class GameAiIntro : public GameAi {
private:
  Coord2 nextpos;
  Coord lastdist;
  int waitcycles;

  void updateGameWork(const vector<Tank> &players, int me) {
    CHECK(0);
  }
  void updateBombardmentWork(const vector<Tank> &players, Coord2 mypos) {
    if(waitcycles > 0) {
      waitcycles--;
      return;
    }
    
    if(nextpos == Coord2(-1000, -1000))
      genNewPos(mypos);
    
    nextKeys.udlrax = normalize(nextpos - mypos);
    nextKeys.udlrax.y *= -1;
    
    Coord tdist = len(nextpos - mypos);
    if(tdist > lastdist) {
      nextKeys.fire[0].down = true;
      genNewPos(mypos);
    } else {
      lastdist = tdist;
    }
  }
  
  void genNewPos(Coord2 cpos) {
    int divert = 50;
    float mag;
    do {
      mag = unsync().frand();
      nextpos = cpos + makeAngle(unsync().frand() * COORDPI * 2) * mag * divert;
      divert += 10;
    } while(!isInside(Coord4(-364, -116, 364, 0), nextpos) && !isInside(Coord4(-112, 0, 112, 92), nextpos));
    lastdist = 1000;
  }
  
public:
  GameAiIntro() {
    nextpos = Coord2(-1000, -1000);
    waitcycles = unsync().choose(FPS * 4);
  }
  
};

/*************
 * Interface
 */

bool InterfaceMain::tick(const InputState &is, RngSeed gameseed) {
  reg_adler_ul(__LINE__);
  if(is.escape.push) {
    inescmenu = !inescmenu;
    if(inescmenu)
      escmenu.reset();
  }
  reg_adler_ul(__LINE__);
  if(FLAGS_showtanks)
    return false;
  
  StackString stp("Interface ticking");
  
  inptest_controls = is.controllers;
  reg_adler_ul(__LINE__);
  if(kst.size() == 0) {
    CHECK(is.controllers.size() != 0);
    kst.resize(is.controllers.size());
  }
  
  reg_adler_ul(__LINE__);
  
  CHECK(kst.size() == is.controllers.size());
  
  for(int i = 0; i < is.controllers.size(); i++) {
    CHECK(is.controllers[i].keys.size() >= 1);
    kst[i].u.newState(is.controllers[i].menu.y > .5);
    kst[i].d.newState(is.controllers[i].menu.y < -.5);
    kst[i].r.newState(is.controllers[i].menu.x > .5);
    kst[i].l.newState(is.controllers[i].menu.x < -.5);
    bool aButtonPushed = false;
    for(int j = 0; j < is.controllers[i].keys.size(); j++)
      if(is.controllers[i].keys[j].push)
        aButtonPushed = true;
    kst[i].accept.newState(aButtonPushed);
    kst[i].cancel.newState(false);
    for(int j = 0; j < SIMUL_WEAPONS; j++)
      kst[i].fire[j].newState(false);
  }
  reg_adler_ul(__LINE__);
  if(inescmenu) {
    reg_adler_ul(__LINE__);
    pair<StdMenuCommand, int> rv = escmenu.tick(kst[controls_primary_id()]);
    if(rv.first == SMR_RETURN)
      inescmenu = false;
    if(rv.second == SMR_NOTHING) {
    } else if(rv.second == ESCMENU_MAINMENU) {
      dprintf("re-initting\n");
      inescmenu = false;
      interface_mode = STATE_MAINMENU;
      init();
    } else if(rv.second == ESCMENU_QUIT) {
      return true;
    } else if(rv.second == OPTS_SETRES) {
      setResolution(opts_res, opts_aspect, opts_fullscreen);
    } else {
      CHECK(0);
    }
    reg_adler_ul(__LINE__);
  } else if(interface_mode == STATE_MAINMENU) {
    reg_adler_ul(__LINE__);
    if(!inptest) {
      introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
      introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
    }
    reg_adler_ul(controls_primary_id());
    reg_adler_ul(__LINE__);
    pair<StdMenuCommand, int> mrv = mainmenu.tick(kst[controls_primary_id()]);
    reg_adler_ul(__LINE__);
    
    if(mrv.second == MAIN_NEWGAME || FLAGS_auto_newgame) {
      if(!faction_toggle)
        faction = 1;
      else
        faction = 4;
      game = new Metagame(kst.size(), Money((long long)(1000 * pow(30, start.toFloat()))), exp(moneyexp), faction - 1, FLAGS_rounds_per_shop, calculateRounds(start, end, moneyexp), gameseed);
      dprintf("ENTERING PLAYING\n");
      interface_mode = STATE_PLAYING;
    } else if(mrv.second == MAIN_EXIT) {
      return true;
    } else if(mrv.second == MAIN_INPUTTEST) {
      inptest = !inptest;
    } else if(mrv.second == MAIN_GRID) {
      grid = !grid;
    } else if(mrv.second == MAIN_NEWGAMEMENU) {
      inptest = false;
      grid = false; // :D
    } else if(mrv.second == OPTS_SETRES) {
      setResolution(opts_res, opts_aspect, opts_fullscreen);
    } else {
      CHECK(mrv.second == -1);
    }
    reg_adler_ul(__LINE__);
  } else if(interface_mode == STATE_PLAYING) {
    reg_adler_ul(__LINE__);
    if(game->runTick(is.controllers)) {
      init();   // full reset
    }
  } else {
    CHECK(0);
  }
  reg_adler_ul(__LINE__);
  return false;
}

void InterfaceMain::ai(const vector<Ai *> &ai) const {
  StackString stp("Interface AI");
  if(interface_mode == STATE_MAINMENU) {
    for(int i = 0; i < ai.size(); i++)
      if(ai[i])
        ai[i]->updatePregame();
  } else if(interface_mode == STATE_PLAYING) {
    game->ai(ai);
  } else {
    CHECK(0);
  }
}

bool tankCostSorter(const pair<string, IDBTank> &lhs, const pair<string, IDBTank> &rhs) {
  if(lhs.second.base_cost != rhs.second.base_cost)
    return lhs.second.base_cost < rhs.second.base_cost;
  return lhs.second.engine > rhs.second.engine;
}

Coord InterfaceMain::start_clamp(const Coord &opt) const {
  return clamp(opt, 0, end - Coord(1) / 16);
}
Coord InterfaceMain::end_clamp(const Coord &opt) const {
  return clamp(opt, start + Coord(1) / 16, 7);
}

int gcd(int x, int y) {
  while(y) {
    int t = y;
    y = x % y;
    x = t;
  }
  return x;
}

bool is_relatively_prime(int i, int j) {
  return gcd(i, j) == 1;
}

class rati {
public:
  bool operator()(pair<int, int> lhs, pair<int, int> rhs) {
    lhs.first *= rhs.second;
    rhs.first *= lhs.second;
    return lhs.first > rhs.first;
  }
};

vector<pair<string, float> > gen_aspects(int x, int y) {
  vector<pair<int, int> > asps;
  asps.push_back(make_pair(4, 3));
  asps.push_back(make_pair(16, 10));
  asps.push_back(make_pair(16, 9));
  
  asps.push_back(make_pair(x / gcd(x, y), y / gcd(x, y)));
  
  if(count(asps.begin(), asps.end(), make_pair(8, 5)))
    asps.erase(find(asps.begin(), asps.end(), make_pair(8, 5)));
  
  sort(asps.begin(), asps.end(), rati());
  asps.erase(unique(asps.begin(), asps.end()), asps.end());
  
  vector<pair<string, float> > rv;
  for(int i = 0; i < asps.size(); i++)
    rv.push_back(make_pair(StringPrintf("%d:%d", asps[i].first, asps[i].second), (float)asps[i].first / asps[i].second));
  
  return rv;
}

void InterfaceMain::opts_res_changed(pair<int, int> newres) {
  if(newres.second * 5 == newres.first * 4) // this isn't actually 5:4, this is probably 4:3
    newres = make_pair(640, 480);
  
  opts_aspect = (float)newres.first / newres.second;
  
  opts_aspect_chooser->changeOptionDb(gen_aspects(newres.first, newres.second));
}

void InterfaceMain::render() const {
  if(isUnoptimized()) {
    setZoomVertical(0, 0, 100);
    setColor(Color(1.0, 0.3, 0.3));
    drawText("Optimizations disabled!", 6, Float2(1, 1));
  }
  
  if(FLAGS_dumpTanks) {  // this should probably be in main.cpp really
    static bool dumpedtanks = false;
    
    if(!dumpedtanks) {
      vector<pair<string, IDBTank> > ttk;
      for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++)
        ttk.push_back(make_pair(itr->first, itr->second));
      sort(ttk.begin(), ttk.end(), tankCostSorter);
      
      dumpedtanks = true;
      
      int x = 0;
      int y = 0;
      
      bool addedfinal = false;
      FILE *of = fopen("tanks.dv2", "w");
      for(int i = 0; i < ttk.size(); i++) {
        fprintf(of, "path {\n  center=%d,%d\n  reflect=spin\n  dupes=1\n  angle=0/1\n", x, y);
        if(i % 3 == 0) {
          x = -1000;
          y += 1000;
          if(i % 6 == 0)
            y += 1000;
        } else {
          x += 1000;
        }
        for(int j = 0; j < ttk[i].second.vertices.size(); j++)
          fprintf(of, "  node= --- | %f,%f | ---\n", (ttk[i].second.vertices[j].y + ttk[i].second.centering_adjustment.y).toFloat() * 100, -(ttk[i].second.vertices[j].x + ttk[i].second.centering_adjustment.x).toFloat() * 100);
        fprintf(of, "}\n\n");
        if(i + 1 == ttk.size() && !addedfinal) {
          i -= 6; // hee hee
          addedfinal = true;
        }
      }
      fclose(of);
    }
  }
  
  if(FLAGS_showtanks) {
    setZoomVertical(0, 0, 200);
    setColor(1.0, 1.0, 1.0);
    float x = 0;
    float y = 0;
    const float xsize = 30;
    const float ysize = 30;
    vector<pair<string, IDBTank> > ttk;
    for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++)
      ttk.push_back(make_pair(itr->first, itr->second));
    sort(ttk.begin(), ttk.end(), tankCostSorter);
    for(int i = 0; i < ttk.size(); i++) {
      vector<Float2> vertices;
      for(int j = 0; j < ttk[i].second.vertices.size(); j++)
        vertices.push_back(Float2(ttk[i].second.vertices[j].y.toFloat(), -ttk[i].second.vertices[j].x.toFloat()) + Float2(xsize, ysize) / 2 + Float2(x, y));
      drawLineLoop(vertices, 0.5);
      drawJustifiedText(strrchr(ttk[i].first.c_str(), '.') + 1, 3, Float2(xsize / 2, ysize - 1) + Float2(x, y), TEXT_CENTER, TEXT_MAX);
      x += xsize;
      if(x + xsize > getZoom().ex) {
        y += ysize;
        x = 0;
      }
    }
    return;
  }
  
  StackString stp("Interface rendering");
  
  {
    smart_ptr<GfxWindow> wnd;
    if(inescmenu)
      wnd.reset(new GfxWindow(getZoom(), 0.5));
  
    if(FLAGS_showGlobalErrors) {
      for(int i = 0; i < returnErrorMessages().size(); i++) {
        setZoomCenter(0, 0, 100);
        setColor(1.0, 1.0, 1.0);
        drawJustifiedText(returnErrorMessages()[i], 30, Float2(0, 0), TEXT_CENTER, TEXT_CENTER);
      }
    }
    
    if(interface_mode == STATE_MAINMENU) {
      setZoomVertical(0, 0, 100);
      mainmenu.render(Float4(5, 50, getZoom().ex - 5, 95), false);
      setColor(C::inactive_text * 0.5);
      drawJustifiedText("Use the arrow keys to choose menu items. Hit Enter to select.", 3, Float2(getZoom().midpoint().x, getZoom().ey - 3), TEXT_CENTER, TEXT_MAX);
      if(grid) {
        setColor(1.0, 1.0, 1.0);
        drawGrid(1, 0.01);
      }
      if(inptest) {
        setZoomVertical(0, 0, 600);
        setColor(1.0, 1.0, 1.0);
        const float xsiz = 100;
        const float bord = xsiz / 25;
        const float usablesiz = xsiz - bord * 4;
        const float crosshair = usablesiz / 2;
        const float textheight = 9;
        const float ysiz = crosshair + bord * 3 + textheight;
        const float crosshairc = crosshair / 2;
        const float textsize = crosshair / 4;
        const float textrsize = textsize * 0.8;
        const int wid = int((getZoom().ey - 100) / xsiz);
        const int textymax = int(crosshair / textsize);
        const int textxmax = 2;
        const float textxofs = crosshair / textxmax;
        
        const float boxthick = 1.0;
        for(int i = 0; i < inptest_controls.size(); i++) {
          const Controller &ct = inptest_controls[i];
          float x = (i % wid) * xsiz + 50;
          float y = (i / wid) * ysiz + 50;
          if(i % wid == 0 && i) {
            setColor(C::box_border);
            drawLine(Float4(50, y, 750, y), boxthick);
          }
          setColor(C::gray(1.0));
          drawJustifiedText(controls_getcc(i).description, textheight, Float2(x + xsiz / 2, y + bord), TEXT_CENTER, TEXT_MIN);
          Float4 chbox(x + bord, y + bord * 2 + textheight, x + bord + crosshair, y + bord * 2 + crosshair + textheight);
          setColor(C::box_border);
          drawLine(Float4(chbox.sx, chbox.sy, chbox.sx + crosshair / 4, chbox.sy), boxthick);
          drawLine(Float4(chbox.sx, chbox.sy, chbox.sx, chbox.sy + crosshair / 4), boxthick);
          drawLine(Float4(chbox.sx, chbox.ey, chbox.sx + crosshair / 4, chbox.ey), boxthick);
          drawLine(Float4(chbox.sx, chbox.ey, chbox.sx, chbox.ey - crosshair / 4), boxthick);
          drawLine(Float4(chbox.ex, chbox.sy, chbox.ex - crosshair / 4, chbox.sy), boxthick);
          drawLine(Float4(chbox.ex, chbox.sy, chbox.ex, chbox.sy + crosshair / 4), boxthick);
          drawLine(Float4(chbox.ex, chbox.ey, chbox.ex - crosshair / 4, chbox.ey), boxthick);
          drawLine(Float4(chbox.ex, chbox.ey, chbox.ex, chbox.ey - crosshair / 4), boxthick);
          setColor(C::inactive_text);
          drawCrosshair(Coord2(ct.menu.x * crosshairc + bord + crosshairc + x, -ct.menu.y * crosshairc + bord * 2 + crosshairc + y + textheight), crosshair / 4, boxthick);
          setColor(C::active_text);
          float textx = x + bord * 3 + crosshair;
          float texty = y + bord * 2 + textheight;
          int ctxt = 0;
          int kd = 0;
          for(int j = 0; j < ct.keys.size(); j++)
            if(ct.keys[j].down)
              kd++;
          for(int j = 0; j < ct.keys.size(); j++) {
            if(ct.keys[j].down) {
              string tbd;
              if(ctxt >= textymax * textxmax) {
                continue;
              } else if(ctxt == textymax * textxmax - 1 && kd > textymax * textxmax) {
                tbd = "...";
              } else {
                tbd = StringPrintf("%d", j);
              }
              if(tbd != "") {
                drawText(tbd, textrsize, Float2(textx + ctxt / textymax * textxofs, texty + (ctxt % textymax) * textsize));
                ctxt++;
              }
            }
          }
        }
      } else {
        GfxWindow gfxw(Float4(0, 0, getZoom().ex, 60), 2.0);
        introscreen->renderToScreen();
      }
    } else if(interface_mode == STATE_PLAYING) {
      game->renderToScreen();
    } else {
      CHECK(0);
    }
  }
  
  if(inescmenu) {
    setZoomVertical(0, 0, 100);
    escmenu.render(getZoom(), true);
  }
  
  // testing line width
  /*
  {
    setZoomVertical(0, 0, 100);
    setColor(C::gray(0.5));
    for(int i = 0; i < 10; i++) {
      const float mult = 2;
      drawLine(Float4(10 + i * mult, 10, 10 + i * mult, 20), mult);
      drawLine(Float4(100, 10 + i * mult, 110, 10 + i * mult), mult);
    }
  }*/
};

void InterfaceMain::checksum(Adler32 *adl) const {
  StackString sstr("WE CHECKSUM SHIT");
  reg_adler_intermed(*adl);
  mainmenu.checksum(adl);
  reg_adler_intermed(*adl);
  escmenu.checksum(adl);
  reg_adler_intermed(*adl);
  optionsmenu.checksum(adl);
  reg_adler_intermed(*adl);
  reg_adler_ul(interface_mode);
  adler(adl, interface_mode);
  reg_adler_intermed(*adl);
  adler(adl, start);
  reg_adler_intermed(*adl);
  adler(adl, end);
  reg_adler_intermed(*adl);
  adler(adl, moneyexp);
  reg_adler_intermed(*adl);
  adler(adl, faction);
  reg_adler_intermed(*adl);
  
  if(game)
    game->checksum(adl);
}

void InterfaceMain::forceResize(int w, int h) {
  w = max(w, 100);
  h = min(h, w / 4 * 3);
  h = max(h, w / 6);  // no, you can't have 6:1 aspect. Not allowed.
  float aspect_per_pixel = opts_aspect / opts_res.first * opts_res.second;
  opts_res.first = w;
  opts_res.second = h;
  opts_aspect = aspect_per_pixel * w / h;
  CHECK(setResolution(make_pair(w, h), opts_aspect, opts_fullscreen));
}

void InterfaceMain::init() {
  StackString stp("IMain initting");
  
  mainmenu = StdMenu();
  escmenu = StdMenu();
  optionsmenu = StdMenu();
  kst.clear();
  introscreen_ais.clear();
  delete introscreen;
  introscreen = new GamePackage;
  
  interface_mode = STATE_MAINMENU;
  {
    vector<string> names = boost::assign::list_of("Junkyard")("Civilian")("Professional")("Military")("Exotic")("Experimental")("Ultimate")("Armageddon");
    
    StdMenu configmenu;
    configmenu.pushMenuItem(StdMenuItemScale::make("Game start", &start, bind(&InterfaceMain::start_clamp, this, _1), StdMenuItemScale::ScaleDisplayer(names, &start, &end, &onstart, true), true, &onstart));
    configmenu.pushMenuItem(StdMenuItemScale::make("Game end", &end, bind(&InterfaceMain::end_clamp, this, _1), StdMenuItemScale::ScaleDisplayer(names, &start, &end, &onstart, false), false, &onstart));
    configmenu.pushMenuItem(StdMenuItemRounds::make("Estimated rounds", &start, &end, &moneyexp));
    
    {
      vector<pair<string, bool> > onoff;
      onoff.push_back(make_pair("On", true));
      onoff.push_back(make_pair("Off", false));
      configmenu.pushMenuItem(StdMenuItemChooser<bool>::make("Factions", onoff, &faction_toggle));
    }
    
    //configmenu.pushMenuItem(StdMenuItem::makeOptions("Faction mode", boost::assign::list_of("Battle")("No factions")("Minor factions")("Normal factions")("Major factions"), &faction));
    configmenu.pushMenuItem(StdMenuItemTrigger::make("Begin", MAIN_NEWGAME));
    configmenu.pushMenuItemAdjacent(StdMenuItemBack::make("Cancel"));
    
    mainmenu.pushMenuItem(StdMenuItemSubmenu::make("New game", configmenu, MAIN_NEWGAMEMENU));
  }
  
  opts_res = getCurrentResolution();
  opts_aspect = getCurrentAspect();
  opts_fullscreen = getCurrentFullscreen();
  
  {
    {
      vector<pair<string, pair<int, int> > > resoptions;
      vector<pair<int, int> > resses = getResolutions();
      for(int i = 0; i < resses.size(); i++)
        resoptions.push_back(make_pair(StringPrintf("%dx%d", resses[i].first, resses[i].second), resses[i]));
      optionsmenu.pushMenuItem(StdMenuItemChooser<pair<int, int> >::make("Resolution", resoptions, &opts_res, bind(&InterfaceMain::opts_res_changed, this, _1)));
    }
    
    {
      vector<pair<string, bool> > onoff;
      onoff.push_back(make_pair("On", true));
      onoff.push_back(make_pair("Off", false));
      optionsmenu.pushMenuItem(StdMenuItemChooser<bool>::make("Fullscreen", onoff, &opts_fullscreen));
    }
    
    opts_aspect_chooser = StdMenuItemChooser<float>::make("Aspect ratio", gen_aspects(640, 480), &opts_aspect);
    optionsmenu.pushMenuItem(opts_aspect_chooser);
    
    optionsmenu.pushMenuItem(StdMenuItemBack::make("Accept", OPTS_SETRES));
    optionsmenu.pushMenuItemAdjacent(StdMenuItemBack::make("Cancel"));
    
    mainmenu.pushMenuItem(StdMenuItemSubmenu::make("Options", &optionsmenu));
  }
  
  mainmenu.pushMenuItem(StdMenuItemTrigger::make("Input test", MAIN_INPUTTEST));
  mainmenu.pushMenuItem(StdMenuItemTrigger::make("Exit", MAIN_EXIT));
  
  if(FLAGS_startingPhase == -1)
    start = 0;
  else
    start = Coord(FLAGS_startingPhase);
  end = 7;
  moneyexp = Coord(0.1133);
  
  faction = FLAGS_factionMode + 1;
  CHECK(faction >= 0 && faction < 5);
  
  if(faction == 1)
    faction_toggle = false;
  else
    faction_toggle = true;
  
  grid = false;
  inptest = false;
  game = NULL;
  
  {
    introscreen->players.resize(16);
    vector<const IDBFaction *> idbfa = ptrize(factionList());
    for(int i = 0; i < introscreen->players.size(); i++) {
      int fid = unsync().choose(idbfa.size());
      introscreen->players[i] = Player(idbfa[fid], 0, Money(0));
      idbfa.erase(idbfa.begin() + fid);
    }
  }
  
  introscreen->game.initTitlescreen(&introscreen->players, &unsync());
  
  for(int i = 0; i < introscreen->players.size(); i++)
    introscreen_ais.push_back(new GameAiIntro());
  
  escmenu.pushMenuItem(StdMenuItemBack::make("Return to game"));
  escmenu.pushMenuItem(StdMenuItemSubmenu::make("Options", &optionsmenu));
  escmenu.pushMenuItem(StdMenuItemTrigger::make("Main menu", ESCMENU_MAINMENU));
  escmenu.pushMenuItem(StdMenuItemTrigger::make("Quit", ESCMENU_QUIT));
  
  for(int i = 0; i < 30; i++)
    introscreen->runTickWithAi(vector<GameAi*>(introscreen_ais.begin(), introscreen_ais.end()), &unsync());
}
InterfaceMain::InterfaceMain() {
  introscreen = NULL;
  inescmenu = false;
  init();
}

InterfaceMain::~InterfaceMain() {
  dprintf("Deleting metagame\n");
  delete game;
  dprintf("Metagame deleted\n");
}

