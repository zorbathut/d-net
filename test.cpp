
#include "test.h"

#include "debug.h"
#include "collide.h"
#include "itemdb.h"
#include "gamemap.h"
#include "parse.h"

#include <fstream>

using namespace std;

void runTests() {
  dprintf("Tests starting\n");
  
  vector<pair<string, const IDBTank *> > tkz;
  for(map<string, IDBTank>::const_iterator itr = tankList().begin(); itr != tankList().end(); itr++)
    tkz.push_back(make_pair(itr->first, &itr->second));
  
  ifstream ifs("data/levels/levellist.txt");
  CHECK(ifs);
  string line;
  while(getLineStripped(ifs, &line)) {
    dprintf("Loading %s\n", line.c_str());
    
    Level lev = loadLevel("data/levels/" + line);
    
    Gamemap gmp(lev.paths, true);
    Collider collide(1, Coord(20));
    collide.cleanup(COM_PLAYER, gmp.getCollisionBounds(), vector<int>(1, 0));
    gmp.updateCollide(&collide);
    
    dprintf("Gamemap valid\n");
    
    set<pair<Coord2, float> > stt;
    for(map<int, vector<pair<Coord2, float> > >::const_iterator itr = lev.playerStarts.begin(); itr != lev.playerStarts.end(); itr++) {
      for(int i = 0; i < itr->second.size(); i++) {
        stt.insert(itr->second[i]);
      }
    }
    
    vector<pair<Coord2, float> > sttv(stt.begin(), stt.end());
    
    dprintf("Found %d origins\n", stt.size());
    for(int i = 0; i < sttv.size(); i++) {
      for(int j = 0; j < tkz.size(); j++) {
        vector<Coord4> loop;
        vector<Coord2> pts = tkz[j].second->getTankVertices(sttv[i].first, sttv[i].second);
        for(int i = 0; i < pts.size(); i++)
          loop.push_back(Coord4(pts[i], pts[(i + 1) % pts.size()]));
        if(collide.checkSimpleCollision(CGR_TANK, 0, loop)) {
          dprintf("Tank collision! %s with level %s\n", tkz[j].first.c_str(), line.c_str());
          CHECK(0);
        }
      }
    }
    
  }
  
  dprintf("Tests passed\n");
}
