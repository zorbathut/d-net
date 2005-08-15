#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include <string>
#include <vector>

using namespace std;

class ProjectileClass {
public:
    int velocity;
    int damage;
};

class Weapon {
public:
    int firerate;
    float costpershot;
    ProjectileClass *projectile;
};

class Upgrade {
public:
    int hull;
    int engine;
    int handling;
};

class HierarchyNode {
public:
    vector<HierarchyNode> branches;

    string name;

    enum {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_DONE, HNT_LAST};
    int type;

    enum {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_LAST};
    int displaymode;

    bool buyable;
    int cost;
    
    Weapon *weapon;
    Upgrade *upgrade;
    
    int quantity;
    
    int cat_restrictiontype;
    
    void checkConsistency() const;
    
    HierarchyNode();
    
};

void initItemdb();

const HierarchyNode &itemDbRoot();

#endif
