
#include "interface_stdmenu.h"

#include "audio.h"
#include "gfx.h"
#include "adler32.h"
#include "adler32_util.h"
#include "audit.h"

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

StdMenuItemTrigger::StdMenuItemTrigger(const string &text, int trigger) : name(text), trigger(trigger) { };

smart_ptr<StdMenuItemTrigger> StdMenuItemTrigger::make(const string &text, int trigger) { return smart_ptr<StdMenuItemTrigger>(new StdMenuItemTrigger(text, trigger)); }

pair<StdMenuCommand, int> StdMenuItemTrigger::tickItem(const Keystates *keys) {
  if(keys && keys->accept.push) {
    queueSound(S::accept);
    return make_pair(SMR_NOTHING, trigger);
  }
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}

float StdMenuItemTrigger::renderItemWidth(float tmx) const {
  return getTextWidth(name, 4);
}

void StdMenuItemTrigger::renderItem(const Float4 &bounds) const {
  drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
}

void StdMenuItemTrigger::checksum(Adler32 *adl) const {
  adler(adl, "trigger");
  adler(adl, name);
  adler(adl, trigger);
}

/*************
 * Scale
 */

StdMenuItemScale::ScaleDisplayer::ScaleDisplayer(const vector<string> &labels, const Coord *start, const Coord *end, const bool *onstart, bool mini) : labels(labels), start(start), end(end), onstart(onstart), mini(mini) { };
StdMenuItemScale::ScaleDisplayer::ScaleDisplayer() { };

StdMenuItemScale::StdMenuItemScale(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos) : name(text), position(position), munge(munge), displayer(sds), selected_val(selected_val), selected_pos(selected_pos) { };

smart_ptr<StdMenuItemScale> StdMenuItemScale::make(const string &text, Coord *position, const function<Coord (const Coord &)> &munge, const ScaleDisplayer &sds, bool selected_val, bool *selected_pos) { return smart_ptr<StdMenuItemScale>(new StdMenuItemScale(text, position, munge, sds, selected_val, selected_pos)); }
  
pair<StdMenuCommand, int> StdMenuItemScale::tickItem(const Keystates *keys) {
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
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}

float StdMenuItemScale::renderItemWidth(float tmx) const {
  return getTextWidth(name, 4);
}

void StdMenuItemScale::renderItem(const Float4 &bounds) const {
  drawText(name.c_str(), 4, bounds.s());
  
  Float4 boundy = Float4(bounds.sx + 35, bounds.sy, bounds.ex, bounds.sy + 4);
  GfxWindow gfxw(boundy, 1.0);
  
  displayer.render(position->toFloat());
}

void StdMenuItemScale::checksum(Adler32 *adl) const {
  adler(adl, "scale");
  adler(adl, name);
  CHECK(position);
  adler(adl, *position);
  adler(adl, selected_val);
  CHECK(selected_pos);
  adler(adl, *selected_pos);
}

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
    setZoomAround(Float4(-0.5, -0.2, labels.size() + 0.5 - 1, 0.16));
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

StdMenuItemRounds::StdMenuItemRounds(const string &text, Coord *start, Coord *end, Coord *exp, int *rounds) : name(text), start(start), end(end), expv(exp), rounds(rounds) { *rounds = calculateRounds(*start, *end, *expv); };

smart_ptr<StdMenuItemRounds> StdMenuItemRounds::make(const string &text, Coord *start, Coord *end, Coord *exp, int *rounds) { return smart_ptr<StdMenuItemRounds>(new StdMenuItemRounds(text, start, end, exp, rounds)); }

pair<StdMenuCommand, int> StdMenuItemRounds::tickItem(const Keystates *keys) {
  if(keys && keys->l.down)
    *expv *= Coord(101) / 100;
  if(keys && keys->r.down)
    *expv /= Coord(101) / 100;
  *expv = clamp(*expv, Coord(0.001), 2);
  
  *rounds = calculateRounds(*start, *end, *expv);
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}
float StdMenuItemRounds::renderItemWidth(float tmx) const {
  return tmx;
}
void StdMenuItemRounds::renderItem(const Float4 &bounds) const {
  drawText(name.c_str(), 4, bounds.s());
  float percentage = (exp(expv->toFloat()) - 1) * 100;
  drawJustifiedText(StringPrintf("%d (+%.2f%% cash/round)", calculateRounds(*start, *end, *expv), percentage), 4, Float2(bounds.ex, bounds.sy), TEXT_MAX, TEXT_MIN);
}

