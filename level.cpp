
#include "level.h"

#include "dvec2.h"
#include "util.h"
#include "gamemap.h"

using namespace std;

Level loadLevel(const string &str) {
  dprintf("Loading %s\n", str.c_str());
  Level rv;
  Dvec2 dv = loadDvec2(str);
  for(int i = 0; i < dv.paths.size(); i++) {
    vector<Float2> tp;
    for(int j = 0; j < dv.paths[i].vpath.size(); j++) {
      CHECK(dv.paths[i].vpath[j].curvl == false && dv.paths[i].vpath[j].curvr == false);
      tp.push_back(dv.paths[i].vpath[j].pos + dv.paths[i].center);
    }
    tp.erase(unique(tp.begin(), tp.end()), tp.end());
    CHECK(tp.size());
    if(tp.back() == tp[0])
      tp.pop_back();
    if(!tp.size()) {
      dprintf("WARNING WARNING WARNING");
      dprintf("File %s contains single-node paths and is being loaded as a level! Cats and dogs sleeping together! Fish raining from the sky!", str.c_str());
      dprintf("WARNING WARNING WARNING");
      
      // We do this cleanup so that the CHECK later on can trigger properly
      dv.paths.erase(dv.paths.begin() + i);
      --i;
      continue;
    }
    CHECK(tp.size());
    {
      set<Float2> flup(tp.begin(), tp.end());
      CHECK(flup.size() == tp.size());
    }
    vector<Coord2> rtp;
    for(int i = 0; i < tp.size(); i++)
      rtp.push_back(Coord2(tp[i]));
    rv.paths.push_back(rtp);
  }
  //dprintf("%d paths parsed\n", rv.paths.size());
  CHECK(rv.paths.size() == dv.paths.size());
  {
    map<int, int> entallow;
    for(int i = 0; i < dv.entities.size(); i++) {
      CHECK(dv.entities[i].type == ENTITY_TANKSTART);
      for(int j = 2; j <= dv.entities.size(); j++) {
        // eventually we could have things set up to only allow certain numbers of players on certain maps
        entallow[j]++;
      }
    }
    for(map<int, int>::iterator itr = entallow.begin(); itr != entallow.end(); itr++) {
      if(itr->second >= itr->first) {
        //dprintf("Entities valid for %d players, got %d starts", itr->first, itr->second);
        rv.playersValid.insert(itr->first);
        for(int i = 0; i < dv.entities.size(); i++) {
          CHECK(dv.entities[i].type == ENTITY_TANKSTART);
          rv.playerStarts[itr->first].push_back(make_pair(dv.entities[i].pos, PI * 2 * dv.entities[i].tank_ang_numer / dv.entities[i].tank_ang_denom));
        }
        
      }
    }
  }
  rv.makeProperSolids();
  
  return rv;
}

void Level::makeProperSolids() {
  vector<Coord2> starts;
  for(map<int, vector<pair<Coord2, float> > >::iterator itr = playerStarts.begin(); itr != playerStarts.end(); itr++) {
    for(int i = 0; i < itr->second.size(); i++) {
      starts.push_back(itr->second[i].first);
    }
  }
  sort(starts.begin(), starts.end());
  starts.erase(unique(starts.begin(), starts.end()), starts.end());
  //dprintf("%d unique starts\n", starts.size());
  vector<int> startsin(paths.size());
  for(int i = 0; i < paths.size(); i++) {
    for(int j = 0; j < starts.size(); j++) {
      if(inPath(starts[j], paths[i])) {
        startsin[i]++;
      }
    }
  }
  for(int i = 0; i < startsin.size(); i++)
    CHECK(startsin[i] == 0 || startsin[i] == starts.size());
  CHECK(count(startsin.begin(), startsin.end(), starts.size()) == 1);
  CHECK(count(startsin.begin(), startsin.end(), 0) == startsin.size() - 1);
  for(int i = 0; i < startsin.size(); i++) {
    bool tanksin = (startsin[i] != 0);
    Coord2 ptin = getPointIn(paths[i]);
    //dprintf("tanks is %d, IP is %d\n", tanksin, inPath(ptin, paths[i]));
    bool toggle = (tanksin != (inPath(ptin, paths[i]) == -1));
    if(toggle) {
      //dprintf("Toggling\n");
      reverse(paths[i].begin(), paths[i].end());
    }
  }
  // check to make sure we have a sane set of paths
  for(int i = 0; i < paths.size(); i++) {
    for(int j = i + 1; j < paths.size(); j++) {
      int rel = getPathRelation(paths[i], paths[j]);
      CHECK(rel != PR_INTERSECT);
      if(rel == PR_LHSENCLOSE) {
        CHECK(startsin[i]);
      } else if(rel == PR_RHSENCLOSE) {
        CHECK(startsin[j]);
      }
    }
  }
}

