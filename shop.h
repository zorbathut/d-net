#ifndef DNET_SHOP
#define DNET_SHOP

#include "player.h"
#include "input.h"
#include "shop_info.h"

class Ai;
  
class ShopLayout {
public:
  float totalwidth;
  
  float hoffset;
  float voffset;
  
  float fontsize;
  float boxborder;
  float itemheight;
  
  float boxwidth;
  
  float pricehpos;
  float quanthpos;
  
  float demowidth;
  float demoxstart;
  float demoystart;
  
  float boxthick;
  
  float hudstart;
  float hudend;
  
  vector<float> expandy;
  
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

  float xofs;

  bool selling;
  bool disabled;  // when we switch, we wait for them to let go of the button

  ShopInfo cshopinf;

  const HierarchyNode &normalize(const HierarchyNode &item) const;
  
  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCategoryNode() const;
  const HierarchyNode &getCurNode() const;

  void doTableUpdate();
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
