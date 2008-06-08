#ifndef DNET_ITEMDB_PRIVATE
#define DNET_ITEMDB_PRIVATE

#include "itemdb.h"

using namespace std;

extern HierarchyNode root;
extern map<string, IDBDeploy> deployclasses;
extern map<string, IDBWarhead> warheadclasses;
extern map<string, IDBProjectile> projectileclasses;
extern map<string, IDBLauncher> launcherclasses;
extern map<string, IDBWeapon> weaponclasses;
extern map<string, IDBUpgrade> upgradeclasses;
extern map<string, IDBGlory> gloryclasses;
extern map<string, IDBBombardment> bombardmentclasses;
extern map<string, IDBTank> tankclasses;
extern map<string, IDBAdjustment> adjustmentclasses;
extern map<string, IDBEffects> effectsclasses;
extern map<string, IDBImplantSlot> implantslotclasses;
extern map<string, IDBImplant> implantclasses;
extern map<string, IDBInstant> instantclasses;

extern vector<IDBFaction> factions;
extern map<string, string> text;

extern const IDBTank *deftank;
extern const IDBGlory *defglory;
extern const IDBBombardment *defbombardment;

extern map<string, IDBShopcache> shopcaches;

#endif