void StdMenuItemRounds::checksum(Adler32 *adl) const {
  adler(adl, "rounds");
  adler(adl, name);
  adler(adl, *start);
  adler(adl, *end);
  adler(adl, *expv);
}

/*************
 * Chooser
 */

template<typename T> void StdMenuItemChooser<T>::syncoptions() {
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

template<typename T> StdMenuItemChooser<T>::StdMenuItemChooser(const string &text, const vector<pair<string, T> > &options, T *storage, optional<function<void (T)> > changefunctor) : name(text), options(options), storage(storage), changefunctor(changefunctor) {
  CHECK(storage);
  syncoptions();
}

template<typename T> smart_ptr<StdMenuItemChooser<T> > StdMenuItemChooser<T>::make(const string &text, const vector<pair<string, T> > &options, T *storage) { return smart_ptr<StdMenuItemChooser<T> >(new StdMenuItemChooser<T>(text, options, storage, optional<function<void (T)> >())); }
template<typename T> smart_ptr<StdMenuItemChooser<T> > StdMenuItemChooser<T>::make(const string &text, const vector<pair<string, T> > &options, T *storage, function<void (T)> changefunctor) { return smart_ptr<StdMenuItemChooser<T> >(new StdMenuItemChooser<T>(text, options, storage, changefunctor)); }

template<typename T> pair<StdMenuCommand, int> StdMenuItemChooser<T>::tickItem(const Keystates *keys) {
  if(keys && keys->l.push)
    item--;
  if(keys && keys->r.push)
    item++;
  
  item = modurot(item, options.size());
  
  *storage = options[item].second;
  
  if(changefunctor)
    (*changefunctor)(*storage);
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}
template<typename T> float StdMenuItemChooser<T>::renderItemWidth(float tmx) const {
  return maxx;
}
template<typename T> void StdMenuItemChooser<T>::renderItem(const Float4 &bounds) const {
  drawText(name, 4, Float2(bounds.sx, bounds.sy));
  drawJustifiedText(options[item].first, 4, Float2(bounds.ex, bounds.sy), TEXT_MAX, TEXT_MIN);
}

template<typename T> void StdMenuItemChooser<T>::changeOptionDb(const vector<pair<string, T> > &newopts) {
  options = newopts;
  
  syncoptions();
}

template<typename T> void StdMenuItemChooser<T>::checksum(Adler32 *adl) const {
  adler(adl, "chooser");
  adler(adl, name);
  for(int i = 0; i < options.size(); i++) {
    adler(adl, options[i].first);
  }
  adler(adl, item);
}

/*************
 * Counter
 */

vector<pair<string, int> > StdMenuItemCounter::makeOpts(int low, int high) {
  vector<pair<string, int> > rv;
  for(int i = low; i <= high; i++)
    rv.push_back(make_pair(StringPrintf("%d", i), i));
  return rv;
}

StdMenuItemCounter::StdMenuItemCounter(const string &text, int *storage, int low, int high) : StdMenuItemChooser<int>(text, makeOpts(low, high), storage, optional<function<void (int)> >()) { };

smart_ptr<StdMenuItemCounter> StdMenuItemCounter::make(const string &text, int *storage, int low, int high) { return smart_ptr<StdMenuItemCounter>(new StdMenuItemCounter(text, storage, low, high)); }

/*************
 * Submenu
 */

StdMenuItemSubmenu::StdMenuItemSubmenu(const string &text, StdMenu menu, int signal) : name(text), submenu(menu), submenu_ptr(NULL), signal(signal) { };
StdMenuItemSubmenu::StdMenuItemSubmenu(const string &text, StdMenu *menu, int signal) : name(text), submenu_ptr(menu), signal(signal) { };

StdMenu &StdMenuItemSubmenu::gsm() {
  if(submenu_ptr) return *submenu_ptr;
  return submenu;
}

const StdMenu &StdMenuItemSubmenu::gsm() const {
  if(submenu_ptr) return *submenu_ptr;
  return submenu;
}

smart_ptr<StdMenuItemSubmenu> StdMenuItemSubmenu::make(const string &text, StdMenu menu, int signal) { return smart_ptr<StdMenuItemSubmenu>(new StdMenuItemSubmenu(text, menu, signal)); }
smart_ptr<StdMenuItemSubmenu> StdMenuItemSubmenu::make(const string &text, StdMenu *menu, int signal) { return smart_ptr<StdMenuItemSubmenu>(new StdMenuItemSubmenu(text, menu, signal)); }

pair<StdMenuCommand, int> StdMenuItemSubmenu::tickEntire(const Keystates &keys) {
  return gsm().tick(keys);
}
void StdMenuItemSubmenu::renderEntire(const Float4 &bounds, bool obscure) const {
  gsm().render(bounds, obscure);
}

pair<StdMenuCommand, int> StdMenuItemSubmenu::tickItem(const Keystates *keys) {
  if(keys && keys->accept.push) {
    queueSound(S::accept);
    gsm().reset();
    return make_pair(SMR_ENTER, signal);
  }
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}
float StdMenuItemSubmenu::renderItemWidth(float tmx) const {
  return getTextWidth(name, 4);
}
void StdMenuItemSubmenu::renderItem(const Float4 &bounds) const {
  drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
}

void StdMenuItemSubmenu::checksum(Adler32 *adl) const {
  adler(adl, "submenu");
  adler(adl, name);
  submenu.checksum(adl);
  if(submenu_ptr)
    submenu_ptr->checksum(adl);
}

/*************
 * Back
 */

StdMenuItemBack::StdMenuItemBack(const string &text, int signal) : name(text), signal(signal) { };

smart_ptr<StdMenuItemBack> StdMenuItemBack::make(const string &text, int signal) { return smart_ptr<StdMenuItemBack>(new StdMenuItemBack(text, signal)); }

pair<StdMenuCommand, int> StdMenuItemBack::tickItem(const Keystates *keys) {
  if(keys && keys->accept.push) {
    queueSound(S::choose);
    return make_pair(SMR_RETURN, signal);
  }
  
  return make_pair(SMR_NOTHING, SMR_NOTHING);
}
float StdMenuItemBack::renderItemWidth(float tmx) const {
  return getTextWidth(name, 4);
}
void StdMenuItemBack::renderItem(const Float4 &bounds) const {
  drawJustifiedText(name.c_str(), 4, Float2(bounds.midpoint().x, bounds.sy), TEXT_CENTER, TEXT_MIN);
}

void StdMenuItemBack::checksum(Adler32 *adl) const {
  adler(adl, "back");
  adler(adl, name);
  adler(adl, signal);
}


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
  
  for(int i = 0; i < items.size(); i++)
    for(int j = 0; j < items[i].size(); j++)
      if(i != vpos && j != hpos)
        items[i][j]->tickItem(NULL);
  
  {
    pair<StdMenuCommand, int> rv;
    if(inside) {
      rv = items[vpos][hpos]->tickEntire(keys);
    } else {
      rv = items[vpos][hpos]->tickItem(&keys);
    }
    
    if(rv.first == SMR_NOTHING) {
      return rv;
    } else if(rv.first == SMR_ENTER) {
      inside = true;
      return make_pair(SMR_NOTHING, rv.second);
    } else if(rv.first == SMR_RETURN && !inside) {
      return make_pair(SMR_RETURN, rv.second);
    } else if(rv.first == SMR_RETURN && inside) {
      inside = false;
      return make_pair(SMR_NOTHING, rv.second);
    } else {
      CHECK(0);
    }
  }
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
}

StdMenu::StdMenu() {
  vpos = 0;
  hpos = 0;
  inside = false;
}

void StdMenu::checksum(Adler32 *adl) const {vector<vector<smart_ptr<StdMenuItem> > > items;
  audit(*adl);
  for(int i = 0; i < items.size(); i++) {
    for(int j = 0; j < items[i].size(); j++) {
      items[i][j]->checksum(adl);
    }
  }
  audit(*adl);
  adler(adl, vpos);
  audit(*adl);
  adler(adl, hpos);
  audit(*adl);
  adler(adl, inside);
  audit(*adl);
}

template class StdMenuItemChooser<bool>;
template class StdMenuItemChooser<float>;
template class StdMenuItemChooser<pair<int, int> >;
