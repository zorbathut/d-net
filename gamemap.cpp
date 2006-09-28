
#include "gamemap.h"

#include "args.h"
#include "collide.h"
#include "gfx.h"
#include "rng.h"

using namespace std;

DECLARE_bool(debugGraphics);

enum {GMS_EMPTY, GMS_ERASED, GMS_UNCHANGED, GMS_CHANGED};
bool isAvailable(int state) {
  return state == GMS_EMPTY || state == GMS_ERASED;
}
bool isChanged(int state) {
  return state == GMS_ERASED || state == GMS_CHANGED;
}

void Gamemap::render() const {
  CHECK(paths.size());
  setColor(0.5f, 0.5f, 0.5f);
  for(int i = 0; i < paths.size(); i++) {
    if(paths[i].first == GMS_EMPTY)
      continue;
    drawLineLoop(paths[i].second, 0.5);
    /*
    if(FLAGS_debugGraphics) {
      for(int j = 0; j < paths[i].second.size(); j++) {
          int m = (j + 2) % paths[i].size();
          Coord2 jk = paths[i][j] - paths[i][k];
          Coord2 mk = paths[i][m] - paths[i][k];
          Coord ja = getAngle(jk);
          Coord ma = getAngle(mk);
          Coord ad = ma - ja;
          while(ad < 0)
            ad += COORDPI * 2;
          Coord2 out = makeAngle(ja + ad / 2) * 5;
          //dprintf("%f, %f\n", out.x.toFloat(), out.y.toFloat());
          drawLine(paths[i][k], paths[i][k] + out, 2.0);
        }
      }
    }
    */
  }
}


void Gamemap::updateCollide(Collider *collider) {
  for(int i = 0; i < paths.size(); i++) {
    if(paths[i].first == GMS_EMPTY) {
    } else if(paths[i].first == GMS_ERASED) {
      collider->dumpGroup(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      paths[i].first = GMS_EMPTY;
    } else if(paths[i].first == GMS_UNCHANGED) {
    } else if(paths[i].first == GMS_CHANGED) {
      collider->dumpGroup(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      for(int j = 0; j < paths[i].second.size(); j++) {
        int k = (j + 1) % paths[i].second.size();
        collider->addToken(CollideId(CGR_WALL, CGR_WALLOWNER, i), Coord4(paths[i].second[j], paths[i].second[k]), Coord4(0, 0, 0, 0));
      }
      collider->markPersistent(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      paths[i].first = GMS_UNCHANGED;
    }
  }
}

Coord4 Gamemap::getBounds() const {
  Coord4 bounds = startCBoundBox();
  for(int i = 0; i < paths.size(); i++) {
    if(!isAvailable(paths[i].first)) {
      for(int j = 0; j < paths[i].second.size(); j++) {
        addToBoundBox(&bounds, paths[i].second[j]);
      }
    }
  }
  CHECK(bounds.isNormalized());
  return bounds;
}

void Gamemap::removeWalls(Coord2 center, float radius) {
  vector<Coord2> inters;
  {
    vector<float> rv;
    int vct = int(frand() * 3) + 3;
    float ofs = frand() * 2 * PI / vct;
    float maxofs = 2 * PI / vct / 2;
    for(int i = 0; i < vct; i++) {
      rv.push_back(i * 2 * PI / vct + ofs + gaussian_scaled(2) * maxofs);
    }
    for(int i = 0; i < rv.size(); i++)
      inters.push_back(center + makeAngle(Coord(rv[i])) * Coord(radius));
  }
  CHECK(!pathReversed(inters));
  if(set<Coord2>(inters.begin(), inters.end()).size() != inters.size()) {
    // We've gotten two duplicate points, start over from scratch!
    removeWalls(center, radius);
    return;
  }
  
  vector<int> lpaths;
  for(int i = 0; i < paths.size(); i++)
    if(!isAvailable(paths[i].first))
      lpaths.push_back(i);
  
  int fillable = 0;
  for(int i = 0; i < lpaths.size(); i++) {
    int tpath = lpaths[i];
    vector<vector<Coord2> > ntp = getDifference(paths[tpath].second, inters);
    if(ntp.size() == 1 && ntp[0] == paths[tpath].second)
      continue; // NO CHANGE!
    paths[tpath].first = GMS_ERASED;
    fillable = min(fillable, lpaths[i]);
    for(int j = 0; j < ntp.size(); j++) {
      if(abs(getArea(ntp[j])) > 1 || getPerimeter(ntp[j]) > 2) {
        while(fillable != paths.size() && !isAvailable(paths[fillable].first))
          fillable++;
        if(fillable == paths.size())
          paths.push_back(make_pair((int)GMS_EMPTY, vector<Coord2>()));
        CHECK(isAvailable(paths[fillable].first));
        paths[fillable].first = GMS_CHANGED;
        paths[fillable].second = ntp[j];
      }
    }
  }
}

void Gamemap::checkConsistency() const {
  int reversed = 0;
  for(int i = 0; i < paths.size(); i++)
    if(!isAvailable(paths[i].first))
      reversed += pathReversed(paths[i].second);
  
  CHECK(reversed == 1);
  
  // TODO: check intersection?
}

Gamemap::Gamemap() { };
Gamemap::Gamemap(const Level &lev) {
  CHECK(lev.paths.size());
  for(int i = 0; i < lev.paths.size(); i++)
    paths.push_back(make_pair((int)GMS_CHANGED, lev.paths[i]));
}
