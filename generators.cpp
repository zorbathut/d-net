
#include "generators.h"

#include "itemdb.h"
#include "itemdb_stream.h"
#include "recorder.h"
#include "player.h"
#include "shop_demo.h"
#include "stream_file.h"
#include "stream_process_vector.h"
#include "stream_process_utility.h"
#include "stream_process_string.h"

#include <deque>

using namespace std;

template<typename T> FileShopcache generateShopCache(const string &itemname, const T &item, float accuracy) {
  dprintf("%s\n", itemname.c_str());
  
  IDBAdjustment adjustment_null;
  
  IDBFaction faction;
  for(int i = 0; i < 4; i++)
    faction.adjustment[i] = &adjustment_null;
  
  Player player(&faction, 0, Money(0));
  
  {
    Recorder recorder;
    
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
      if(ticks >= 1200) {
        bool end = true;
        if(ticks % 3600 == 0)
          dprintf("Tick %d\n", ticks);
        for(int i = 0; i < oldstats.size(); i++) {
          float high = *max_element(oldstats[i].begin(), oldstats[i].end());
          float low = *min_element(oldstats[i].begin(), oldstats[i].end());
          float ratdiff = low / high;
          float absdiff = high - low;
          if(ratdiff < accuracy && absdiff > 0.01)
            end = false;
          if(ticks % 36000 == 0)
            dprintf("%f (%f) and %f (%f)", ratdiff, accuracy, absdiff, 0.01);
        }
        if(end && !recorder.hasItems()) {
          CHECK(ticks < 36000);
          end = false;
        }
        if(end) {
          demo.dumpMetastats(&recorder);
          dprintf("Done at %d ticks\n", ticks);
          return recorder.data();
        } else {
          for(int i = 0; i < oldstats.size(); i++)
            oldstats[i].erase(oldstats[i].begin());
        }
      }
    }
  }
}

void generateCachedShops(float accuracy) {
  
  const int gencount = weaponList().size() + bombardmentList().size() + gloryList().size();
  int gendone = 0;
  
  vector<pair<string, FileShopcache> > rsis;
  
  for(map<string, IDBWeapon>::const_iterator itr = weaponList().begin(); itr != weaponList().end(); itr++) {
    dprintf("%d/%d (%.0f%%)\n", gendone, gencount, (float)gendone / gencount * 100);
    gendone++;
    if(itr->second.nocache)
      continue;
    rsis.push_back(make_pair(itr->first, generateShopCache(itr->first, itr->second, accuracy)));
  }
  
  for(map<string, IDBBombardment>::const_iterator itr = bombardmentList().begin(); itr != bombardmentList().end(); itr++) {
    dprintf("%d/%d (%.0f%%)\n", gendone, gencount, (float)gendone / gencount * 100);
    gendone++;
    rsis.push_back(make_pair(itr->first, generateShopCache(itr->first, itr->second, accuracy)));
  }
  
  for(map<string, IDBGlory>::const_iterator itr = gloryList().begin(); itr != gloryList().end(); itr++) {
    dprintf("%d/%d (%.0f%%)\n", gendone, gencount, (float)gendone / gencount * 100);
    gendone++;
    rsis.push_back(make_pair(itr->first, generateShopCache(itr->first, itr->second, accuracy)));
  }
  
  {
    OStreamFile ofil("data/shopcache.dwh");
    ofil.write(rsis);
  }
  
  {
    vector<pair<string, FileShopcache> > rsis2;
    IStreamFile ifil("data/shopcache.dwh");
    ifil.read(&rsis2);
    CHECK(rsis == rsis2);
    
    for(int i = 0; i < rsis.size(); i++)
      for(int j = 0; j < rsis[i].second.entries.size(); j++)
        CHECK(rsis[i].second.entries[j].impact >= 0 && rsis[i].second.entries[j].impact < 16 || rsis[i].second.entries[j].impact == -1);
  }
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
