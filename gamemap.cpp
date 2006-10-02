
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

void Gamemap::render() const {
  CHECK(paths.size());
  setColor(0.5f, 0.5f, 0.5f);
  for(int i = 0; i < paths.size(); i++) {
    if(isAvailable(paths[i].first))
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
      // implicit in CGR_WALL now, but leaving this in just for the time being
      //collider->markPersistent(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      paths[i].first = GMS_UNCHANGED;
    }
  }
}

Coord4 Gamemap::getBounds() const {
  return getInternalBounds();
}

const Coord resolution = Coord(20);
const Coord offset = Coord(1.23456f); // Nasty hack to largely eliminate many border cases

void Gamemap::removeWalls(Coord2 center, float radius) {
  {
    Coord4 bounds = startCBoundBox();
    Coord4 ib = getInternalBounds();
    addToBoundBox(&bounds, ib);
    bounds = bounds + Coord4(-offset, -offset, -offset, -offset);
    bounds = snapToEnclosingGrid(bounds, resolution);
    int nsx = (bounds.sx / resolution).toInt();
    int nsy = (bounds.sy / resolution).toInt();
    int nex = (bounds.ex / resolution).toInt();
    int ney = (bounds.ey / resolution).toInt();
    CHECK(nsx == sx && nsy == sy && nex == ex && ney == ey);
  }
  {
    Coord4 ib = getInternalBounds();
    Coord cradius(radius);
    if(center.x + cradius + 1 > ib.ex || center.x - cradius - 1 < ib.sx || center.y + cradius + 1 > ib.ey || center.y - cradius - 1 < ib.sy) {
      dprintf("probin' at %f,%f %f", center.x.toFloat(), center.y.toFloat(), radius);
      Coord4 bounds = startCBoundBox();
    
      addToBoundBox(&bounds, ib);
      addToBoundBox(&bounds, center + Coord2(radius + 1, radius + 1));
      addToBoundBox(&bounds, center - Coord2(radius + 1, radius + 1));
      dprintf("%f, %f, %f, %f\n", bounds.sx.toFloat(), bounds.sy.toFloat(), bounds.ex.toFloat(), bounds.ey.toFloat());
      dprintf("%f, %f, %f, %f\n", ib.sx.toFloat(), ib.sy.toFloat(), ib.ex.toFloat(), ib.ey.toFloat());
    
      bounds = bounds + Coord4(-offset, -offset, -offset, -offset);
      bounds = snapToEnclosingGrid(bounds, resolution);
      
      int nsx = (bounds.sx / resolution).toInt();
      int nsy = (bounds.sy / resolution).toInt();
      int nex = (bounds.ex / resolution).toInt();
      int ney = (bounds.ey / resolution).toInt();
      CHECK(nsx <= sx);
      CHECK(nsy <= sy);
      CHECK(nex >= ex);
      CHECK(ney >= ey);
      
      {
        vector<vector<int> > nlinks((nex - nsx) * (ney - nsy));
        
        for(int x = nsx; x < nex; x++)
          for(int y = nsy; y < ney; y++)
            if(y >= sy && y < ey && x >= sx && x < ex)
              nlinks[(y - nsy) * (nex - nsx) + x - nsx].swap(links[linkid(x, y)]);
          
        links.swap(nlinks);
      }
      
      int osx = sx;
      int osy = sy;
      int oex = ex;
      int oey = ey;
      
      sx = nsx;
      sy = nsy;
      ex = nex;
      ey = ney;
      
      for(int x = sx; x < ex; x++) {
        for(int y = sy; y < ey; y++) {
          if(y < osy || y >= oey || x < osx || x >= oex) {
            Coord4 tile = getTileBounds(x, y);
            CHECK(links[linkid(x, y)].size() == 0);
            vector<Coord2> innerpath;
            innerpath.push_back(Coord2(tile.sx, tile.sy));
            innerpath.push_back(Coord2(tile.ex, tile.sy));
            innerpath.push_back(Coord2(tile.ex, tile.ey));
            innerpath.push_back(Coord2(tile.sx, tile.ey));
            paths[addPath(x, y)].second = innerpath;
          }
        }
      }
    }
  }

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
  
  {
    Coord4 bounds = startCBoundBox();
    addToBoundBox(&bounds, inters);
    bounds = snapToEnclosingGrid(bounds, resolution);
    int nsx = (bounds.sx / resolution).toInt();
    int nsy = (bounds.sy / resolution).toInt();
    int nex = (bounds.ex / resolution).toInt();
    int ney = (bounds.ey / resolution).toInt();
    
    for(int tx = nsx; tx < nex; tx++) {
      for(int ty = nsy; ty < ney; ty++) {
        int linid = linkid(tx, ty);
        for(int i = 0; i < links[linid].size(); i++) {
          CHECK(!isAvailable(paths[links[linid][i]].first));
        
          vector<vector<Coord2> > ntp = getDifference(paths[links[linid][i]].second, inters);
          if(ntp.size() == 1 && ntp[0] == paths[links[linid][i]].second)
            continue; // NO CHANGE!
          removePath(links[linid][i], tx, ty);
          for(int j = 0; j < ntp.size(); j++) {
            if(abs(getArea(ntp[j])) > 1 || getPerimeter(ntp[j]) > 2) {
              paths[addPath(tx, ty)].second = ntp[j];
            }
          }
        }
      }
    }
  }
}

