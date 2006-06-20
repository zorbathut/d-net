#ifndef DNET_SHOP
#define DNET_SHOP

#include "player.h"
#include "input.h"

class Ai;

class Shop {
private:
  HierarchyNode dynamic_equip;

  Player *player;

  vector<int> curloc;

  float xofs;
  vector<float> expandy;

  bool selling;
  bool disabled;  // when we switch, we wait for them to let go of the button

  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCurNode() const;
  const HierarchyNode &getCategoryNode() const;

  void doTableUpdate();
  void doTableRender() const;
  void renderNode(const HierarchyNode &node, int depth) const;

public:
  bool runTick(const Keystates &keys);
  void ai(Ai *ai) const;
  void renderToScreen() const;

  Shop();
  Shop(Player *player);
};

#endif
