
#include "gamemap.h"

#include "args.h"
#include "collide.h"
#include "gfx.h"
#include "rng.h"
#include "adler32_util.h"

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
    if(isAvailable(paths[i].state))
      continue;
    
    for(int j = 0; j < paths[i].renderpath.size(); j++)
      drawLinePath(paths[i].renderpath[j], 0.75);
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
  
  {
    GfxInvertingStencil gfxis;
    for(int i = 0; i < paths.size(); i++) {
      invertStencilLoop(paths[i].collisionpath);
    }
    
    const Coord boundenex = max(getCollisionBounds().span_x(), getCollisionBounds().span_y());
  
    // Four giant rectangles. Two vertical that include corners, two horizontal that don't.
    {
      vector<Coord2> left;
      left.push_back(Coord2(getCollisionBounds().sx - boundenex, getCollisionBounds().sy - boundenex)); // far top-left
      left.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().sy - boundenex)); // near top-left
      left.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().ey + boundenex)); // near bottom-left
      left.push_back(Coord2(getCollisionBounds().sx - boundenex, getCollisionBounds().ey + boundenex)); // far bottom-left
      invertStencilLoop(left);
    }
    
    {
      vector<Coord2> right;
      right.push_back(Coord2(getCollisionBounds().ex + boundenex, getCollisionBounds().sy - boundenex)); // far top-right
      right.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().sy - boundenex)); // near top-right
      right.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().ey + boundenex)); // near bottom-right
      right.push_back(Coord2(getCollisionBounds().ex + boundenex, getCollisionBounds().ey + boundenex)); // far bottom-right
      invertStencilLoop(right);
    }
    
    {
      vector<Coord2> top;
      top.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().sy - boundenex)); // far top-left
      top.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().sy)); // near top-left
      top.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().sy)); // near top-right
      top.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().sy - boundenex)); // far top-left
      invertStencilLoop(top);
    }
    
    {
      vector<Coord2> bottom;
      bottom.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().ey + boundenex)); // far bottom-left
      bottom.push_back(Coord2(getCollisionBounds().sx, getCollisionBounds().ey)); // near bottom-left
      bottom.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().ey)); // near bottom-right
      bottom.push_back(Coord2(getCollisionBounds().ex, getCollisionBounds().ey + boundenex)); // far bottom-left
      invertStencilLoop(bottom);
    }
  }
  
  {
    GfxStenciled gfxs;
    setColor(Color(0.3, 0.3, 0.3));
    drawGrid(10.0, 0.1);
  }
}