Gamemap::Gamemap() { };
Gamemap::Gamemap(const Level &lev) {
  CHECK(lev.paths.size());
  
  Coord4 bounds = startCBoundBox();
  for(int i = 0; i < lev.paths.size(); i++) {
    addToBoundBox(&bounds, lev.paths[i]);
  }
  CHECK(bounds.isNormalized());
  
  bounds = bounds + Coord4(-offset, -offset, -offset, -offset);
  bounds = snapToEnclosingGrid(bounds, resolution);
  sx = (bounds.sx / resolution).toInt() - 1;
  sy = (bounds.sy / resolution).toInt() - 1;
  ex = (bounds.ex / resolution).toInt() + 1;
  ey = (bounds.ey / resolution).toInt() + 1;
  links.resize((ex - sx) * (ey - sy));

  for(int x = sx; x < ex; x++) {
    for(int y = sy; y < ey; y++) {
      Coord4 tile = getTileBounds(x, y);
      vector<Coord2> outerpath;
      outerpath.push_back(Coord2(tile.sx, tile.sy));
      outerpath.push_back(Coord2(tile.sx, tile.ey));
      outerpath.push_back(Coord2(tile.ex, tile.ey));
      outerpath.push_back(Coord2(tile.ex, tile.sy));
      for(int i = 0; i < lev.paths.size(); i++) {
        vector<vector<Coord2> > resu = getDifference(lev.paths[i], outerpath);
        for(int j = 0; j < resu.size(); j++) {
          Coord4 bounds = startCBoundBox();
          addToBoundBox(&bounds, resu[j]);
          //dprintf("%f,%f,%f,%f vs %f,%f,%f,%f\n", bounds.sx.toFloat(), bounds.sy.toFloat(), bounds.ex.toFloat(), bounds.ey.toFloat(), sxp.toFloat(), syp.toFloat(), exp.toFloat(), eyp.toFloat());
          CHECK(bounds.sx >= tile.sx - Coord(0.0001));
          CHECK(bounds.sy >= tile.sy - Coord(0.0001));
          CHECK(bounds.ex <= tile.ex + Coord(0.0001));
          CHECK(bounds.ey <= tile.ey + Coord(0.0001));
          paths[addPath(x, y)].second = resu[j];
        }
      }
    }
  }
}

Coord4 Gamemap::getInternalBounds() const {
  return Coord4(sx * resolution + offset, sy * resolution + offset, ex * resolution + offset, ey * resolution + offset);
}
Coord4 Gamemap::getTileBounds(int x, int y) const {
  return Coord4(x * resolution + offset, y * resolution + offset, (x + 1) * resolution + offset, (y + 1) * resolution + offset);
}

int Gamemap::linkid(int x, int y) const {
  return (y - sy) * (ex - sx) + x - sx;
}

void Gamemap::removePath(int id, int x, int y) {
  CHECK(!isAvailable(paths[id].first));
  int lid = linkid(x, y);
  CHECK(count(links[lid].begin(), links[lid].end(), id) == 1);
  links[lid].erase(find(links[lid].begin(), links[lid].end(), id));
  paths[id].first = GMS_ERASED;
  paths[id].second.clear();
  available.push_back(id);
}

int Gamemap::addPath(int x, int y) {
  int ite;
  if(available.size()) {
    ite = available.back();
    available.pop_back();
  } else {
    ite = paths.size();
    paths.push_back(make_pair<int, vector<Coord2> >(GMS_EMPTY, vector<Coord2>()));
  }
  CHECK(isAvailable(paths[ite].first));
  links[linkid(x, y)].push_back(ite);
  paths[ite].first = GMS_CHANGED;
  return ite;
}
