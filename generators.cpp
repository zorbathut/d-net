
#include "generators.h"

#include "itemdb.h"
#include "recorder.h"
#include "player.h"
#include "shop_demo.h"

#include <deque>

using namespace std;

template<typename T> void generateShopCache(const string &itemname, const T &item, FILE *ofil, float accuracy) {
  dprintf("%s\n", itemname.c_str());
  
  fprintf(ofil, "shopcache {\n  name=%s\n", itemname.c_str());
  
  IDBAdjustment adjustment_null;
  
  IDBFaction faction;
  for(int i = 0; i < 4; i++)
    faction.adjustment[i] = &adjustment_null;
  
  Player player(&faction, 0, Money(0));
  
  {
    Recorder recorder(ofil);
    
    ShopDemo demo;
    demo.init(&item, &player, &recorder);
    
    vector<deque<float> > oldstats(demo.getStats().size());
    int ticks = 0;
    while(1) {
      ticks++;
      demo.runSingleTick();
      {
        vector<float> nst = demo.getStats();
        for(int i = 0; i < nst.size(); i++)
          oldstats[i].push_back(nst[i]);
      }
      if(ticks >= 600) {
        bool end = true;
        for(int i = 0; i < oldstats.size(); i++) {
          float high = *max_element(oldstats[i].begin(), oldstats[i].end());
          float low = *min_element(oldstats[i].begin(), oldstats[i].end());
          float ratdiff = low / high;
          float absdiff = high - low;
          if(ratdiff < accuracy && absdiff > 0.01)
            end = false;
        }
        if(end) {
          demo.dumpMetastats(&recorder);
          dprintf("Done at %d ticks\n", ticks);
          break;
        } else {
          for(int i = 0; i < oldstats.size(); i++)
            oldstats[i].erase(oldstats[i].begin());
        }
      }
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