void Gamemap::updateCollide(Collider *collider) {
  for(int i = 0; i < paths.size(); i++) {
    if(paths[i].state == GMS_EMPTY) {
    } else if(paths[i].state == GMS_ERASED) {
      collider->dumpGroup(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      paths[i].state = GMS_EMPTY;
    } else if(paths[i].state == GMS_UNCHANGED) {
    } else if(paths[i].state == GMS_CHANGED) {
      collider->dumpGroup(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      for(int j = 0; j < paths[i].collisionpath.size(); j++) {
        int k = (j + 1) % paths[i].collisionpath.size();
        collider->addUnmovingToken(CollideId(CGR_WALL, CGR_WALLOWNER, i), Coord4(paths[i].collisionpath[j], paths[i].collisionpath[k]));
      }
      // implicit in CGR_WALL now, but leaving this in just for the time being
      //collider->markPersistent(CollideId(CGR_WALL, CGR_WALLOWNER, i));
      paths[i].state = GMS_UNCHANGED;
    }
  }
}

void Gamemap::checksum(Adler32 *adl) const {
  adler(adl, smashable);
  adler(adl, sx);
  adler(adl, sy);
  adler(adl, ex);
  adler(adl, ey);
  adler(adl, render_bounds);
  adler(adl, links);
  adler(adl, nlinks);
  adler(adl, paths);
}

Coord4 Gamemap::getRenderBounds() const {
  return render_bounds;
}
Coord4 Gamemap::getCollisionBounds() const {
  return getInternalBounds();
}

const Coord resolution = Coord(20);
const Coord2 offset = Coord2(1.23456f, 0.12345f); // Nasty hack to largely eliminate many border cases

void Gamemap::removeWalls(Coord2 center, Coord radius, Rng *rng) {
  if(!smashable)
    return;
  
  CHECK(nlinks.size() == 0);
  {
    Coord4 bounds = startCBoundBox();
    Coord4 ib = getInternalBounds();
    addToBoundBox(&bounds, ib);
    bounds = bounds + Coord4(-offset, -offset);
    bounds = snapToEnclosingGrid(bounds, resolution);
    int nsx = (bounds.sx / resolution).toInt();
    int nsy = (bounds.sy / resolution).toInt();
    int nex = (bounds.ex / resolution).toInt();
    int ney = (bounds.ey / resolution).toInt();
    CHECK(nsx == sx && nsy == sy && nex == ex && ney == ey);
  }
  {
    Coord4 ib = getInternalBounds();
    if(center.x + radius + 1 > ib.ex || center.x - radius - 1 < ib.sx || center.y + radius + 1 > ib.ey || center.y - radius - 1 < ib.sy) {
      dprintf("probin' at %f,%f %f", center.x.toFloat(), center.y.toFloat(), radius.toFloat());
      Coord4 bounds = startCBoundBox();
    
      addToBoundBox(&bounds, ib);
      addToBoundBox(&bounds, center + Coord2(radius + 1, radius + 1));
      addToBoundBox(&bounds, center - Coord2(radius + 1, radius + 1));
      dprintf("%f, %f, %f, %f\n", bounds.sx.toFloat(), bounds.sy.toFloat(), bounds.ex.toFloat(), bounds.ey.toFloat());
      dprintf("%f, %f, %f, %f\n", ib.sx.toFloat(), ib.sy.toFloat(), ib.ex.toFloat(), ib.ey.toFloat());
    
      bounds = bounds + Coord4(-offset, -offset);
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
            int pid = addPath(x, y);
            paths[pid].collisionpath = innerpath;
            paths[pid].generateRenderPath();
          }
        }
      }
      flushAdds();
    }
  }

  vector<Coord2> inters;
  {
    vector<Coord> rv;
    int vct = floor(rng->cfrand() * 3).toInt() + 3;
    Coord ofs = rng->cfrand() * 2 * COORDPI / vct;
    Coord maxofs = 2 * COORDPI / vct / 2;
    for(int i = 0; i < vct; i++) {
      rv.push_back(i * 2 * COORDPI / vct + ofs + rng->cgaussian_scaled(2) * maxofs);
    }
    for(int i = 0; i < rv.size(); i++)
      inters.push_back(center + makeAngle(rv[i]) * radius);
  }
  CHECK(!pathReversed(inters));
  if(set<Coord2>(inters.begin(), inters.end()).size() != inters.size() || !inPath(center, inters)) {
    // We've gotten two duplicate points, start over from scratch!
    // Alternatively, the center isn't included in the explosion.
    removeWalls(center, radius, rng);
    return;
  }
  
  addToBoundBox(&render_bounds, inters);
  
  {
    Coord4 bounds = startCBoundBox();
    addToBoundBox(&bounds, inters);
    bounds = bounds + Coord4(-offset, -offset);
    bounds = snapToEnclosingGrid(bounds, resolution);
    int nsx = (bounds.sx / resolution).toInt();
    int nsy = (bounds.sy / resolution).toInt();
    int nex = (bounds.ex / resolution).toInt();
    int ney = (bounds.ey / resolution).toInt();
    
    for(int tx = nsx; tx < nex; tx++) {
      for(int ty = nsy; ty < ney; ty++) {
        int linid = linkid(tx, ty);
        for(int i = 0; i < links[linid].size(); i++) {
          CHECK(!isAvailable(paths[links[linid][i]].state));
        
          vector<vector<Coord2> > ntp = getDifference(paths[links[linid][i]].collisionpath, inters);
          if(ntp.size() == 1 && ntp[0] == paths[links[linid][i]].collisionpath)
            continue; // NO CHANGE!
          removePath(links[linid][i], tx, ty);
          i--;
          for(int j = 0; j < ntp.size(); j++) {
            if(abs(getArea(ntp[j])) > 1 || getPerimeter(ntp[j]) > 2) {
              int pid = addPath(tx, ty);
              paths[pid].collisionpath = ntp[j];
              paths[pid].generateRenderPath();
            }
          }
        }
      }
    }
    flushAdds();
  }
}

bool Gamemap::isInsideWall(Coord2 point) const {
  Coord2 npoint = point - offset;
  int tx = floor(npoint.x / resolution).toInt();
  int ty = floor(npoint.y / resolution).toInt();
  int linid = linkid(tx, ty);
  for(int i = 0; i < links[linid].size(); i++) {
    CHECK(!isAvailable(paths[links[linid][i]].state));
    
    if(inPath(point, paths[links[linid][i]].collisionpath))
      return true;
  }
  return false;
}

