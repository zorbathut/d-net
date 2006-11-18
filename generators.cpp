
#include "generators.h"

#include "itemdb.h"
#include "recorder.h"
#include "player.h"
#include "shop_demo.h"

using namespace std;

template<typename T> void generateShopCache(const string &itemname, const T &item, FILE *ofil, float accuracy) {
  dprintf("%s\n", itemname.c_str());
  
  fprintf(ofil, "shopcache {\n  itemname=%s\n", itemname.c_str());
  
  IDBAdjustment adjustment_null;
  
  IDBFaction faction;
  for(int i = 0; i < 4; i++)
    faction.adjustment[i] = &adjustment_null;
  
  Player player(&faction, 0);
  
  {
    Recorder recorder(ofil);
    
    ShopDemo demo;
    demo.init(&item, &player, &recorder);
    
    vector<float> oldstats = demo.getStats();
    while(1) {
      for(int i = 0; i < 600; i++)
        demo.runSingleTick();
      vector<float> newstats = demo.getStats();
      CHECK(oldstats.size() == newstats.size());
      bool end = true;
      for(int i = 0; i < newstats.size(); i++) {
        float ratdiff = oldstats[i] / newstats[i];
        float absdiff = oldstats[i] - newstats[i];
        if(ratdiff > 1)
          ratdiff = 1 / ratdiff;
        absdiff = abs(absdiff);
        if(ratdiff < accuracy && absdiff > 0.01)
          end = false;
      }
      if(end)
        break;
      dprintf("continuing\n");
      oldstats = newstats;
    }
  }
  
  fprintf(ofil, "}\n\n");
}

void generateCachedShops(float accuracy) {
  FILE *ofil = fopen("data/shopcache.dwh", "w");
  for(map<string, IDBWeapon>::const_iterator itr = weaponList().begin(); itr != weaponList().end(); itr++) {
    generateShopCache(itr->first, itr->second, ofil, accuracy);
  }
  
  for(map<string, IDBBombardment>::const_iterator itr = bombardmentList().begin(); itr != bombardmentList().end(); itr++) {
    generateShopCache(itr->first, itr->second, ofil, accuracy);
  }
  
  for(map<string, IDBGlory>::const_iterator itr = gloryList().begin(); itr != gloryList().end(); itr++) {
    generateShopCache(itr->first, itr->second, ofil, accuracy);
  }
  fclose(ofil);
}

void generateWeaponStats() {
  FILE *ofil = fopen("tools/weapondump.dat", "w");
  IDBAdjustment adj;
  map<string, vector<pair<float, float> > > goof;
  for(map<string, IDBWeapon>::const_iterator itr = weaponList().begin(); itr != weaponList().end(); itr++) {
    IDBWeaponAdjust wa(&itr->second, adj);
    string name = wa.name();
    name = string(name.c_str(), (const char*)strrchr(name.c_str(), ' '));
    if(wa.cost_pack() > Money(0))
      goof[name].push_back(make_pair(wa.stats_damagePerSecond() * itr->second.launcher->stats->dps_efficiency, wa.stats_costPerSecond() * itr->second.launcher->stats->cps_efficiency));
  }
  
  for(map<string, vector<pair<float, float> > >::iterator itr = goof.begin(); itr != goof.end(); itr++) {
    sort(itr->second.begin(), itr->second.end());
    fprintf(ofil, "%s", itr->first.c_str());
    for(int i = 0; i < itr->second.size(); i++)
      fprintf(ofil, ",%f,%f", itr->second[i].first, itr->second[i].second);
    fprintf(ofil, "\n");
  }
  fclose(ofil);
}

void generateFactionStats() {
  FILE *ofil = fopen("tools/factiondump.dat", "w");
  CHECK(ofil);
  for(int j = 0; j < IDBAdjustment::LAST; j++)
    fprintf(ofil, "\t%s", adjust_text[j]);
  fprintf(ofil, "\n");
  for(int i = 0; i < factionList().size(); i++) {
    fprintf(ofil, "%s", factionList()[i].name.c_str());
    for(int j = 0; j < IDBAdjustment::LAST; j++)
      fprintf(ofil, "\t%d", factionList()[i].adjustment[3]->adjusts[j]);
    fprintf(ofil, "\n");
  }
  fclose(ofil);
}

void dumpText() {
  FILE *ofil = fopen("tools/textdump.txt", "w");
  CHECK(ofil);
  for(map<string, string>::const_iterator itr = textList().begin(); itr != textList().end(); itr++) {
    fprintf(ofil, "%s\n\n\n\n", itr->second.c_str());
  }
  fclose(ofil);
}
