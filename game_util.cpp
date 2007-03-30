
#include "game_util.h"

#include "game_tank.h"
#include "rng.h"

using namespace std;

vector<pair<float, Tank *> > GameImpactContext::getAdjacency(const Coord2 &center) const {
  vector<pair<float, Tank *> > rv;
  for(int i = 0; i < players.size(); i++) {
    if(players[i]->isLive()) {
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

void GameImpactContext::record(const IDBWarheadAdjust &warhead, Coord2 pos, const Tank *impact_tank, const Tank *owner_tank) const {
  if(recorder) {
    int target = -1;
    if(impact_tank && impact_tank->team != owner_tank->team) {
      target = findTankId(impact_tank);
    }
    
    vector<pair<float, Tank *> > dists = getAdjacency(pos);
    vector<pair<float, int> > distadj;
    for(int i = 0; i < dists.size(); i++)
      if(dists[i].first <= warhead.base()->radiusfalloff * WARHEAD_RADIUS_MAXMULT && dists[i].second->team != owner_tank->team)
        distadj.push_back(make_pair(dists[i].first, findTankId(dists[i].second)));
    sort(distadj.begin(), distadj.end());
    
    if(target != -1 || distadj.size())
      recorder->warhead(warhead.base(), target, distadj);
  }
}

int GameImpactContext::findTankId(const Tank *tank) const {
  CHECK(count(players.begin(), players.end(), tank) == 1);
  return find(players.begin(), players.end(), tank) - players.begin();
}

bool DeployLocation::isTank() const {
  return tank_int;
}

const Tank &DeployLocation::tank() const {
  CHECK(tank_int);
  return *tank_int;
}

Coord2 DeployLocation::pos() const {
  if(tank_int)
    return tank_int->pos;
  else
    return pos_int;
}
float DeployLocation::d() const {
  if(tank_int)
    return tank_int->d;
  else
    return d_int;
}

DeployLocation::DeployLocation(const Tank *tank) {
  CHECK(tank);
  tank_int = tank;
}

DeployLocation::DeployLocation(Coord2 pos, float d) {
  tank_int = NULL;
  pos_int = pos;
  d_int = d;
}

void dealDamage(float dmg, Tank *target, Tank *owner, float damagecredit, bool killcredit) {
  if(target->team == owner->team)
    return; // friendly fire exception
  if(target->takeDamage(dmg) && killcredit)
    owner->addKill();
  owner->addDamage(dmg * damagecredit);
};

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, float normal, Coord2 vel, Tank *impact, const GamePlayerContext &gpc, float damagecredit, bool killcredit, bool impacted) {
  
  gpc.gic->record(warhead, pos, impact, gpc.owner);
  
  // this stuff is kind of copied into detonateWarheadDamageOnly
  if(impact)
    dealDamage(warhead.impactdamage(), impact, gpc.owner, damagecredit, killcredit);
  
  if(warhead.radiusfalloff() >= 0) {
    vector<pair<float, Tank *> > adjacency = gpc.gic->getAdjacency(pos);
    for(int i = 0; i < adjacency.size(); i++) {
      if(adjacency[i].first < warhead.radiusfalloff())
        dealDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - adjacency[i].first), adjacency[i].second, gpc.owner, damagecredit, killcredit);
    }
  }
  
  if(warhead.radiusfalloff() > 0)
    gpc.gic->effects->push_back(GfxBlast(pos.toFloat(), warhead.radiusfalloff(), warhead.radiuscolor_bright(), warhead.radiuscolor_dim()));
  
  if(warhead.wallremovalradius() > 0 && gpc.gic->rng->frand() < warhead.wallremovalchance())
    gpc.gic->gamemap->removeWalls(pos, warhead.wallremovalradius(), gpc.gic->rng);
  
  if(impacted) {
    for(int i = 0; i < warhead.effects_impact().size(); i++) {
      for(int j = 0; j < warhead.effects_impact()[i]->quantity; j++) {
        gpc.gic->effects->push_back(GfxIdb(pos.toFloat(), normal, vel.toFloat(), warhead.effects_impact()[i]));
      }
    }
  }
  
  vector<IDBDeployAdjust> dep = warhead.deploy();
  for(int i = 0; i < dep.size(); i++)
    deployProjectile(dep[i], DeployLocation(pos - normalize(vel) / 10, getAngle(vel.toFloat())), gpc);
  
}

