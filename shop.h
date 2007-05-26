#ifndef DNET_SHOP
#define DNET_SHOP

#include "shop_info.h"
#include "player.h"

using namespace std;

class Ai;

class ShopPlacement {
public:
  int depth;
  int current;
  int siblings;
  int active;

  ShopPlacement(int depth, int current, int siblings, int active) : depth(depth), current(current), siblings(siblings), active(active) { };
  ShopPlacement() { };
};
  
class ShopLayout {
private:

  float aspect;
  
  float cint_fontsize;
  float cint_itemheight;

  float cint_height;
  
  float int_xofs;
  vector<float> int_expandy;
  vector<pair<float, pair<bool, bool> > > int_scroll;
  
  bool miniature;
  
  // Misc
  float expandy(int depth) const;
  float scrollpos(int depth) const;
  pair<bool, bool> scrollmarkers(int depth) const;
  
  // X dimension functions
  float framestart(int depth) const;
  float frameend(int depth) const;
  float framewidth(int depth) const;
  
  float xmargin() const;
  
  float boxstart(int depth) const;
  float boxend(int depth) const;
  float boxwidth(int depth) const;
  
  float quantx(int depth) const;
  float pricex(int depth) const;
  
  // Y dimension functions
  float ystart() const;
  float yend() const;
  float ymargin() const;
  
  // XY functions
  Float4 zone(int depth) const;
  float border() const;
  
  // Bigger things
  float itemypos(const ShopPlacement &place) const;
  vector<pair<int, float> > getPriorityAndPlacement(const ShopPlacement &place) const;
  
  // Functions that do things
  void drawMarkerSet(int depth, bool down) const;

public:

  // Static-zoom XY functions  
  Float2 cashpos() const;
    
  Float4 hud() const;
  Float4 demo() const;

  // Dynamic-zoom XY functions  
  Float4 box(const ShopPlacement &place) const;
  Float4 boximplantupgrade(const ShopPlacement &place) const;

  Float2 description(const ShopPlacement &place) const;
  Float2 descriptionimplantupgrade(const ShopPlacement &place) const;
  Float2 quantity(const ShopPlacement &place) const;
  Float2 price(const ShopPlacement &place) const;

  // Scalars, flags
  float fontsize() const { return cint_fontsize; };
  float boxthick() const;
  vector<int> renderOrder(const ShopPlacement &place) const;  // "current" is ignored

  void drawScrollMarkers(int depth) const;
  Float4 getScrollBBox(int depth) const;
  
  // Updating
  void updateExpandy(int depth, bool this_branches);
  void updateScroll(const vector<int> &depth, const vector<int> &items);
  
  // Zoom functions
  void staticZoom() const;
  void dynamicZoom() const;
  
  ShopLayout();
  ShopLayout(bool miniature, float aspect);
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
  bool runTick(const Keystates &keys, Player *player);
  void ai(Ai *ai, const Player *player) const;
  void renderToScreen(const Player *player) const;

  void init(bool miniature, const Player *player, int players, Money highestPlayerCount, float aspectRatio);

  Shop();

private:
  Shop(const Shop &rhs);
  void operator=(const Shop &rhs);
};

#endif
