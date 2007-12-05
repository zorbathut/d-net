
#include "shop_layout.h"
#include "util.h"
#include "gfx.h"

float ShopLayout::expandy(int tier) const {
  CHECK(tier >= 0);
  if(tier < int_expandy.size())
    return int_expandy[tier];
  return 1.0;
}

float ShopLayout::scrollpos(int tier) const {
  CHECK(tier >= 0);
  if(tier < int_scroll.size())
    return int_scroll[tier].first * expandy(tier);
  return 0.0;
}

pair<bool, bool> ShopLayout::scrollmarkers(int tier) const {
  CHECK(tier >= 0);
  if(expandy(tier) != 1.0)
    return make_pair(false, false);
  if(tier < int_scroll.size())
    return int_scroll[tier].second;
  return make_pair(false, false);
}

float ShopLayout::framestart(int depth) const {
  if(depth == 0)
    return 0;
  return frameend(depth - 1);
}

float ShopLayout::frameend(int depth) const {
  if(depth == 0)
    return framewidth(depth);
  return frameend(depth - 1) + framewidth(depth);
}

const float leftside = 0.42;
const float rightside = 1.0 - leftside;

// this is probably O(n^2) or something
float ShopLayout::framewidth(int depth) const {
  float start = 0;
  if(depth > 0)
    start = frameend(depth - 1);
  
  float ls = leftside * cint_height * aspect;
  float rs = rightside * cint_height * aspect;
  
  if(int_xofs >= start)
    return ls;
  if(int_xofs < start - ls)
    return rs;
  //dprintf("fw: start %f, xofs %f, ls rs %f %f, ret %f, height/aspect/ls %f %f %f\n", start, int_xofs, ls, rs, lerp(ls, rs, (start - int_xofs) / ls), cint_height, aspect, leftside);
  return lerp(ls, rs, (start - int_xofs) / ls);
}

float ShopLayout::xmargin() const {
  return cint_itemheight / 2;
}

float ShopLayout::boxstart(int depth) const {
  return framestart(depth) + xmargin() / 2;
}
float ShopLayout::boxend(int depth) const {
  return frameend(depth) - xmargin() / 2;
}
float ShopLayout::boxwidth(int depth) const {
  return boxend(depth) - boxstart(depth);
}

float ShopLayout::quantx(int depth) const {
  return lerp(boxstart(depth) + border(), boxend(depth) - border(), 0.68);
}
float ShopLayout::pricex(int depth) const {
  return boxend(depth) - border();
}

float ShopLayout::ystart() const {
  return cint_fontsize * 2;
}
float ShopLayout::yend() const {
  return cint_height - cint_fontsize;
}

const float marginproportion = 0.25;
float ShopLayout::ymargin() const {
  return (cint_itemheight - cint_fontsize) / 2 * marginproportion;
}

Float4 ShopLayout::zone(int depth) const {
  return Float4(framestart(depth), ystart(), frameend(depth), yend());
}
float ShopLayout::border() const {
  return (cint_itemheight - cint_fontsize) / 2 * (1.0 - marginproportion);
}

float ShopLayout::itemypos(const ShopPlacement &place) const {
  return ystart() + (place.current - scrollpos(place.depth)) * cint_itemheight * expandy(place.depth);
}

vector<pair<int, float> > ShopLayout::getPriorityAndPlacement(const ShopPlacement &place) const {
  vector<pair<int, float> > rendpos;
  if(place.siblings) {
    int desiredfront = place.active;
    for(int i = 0; i < desiredfront; i++)
      rendpos.push_back(make_pair(i, itemypos(ShopPlacement(place.depth, i, place.siblings, place.current))));
    for(int i = place.siblings - 1; i > desiredfront; i--)
      rendpos.push_back(make_pair(i, itemypos(ShopPlacement(place.depth, i, place.siblings, place.current))));
    rendpos.push_back(make_pair(desiredfront, itemypos(ShopPlacement(place.depth, desiredfront, place.siblings, place.current))));
  }
  return rendpos;
}

void ShopLayout::drawMarkerSet(int depth, bool down) const {
  StackString ss("slayout dms");
  Float4 zon = zone(depth);
  
  string text;
  if(!down) {
    text = "Up";
    zon.ey = zon.sy + (cint_itemheight - ymargin() * 2);
  } else {
    text = "Down";
    zon.sy = zon.ey - (cint_itemheight - ymargin() * 2);
  }
  
  // Center
  {
    Float4 czon = zon;
    czon.sx = (zon.sx * 2 + zon.ex) / 3;
    czon.ex = (zon.sx + zon.ex * 2) / 3;
    drawSolid(czon);
    setColor(C::box_border);
    drawRect(czon, boxthick());
    setColor(C::active_text);
    drawJustifiedText(text, cint_fontsize, czon.midpoint(), TEXT_CENTER, TEXT_CENTER);
  }
  
  vector<float> beef;
  beef.push_back((zon.sx * 5 + zon.ex) / 6);
  beef.push_back((zon.sx + zon.ex * 5) / 6);
  for(int i = 0; i  < beef.size(); i++) {
    vector<Float2> tri;
    if(!down) {
      tri.push_back(Float2(beef[i], zon.sy));
      tri.push_back(Float2(beef[i] + zon.span_y(), zon.ey));
      tri.push_back(Float2(beef[i] - zon.span_y(), zon.ey));
    } else {
      tri.push_back(Float2(beef[i], zon.ey));
      tri.push_back(Float2(beef[i] + zon.span_y(), zon.sy));
      tri.push_back(Float2(beef[i] - zon.span_y(), zon.sy));
    }
    drawSolidLoop(tri);
    drawLineLoop(tri, boxthick());
  }
}

