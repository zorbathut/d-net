#ifndef DNET_SHOP
#define DNET_SHOP

#include "player.h"
#include "input.h"
#include "shop_demo.h"

class Ai;

class Shop {
private:
  mutable HierarchyNode dynamic_equip;

  Player *player;

  vector<int> curloc;
  vector<int> lastloc;

  float xofs;
  vector<float> expandy;

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
  
  void drawWeaponStats(const IDBWeapon *weap) const;

public:
  bool runTick(const Keystates &keys);
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();

  void init(Player *player);

private:
  Shop(const Shop &rhs);
  void operator=(const Shop &rhs);
};

#endif