Gamemap::Gamemap() {
  // these just exist for checksum
  smashable = 0;
  sx = 6293;
  sy = 88883;
  ex = -123987;
  ey = -9438;
  render_bounds = Coord4(1, 2, 3, 4);
};
Gamemap::Gamemap(const vector<vector<Coord2> > &lev, bool smashable) : smashable(smashable) {
  CHECK(lev.size());
  
  Coord4 bounds = startCBoundBox();
  for(int i = 0; i < lev.size(); i++) {
    addToBoundBox(&bounds, lev[i]);
  }
  CHECK(bounds.isNormalized());
  
  render_bounds = bounds;
  
  bounds = bounds + Coord4(-offset, -offset);
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
      for(int i = 0; i < lev.size(); i++) {
        vector<vector<Coord2> > resu = getDifference(lev[i], outerpath);
        for(int j = 0; j < resu.size(); j++) {
          Coord4 bounds = startCBoundBox();
          addToBoundBox(&bounds, resu[j]);
          //dprintf("%f,%f,%f,%f vs %f,%f,%f,%f\n", bounds.sx.toFloat(), bounds.sy.toFloat(), bounds.ex.toFloat(), bounds.ey.toFloat(), sxp.toFloat(), syp.toFloat(), exp.toFloat(), eyp.toFloat());
          CHECK(bounds.sx >= tile.sx - Coord(0.0001));
          CHECK(bounds.sy >= tile.sy - Coord(0.0001));
          CHECK(bounds.ex <= tile.ex + Coord(0.0001));
          CHECK(bounds.ey <= tile.ey + Coord(0.0001));
          int pid = addPath(x, y);
          paths[pid].collisionpath = resu[j];
          paths[pid].generateRenderPath();
        }
      }
    }
  }
  flushAdds();
}

Coord4 Gamemap::getInternalBounds() const {
  return Coord4(sx * resolution, sy * resolution, ex * resolution, ey * resolution) + Coord4(offset, offset);
}
Coord4 Gamemap::getTileBounds(int x, int y) const {
  return Coord4(x * resolution, y * resolution, (x + 1) * resolution, (y + 1) * resolution) + Coord4(offset, offset);
}

int Gamemap::linkid(int x, int y) const {
  return (y - sy) * (ex - sx) + x - sx;
}

void Gamemap::removePath(int id, int x, int y) {
  CHECK(!isAvailable(paths[id].state));
  int lid = linkid(x, y);
  CHECK(count(links[lid].begin(), links[lid].end(), id) == 1);
  links[lid].erase(find(links[lid].begin(), links[lid].end(), id));
  paths[id].state = GMS_ERASED;
  paths[id].collisionpath.clear();
  paths[id].renderpath.clear();
  available.push_back(id);
}

int Gamemap::addPath(int x, int y) {
  int ite;
  if(available.size()) {
    ite = available.back();
    available.pop_back();
  } else {
    ite = paths.size();
    paths.resize(paths.size() + 1);
    paths[ite].state = GMS_EMPTY;
  }
  CHECK(isAvailable(paths[ite].state));
  nlinks.push_back(make_pair(linkid(x, y), ite));
  paths[ite].state = GMS_CHANGED;
  return ite;
}

void Gamemap::flushAdds() {
  for(int i = 0; i < nlinks.size(); i++)
    links[nlinks[i].first].push_back(nlinks[i].second);
  nlinks.clear();
}

Coord diffFromBase(const Coord &x) {
  Coord d = x / resolution;
  return abs(d - floor(d + Coord(0.5)));
}

bool onboundary(const Coord2 &x, const Coord2 &y) {
  if(abs(x.x - y.x) < Coord(0.0001) && diffFromBase(x.x - offset.x) < Coord(0.0001) && abs(x.y - y.y) > abs(x.x - y.x) * 10)
    return true;
  if(abs(x.y - y.y) < Coord(0.0001) && diffFromBase(x.y - offset.y) < Coord(0.0001) && abs(x.x - y.x) > abs(x.y - y.y) * 10)
    return true;
  return false;
}

void Gamemap::Pathchunk::generateRenderPath() {
  renderpath.clear();
  {
    vector<Coord2> cdd;
    for(int i = 0; i < collisionpath.size(); i++) {
      int j = (i + 1) % collisionpath.size();
      if(!onboundary(collisionpath[i], collisionpath[j])) {
        if(!cdd.size())
          cdd.push_back(collisionpath[i]);
        cdd.push_back(collisionpath[j]);
      } else {
        if(cdd.size()) {
          CHECK(cdd.size() >= 2);
          renderpath.push_back(cdd);
          cdd.clear();
        }
      }
    }
    
    if(cdd.size()) {
      CHECK(cdd.size() >= 2);
      renderpath.push_back(cdd);
      cdd.clear();
    }
  }
}

void adler(Adler32 *adl, const Gamemap::Pathchunk &pc) {
  adler(adl, pc.state);
  adler(adl, pc.collisionpath);
  adler(adl, pc.renderpath);
}