Float2 ShopLayout::cashpos() const {
  if(miniature)
    return Float2(50, 1);
  else
    return Float2(60, 1);
}

Float4 ShopLayout::hud() const {
  return Float4(xmargin() + cint_fontsize / 2, ystart() + cint_fontsize * 3, leftside * cint_height * aspect - (xmargin() + cint_fontsize / 2), demo().sy);
}
Float4 ShopLayout::demo() const {
  const float xpadding = cint_fontsize * 4;
  Float4 dempos;
  dempos.sx = xmargin() + xpadding;
  dempos.ex = leftside * cint_height * aspect - (xmargin() + xpadding);
  dempos.ey = yend();
  dempos.sy = dempos.ey - dempos.span_x();
  return dempos;
}

Float4 ShopLayout::box(const ShopPlacement &place) const {
  float iyp = itemypos(place);
  return Float4(boxstart(place.depth), iyp + ymargin(), boxend(place.depth), iyp + cint_itemheight - ymargin());
}
Float4 ShopLayout::boximplantupgrade(const ShopPlacement &place) const {
  float iyp = itemypos(place);
  return Float4(boxstart(place.depth) + boxwidth(place.depth) / 5, iyp + ymargin(), boxend(place.depth), iyp + cint_itemheight - ymargin());
}
float ShopLayout::boxfade(const ShopPlacement &place) const {
  return expandy(place.depth);
}

Float2 ShopLayout::description(const ShopPlacement &place) const {
  return box(place).s() + Float2(border(), border());
}
Float2 ShopLayout::descriptionimplantupgrade(const ShopPlacement &place) const {
  return boximplantupgrade(place).s() + Float2(border(), border());
}
Float2 ShopLayout::quantity(const ShopPlacement &place) const {
  return Float2(quantx(place.depth), box(place).sy + border());
}
Float2 ShopLayout::price(const ShopPlacement &place) const {
  return Float2(pricex(place.depth), box(place).sy + border());
}

Float4 ShopLayout::textbox(int depth) const {
  return Float4(lerp(boxstart(depth), boxend(depth), 0.05), ystart(), lerp(boxstart(depth), boxend(depth), 1.0 - 0.05), yend());
}

float ShopLayout::boxthick() const {
  return cint_fontsize / 10;
}
vector<int> ShopLayout::renderOrder(const ShopPlacement &place) const {
  vector<pair<int, float> > ifp = getPriorityAndPlacement(place);
  vector<int> rv;
  for(int i = 0; i < ifp.size(); i++)
    rv.push_back(ifp[i].first);
  return rv;
}

void ShopLayout::drawScrollMarkers(int depth) const {
  if(scrollmarkers(depth).first) {
    drawMarkerSet(depth, false);
  }
  if(scrollmarkers(depth).second) {
    drawMarkerSet(depth, true);
  }
}

Float4 ShopLayout::getScrollBBox(int depth) const {
  Float4 zon = zone(depth);
  if(scrollmarkers(depth).first)
    zon.sy += cint_itemheight;
  if(scrollmarkers(depth).second)
    zon.ey -= cint_itemheight;
  return zon;
}

void ShopLayout::updateExpandy(int depth, bool this_branches) {
  int_xofs = approach(int_xofs, framestart(max(depth - 2 + this_branches, 0)), 10);  // this moves the screen left and right
  
  { // this controlls expansion of categories
    int sz = max((int)int_expandy.size(), depth + 1);
    int_expandy.resize(sz, 1.0);
    vector<float> nexpandy(sz, 1.0);
    if(!this_branches && depth >= 2)
      nexpandy[depth - 2] = 0.0;
    for(int i = 0; i < int_expandy.size(); i++)
      int_expandy[i] = approach(int_expandy[i], nexpandy[i], 0.2);
  }
}

void ShopLayout::updateScroll(const vector<int> &curpos, const vector<int> &options) {
  const float max_rows = (yend() - ystart()) / cint_itemheight;
  
  if(int_scroll.size() < options.size()) {
    int_scroll.resize(options.size(), make_pair(0, make_pair(false, false)));
  }
  
  vector<int> vcurpos = curpos;
  if(vcurpos.size() < int_scroll.size())
    vcurpos.resize(int_scroll.size(), 0);
  
  vector<int> voptions = options;
  if(voptions.size() < int_scroll.size())
    voptions.resize(int_scroll.size(), 0);
  
  CHECK(vcurpos.size() == int_scroll.size());
  CHECK(vcurpos.size() == voptions.size());
 
  for(int i = 0; i < vcurpos.size(); i++) {
    float diff = abs(int_scroll[i].first - (vcurpos[i] - max_rows / 2));
    diff = diff / 30;
    if(diff < 0.05)
      diff = 0;
    int_scroll[i].first = clamp(approach(int_scroll[i].first, vcurpos[i] - max_rows / 2, diff), 0, max(0.f, voptions[i] - max_rows));
    int_scroll[i].second.first = (abs(int_scroll[i].first) > 0.01);
    int_scroll[i].second.second = (abs(int_scroll[i].first - max(0.f, voptions[i] - max_rows)) > 0.01);
  }
}

void ShopLayout::staticZoom() const {
  setZoomVertical(0, 0, cint_height);
}
void ShopLayout::dynamicZoom() const {
  setZoomVertical(int_xofs, 0, cint_height);
}

ShopLayout::ShopLayout() {
  // not valid
}

ShopLayout::ShopLayout(bool miniature, float aspect) : aspect(aspect), miniature(miniature) {
  if(miniature) {
    cint_fontsize = 1.95;
    cint_itemheight = 4;
  } else {
    cint_fontsize = 1.5;
    cint_itemheight = 3;
  }
  
  cint_height = 65;
  
  int_xofs = 0;
  int_expandy.resize(2, 1.0); // not really ideal but hey
}
