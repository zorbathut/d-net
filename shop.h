#ifndef DNET_SHOP
#define DNET_SHOP

#include "shop_info.h"

using namespace std;

class Ai;
  
class ShopLayout {
private:
  float int_totalwidth;
  
  float int_hoffset;
  float int_voffset;
  
  float int_fontsize;
  float int_boxborder;
  float int_itemheight;
  
  float int_boxwidth;
  
  float int_pricehpos;
  float int_quanthpos;
  
  float int_demowidth;
  float int_demoxstart;
  float int_demoystart;
  
  float int_boxthick;
  
  float int_hudstart;
  float int_hudend;
  
  float int_equipDiff;
  
  float int_xofs;
  vector<float> int_expandy;
  
public:
  
  float voffset() const { return int_voffset; };
  
  Float4 box(int depth) const;
  
  Float2 description(int depth) const;
  Float2 quantity(int depth) const;
  Float2 price(int depth) const;
  Float2 equipbit(int depth, int id) const;
  
  Float4 hud() const;
  Float4 demo() const;
  
  float hoffbase(int depth) const;
  
  float fontsize() const { return int_fontsize; };
  float itemheight() const { return int_itemheight; };
  
  float boxwidth() const { return int_boxwidth; };
    
  float boxthick() const { return int_boxthick; };
  
  float expandy(int tier) const { return int_expandy[tier]; };
  
  void updateExpandy(int depth, bool this_branches);
  
  float equipDiff() const { return int_equipDiff; };
  Float2 equip1(int depth) const;
  Float2 equip2(int depth) const;
  
  ShopLayout();
  ShopLayout(bool miniature);
};

class Shop {
private:
  bool miniature;

  ShopLayout slay;

  vector<int> curloc;
  vector<int> lastloc;

  bool selling;
  bool disabled;  // when we switch, we wait for them to let go of the button

  HierarchyNode hierarchroot;

  ShopInfo cshopinf;

  void renormalize(HierarchyNode &item, const Player *player);
  
  const HierarchyNode &getStepNode(int step, const Player *player) const;

  const HierarchyNode &getCategoryNode(const Player *player) const;
  const HierarchyNode &getCurNode(const Player *player) const;

  void doTableRender(const Player *player) const;
  void renderNode(const HierarchyNode &node, int depth, const Player *player) const;
  
  bool hasInfo(int type) const;

public:
  bool runTick(const Keystates &keys, Player *player);
  void ai(Ai *ai, const Player *player) const;
  void renderToScreen(const Player *player) const;

  void init(bool miniature, const HierarchyNode &hnode);

  Shop();

private:
  Shop(const Shop &rhs);
  void operator=(const Shop &rhs);
};

#endif
