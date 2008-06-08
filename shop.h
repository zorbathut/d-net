#ifndef DNET_SHOP
#define DNET_SHOP

#include "shop_info.h"
#include "shop_layout.h"

using namespace std;

class Ai;

class Shop {
private:
  bool miniature;

  ShopLayout slay;

  vector<int> curloc;
  vector<int> lastloc;

  const IDBWeapon *equipselected;

  HierarchyNode hierarchroot;

  // these two are just for generating the shop
  int playercount;
  Money highestcash;

  ShopInfo cshopinf;

  void renormalize(HierarchyNode &item, const Player *player, int playercount, Money highestcash);
  
  const HierarchyNode &getStepNode(int step) const;

  const HierarchyNode &getCategoryNode() const;
  const HierarchyNode &getCurNode() const;

  void doTableRender(const Player *player) const;
  void renderNode(const HierarchyNode &node, int depth, const Player *player) const;
  
  bool hasInfo(int type) const;

public:
  bool runTick(const Keystates &keys, Player *player, int playercount);
  void ai(Ai *ai, const Player *player) const;
  void renderToScreen(const Player *player) const;
  void checksum(Adler32 *adler) const;

  void init(bool miniature, const Player *player, int players, Money highestPlayerCount, float aspectRatio);

  Shop();

private:
  Shop(const Shop &rhs);
  void operator=(const Shop &rhs);
};

#endif
