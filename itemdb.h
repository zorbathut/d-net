#ifndef DNET_ITEMDB
#define DNET_ITEMDB

#include "gfx.h"

#include <string>
#include <vector>

using namespace std;

struct IDBDeploy {
public:
    
    float anglevariance;

    float getDamagePerShotMultiplier() const;

};

struct IDBWarhead {
public:
    float impactdamage;
    float radiusdamage;
    float radiusfalloff;
    float wallremovalradius;
    float wallremovalchance;

    float getDamagePerShot() const;

};

enum { PM_NORMAL, PM_MISSILE, PM_AIRBRAKE };

struct IDBProjectile {
public:
    int motion;
    float velocity;
    Color color;
    const IDBWarhead *warhead;

    float getDamagePerShot() const;

};

struct IDBWeapon {
public:
    int firerate;
    float costpershot;
    string name;
    const IDBDeploy *deploy;
    const IDBProjectile *projectile;

    float getDamagePerShot() const;
    float getDamagePerSecond() const;
    float getCostPerDamage() const;

};

struct IDBGlory {
public:
    int minsplits;
    int maxsplits;
    int minsplitsize;
    int maxsplitsize;

    const IDBDeploy *deploy;
    const IDBProjectile *projectile;

    int shotspersplit;
};

struct IDBUpgrade {
public:
    int hull;
    int engine;
    int handling;
};

struct HierarchyNode {
public:
    vector<HierarchyNode> branches;

    string name;

    enum {HNT_CATEGORY, HNT_WEAPON, HNT_UPGRADE, HNT_GLORY, HNT_DONE, HNT_LAST};
    int type;

    enum {HNDM_BLANK, HNDM_COST, HNDM_PACK, HNDM_COSTUNIQUE, HNDM_LAST};
    int displaymode;

    bool buyable;
    int cost;
    
    const IDBWeapon *weapon;
    const IDBUpgrade *upgrade;
    const IDBGlory *glory;
    
    int quantity;
    
    int cat_restrictiontype;
    
    void checkConsistency() const;
    
    HierarchyNode();
    
};

void initItemdb();

const HierarchyNode &itemDbRoot();

const IDBWeapon *defaultWeapon();
const IDBGlory *defaultGlory();

#endif
