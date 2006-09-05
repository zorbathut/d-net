#ifndef DNET_SHOP
#define DNET_SHOP

#include "player.h"
#include "input.h"
#include "shop_info.h"

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
  float boxborder() const { return int_boxborder; };
  float itemheight() const { return int_itemheight; };
  
  float boxwidth() const { return int_boxwidth; };
    
  float boxthick() const { return int_boxthick; };
  
  float xofs() const { return int_xofs; };
  float expandy(int tier) const { return int_expandy[tier]; };
  
  void updateExpandy(int depth, bool this_branches);
  
  ShopLayout();
  ShopLayout(bool miniature);
};

class Shop {
private:
  mutable HierarchyNode dynamic_equip;
  bool miniature;

  Player *player;

  ShopLayout slay;

  vector<int> curloc;
  vector<int> lastloc;

  bool selling;
  bool disabled;  // when we switch, we wait for them to let go of the button

  ShopInfo cshopinf;

  const HierarchyNode &normalize(const HierarchyNode &item) const;
  
  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCategoryNode() const;
  const HierarchyNode &getCurNode() const;

  void doTableRender() const;
  void renderNode(const HierarchyNode &node, int depth) const;

public:
  bool runTick(const Keystates &keys);
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();

  void init(Player *player, bool miniature);

private:
  Shop(const Shop &rhs);
  void operator=(const Shop &rhs);
};

#endif
