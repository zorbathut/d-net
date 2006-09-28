
#include "game_util.h"

#include "game_tank.h"
#include "rng.h"

using namespace std;

vector<pair<float, Tank *> > GameImpactContext::getAdjacency(const Coord2 &center) const {
  vector<pair<float, Tank *> > rv;
  for(int i = 0; i < players.size(); i++) {
    if(players[i]->live) {
      vector<Coord2> tv = players[i]->getTankVertices(players[i]->pos, players[i]->d);
      if(inPath(center, tv)) {
        rv.push_back(make_pair(0, players[i]));
        continue;
      }
      float closest = 1e10;
      for(int j = 0; j < tv.size(); j++) {
        float tdist = distanceFromLine(Coord4(tv[j], tv[(j + 1) % tv.size()]), center).toFloat();
        if(tdist < closest)
          closest = tdist;
      }
      CHECK(closest < 1e10);
      CHECK(closest >= 0);
      rv.push_back(make_pair(closest, players[i]));
    }
  }
  return rv;
}

void dealDamage(float dmg, Tank *target, Tank *owner, float damagecredit, bool killcredit) {
  if(target->team == owner->team)
    return; // friendly fire exception
  if(target->takeDamage(dmg) && killcredit)
    owner->addKill();
  owner->addDamage(dmg * damagecredit);
};

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Tank *impact, Tank *owner, const GameImpactContext &gic, float damagecredit, bool killcredit) {
  
  if(impact)
    dealDamage(warhead.impactdamage(), impact, owner, damagecredit, killcredit);
  
  if(warhead.radiusfalloff() >= 0) {
    vector<pair<float, Tank *> > adjacency = gic.getAdjacency(pos);
    for(int i = 0; i < adjacency.size(); i++) {
      if(adjacency[i].first < warhead.radiusfalloff())
        dealDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - adjacency[i].first), adjacency[i].second, owner, damagecredit, killcredit);
    }
  }
  
  for(int i = 0; i < 6; i++)
    gic.effects->push_back(GfxPoint(pos.toFloat(),  (makeAngle(frand() * 2 * PI) * 20) * (1.0 - frand() * frand()), 0.1, Color(1.0, 1.0, 1.0)));
  
  if(warhead.radiusfalloff() > 0)
    gic.effects->push_back(GfxBlast(pos.toFloat(), warhead.radiusfalloff(), warhead.radiuscolor_bright(), warhead.radiuscolor_dim()));
  
  if(warhead.wallremovalradius() > 0 && frand() < warhead.wallremovalchance()) {
    gic.gamemap->removeWalls(pos, warhead.wallremovalradius());
  }

};

Team::Team() {
  weapons_enabled = true;
  color = Color(0, 0, 0);
  swap_colors = false;
}

static vector<Tank*> ptrize(vector<Tank> *players) {
  vector<Tank*> ptrs;
  for(int i = 0; i < players->size(); i++)
    ptrs.push_back(&(*players)[i]);
  return ptrs;
}

GameImpactContext::GameImpactContext(vector<Tank> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap) : players(ptrize(players)), effects(effects), gamemap(gamemap) { };