void detonateWarheadDamageOnly(const IDBWarheadAdjust &warhead, Tank *impact, const vector<pair<float, Tank*> > &radius) {
  if(impact)
    impact->takeDamage(warhead.impactdamage());
  
  for(int i = 0; i < radius.size(); i++)
    if(radius[i].first < warhead.radiusfalloff())
      radius[i].second->takeDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - radius[i].first));
}

void deployProjectile(const IDBDeployAdjust &deploy, const DeployLocation &location, const GamePlayerContext &gpc, vector<float> *tang) {
  
  int type = deploy.type();
  if(type == DT_NORMAL) {
    if(location.isTank())
      type = DT_FORWARD;
    else
      type = DT_CENTROID;
  }
  
  vector<pair<Coord2, float> > proji;
  
  if(type == DT_FORWARD) {
    CHECK(location.isTank());
    CHECK(!tang);
    proji.push_back(make_pair(location.tank().getFiringPoint(), location.tank().d));
  } else if(type == DT_CENTROID) {
    CHECK(!tang);
    proji.push_back(make_pair(location.pos(), location.d()));
  } else if(type == DT_MINEPATH) {
    CHECK(location.isTank());
    CHECK(!tang);
    proji.push_back(make_pair(location.tank().getMinePoint(gpc.gic->rng), location.tank().d));
  } else if(type == DT_EXPLODE) {
    vector<float> ang;
    {
      int ct = int(gpc.gic->rng->frand() * (deploy.exp_maxsplits() - deploy.exp_minsplits() + 1)) + deploy.exp_minsplits();
      CHECK(ct <= deploy.exp_maxsplits() && ct >= deploy.exp_minsplits());
      for(int i = 0; i < ct; i++)
        ang.push_back(gpc.gic->rng->frand() * (deploy.exp_maxsplitsize() - deploy.exp_minsplitsize()) + deploy.exp_minsplitsize());
      for(int i = 1; i < ang.size(); i++)
        ang[i] += ang[i - 1];
      float angtot = ang.back();
      float shift = gpc.gic->rng->frand() * PI * 2;
      for(int i = 0; i < ang.size(); i++) {
        ang[i] *= PI * 2 / angtot;
        ang[i] += shift;
      }
    }
    if(tang) {
      *tang = ang;
    }
    for(int i = 0; i < ang.size(); i++)
      for(int j = 0; j < deploy.exp_shotspersplit(); j++)
        proji.push_back(make_pair(location.pos(), ang[i]));
  } else {
    CHECK(0);
  }
  
  CHECK(proji.size());
  for(int i = 0; i < proji.size(); i++)
    proji[i].second += deploy.anglestddev() * gpc.gic->rng->gaussian();
  
  {
    vector<IDBDeployAdjust> idd = deploy.chain_deploy();
    for(int i = 0; i < idd.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        deployProjectile(idd[i], DeployLocation(proji[j].first, proji[j].second), gpc);
  }
  
  {
    vector<IDBProjectileAdjust> idp = deploy.chain_projectile();
    for(int i = 0; i < idp.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        gpc.projpack->add(Projectile(proji[j].first, proji[j].second, idp[i], gpc.gic->rng));
  }
  
  {
    vector<IDBWarheadAdjust> idw = deploy.chain_warhead();
    for(int i = 0; i < idw.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        detonateWarhead(idw[i], proji[j].first, NO_NORMAL, Coord2(0, 0), NULL, gpc, 1.0, true, true);
  }
}
Team::Team() {
  weapons_enabled = true;
  color = Color(0, 0, 0);
  swap_colors = false;
}

GameImpactContext::GameImpactContext(vector<Tank> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap, Rng *rng, Recorder *recorder) : players(ptrize(players)), effects(effects), gamemap(gamemap), rng(rng), recorder(recorder) { };

GamePlayerContext::GamePlayerContext(Tank *owner, ProjectilePack *projpack, const GameImpactContext &gic) : projpack(projpack), owner(owner), gic(&gic) { };
