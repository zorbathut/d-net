
#include "game_util.h"

#include "game_tank.h"
#include "rng.h"
#include "adler32.h"

using namespace std;

vector<pair<float, Tank *> > GameImpactContext::getAdjacency(const Coord2 &center) const {
  vector<pair<float, Tank *> > rv;
  for(int i = 0; i < players.size(); i++) {
    if(players[i]->isLive()) {
      vector<Coord2> tv = players[i]->getTankVertices(players[i]->pi);
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

int GameImpactContext::getClosestFoeId(const Coord2 &pos, int owner) const {
  vector<pair<float, Tank *> > taj = getAdjacency(pos);
  float closest = 1e20; // no
  int id = -1; // denied
  for(int i = 0; i < taj.size(); i++) {
    if(taj[i].first < closest && taj[i].second->team != players[owner]->team) {
      closest = taj[i].first;
      id = i;
    }
  }
  if(id == -1)
    return -1;
  else
    return findTankId(taj[id].second);
}

float GameImpactContext::getClosestFoeDistance(const Coord2 &pos, int owner) const {
  vector<pair<float, Tank *> > taj = getAdjacency(pos);
  float closest = 1e20; // no
  for(int i = 0; i < taj.size(); i++)
    if(taj[i].first < closest && taj[i].second->team != players[owner]->team)
      closest = taj[i].first;
  return closest;
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
      recorder->warhead(warhead.base(), warhead.multfactor().toFloat(), target, distadj);
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

Coord DeployLocation::vengeance() const {
  CHECK(tank_int);
  return getAngle(tank_int->get_vengeance_location() - tank_int->pi.pos);
}

Coord2 DeployLocation::pos() const {
  if(tank_int)
    return tank_int->pi.pos;
  else
    return pos_int;
}
Coord DeployLocation::d() const {
  if(tank_int)
    return tank_int->pi.d;
  else
    return d_int;
}
Coord DeployLocation::impacted_ang() const {
  CHECK(!tank_int);
  CHECK(has_impacted_ang);
  return impacted_ang_int;
}

DeployLocation::DeployLocation(const Tank *tank) {
  CHECK(tank);
  tank_int = tank;
  has_impacted_ang = false;
}

DeployLocation::DeployLocation(Coord2 pos, Coord d) {
  tank_int = NULL;
  pos_int = pos;
  d_int = d;
  has_impacted_ang = false;
}

DeployLocation::DeployLocation(Coord2 pos, Coord d, Coord impacted_ang) {
  tank_int = NULL;
  pos_int = pos;
  d_int = d;
  if(impacted_ang != NO_NORMAL) {
    has_impacted_ang = true;
    impacted_ang_int = impacted_ang;
  } else {
    has_impacted_ang = false;
  }
}

void dealDamage(Coord dmg, Tank *target, Tank *owner, const DamageFlags &flags) {
  CHECK(target);
  CHECK(owner);
  if(target->team == owner->team)
    return; // friendly fire exception
  CHECK(target != owner); // something has gone horribly wrong if this is the case
  //dprintf("Dealing %f damage\n", dmg);
  if(flags.glory)
    dmg *= 1 - target->getGloryResistance();
  if(target->takeDamage(dmg, owner->pi.pos) && flags.killcredit)
    owner->addKill();
  owner->addDamage(dmg * flags.damagecredit);
};

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Coord normal, Coord2 vel, Tank *impact, const GamePlayerContext &gpc, const DamageFlags &flags, bool impacted) {
  
  gpc.gic->record(warhead, pos, impact, gpc.owner);
  
  // this stuff is kind of copied into detonateWarheadDamageOnly
  if(impact)
    dealDamage(warhead.impactdamage(), impact, gpc.owner, flags);
  
  if(warhead.radiusfalloff() >= 0) {
    vector<pair<float, Tank *> > adjacency = gpc.gic->getAdjacency(pos);
    for(int i = 0; i < adjacency.size(); i++) {
      if(adjacency[i].first < warhead.radiusfalloff())
        dealDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - adjacency[i].first), adjacency[i].second, gpc.owner, flags);
    }
  }
  
  if(warhead.radiusfalloff() > 0 && (warhead.radiuscolor_bright() != C::black || warhead.radiuscolor_dim() != C::black))
    gpc.gic->effects->push_back(GfxBlast(pos.toFloat(), warhead.radiusfalloff().toFloat(), warhead.radiuscolor_bright(), warhead.radiuscolor_dim()));
  
  if(warhead.wallremovalradius() > 0 && gpc.gic->rng->frand() < warhead.wallremovalchance())
    gpc.gic->gamemap->removeWalls(pos, warhead.wallremovalradius(), gpc.gic->rng);
  
  if(impacted) {
    for(int i = 0; i < warhead.effects_impact().size(); i++) {
      for(int j = 0; j < warhead.effects_impact()[i].quantity(); j++) {
        gpc.gic->effects->push_back(GfxIdb(pos.toFloat(), normal.toFloat(), vel.toFloat(), warhead.effects_impact()[i]));
      }
    }
  }
  
  {
    Coord2 shifted_pos = pos;
    if(len(vel) > Coord(0))
      shifted_pos -= normalize(vel) / 10;
    
    vector<IDBDeployAdjust> dep = warhead.deploy();
    for(int i = 0; i < dep.size(); i++)
      deployProjectile(dep[i], DeployLocation(shifted_pos, getAngle(vel)), gpc, flags);
  }
  
}

void detonateWarheadDamageOnly(const IDBWarheadAdjust &warhead, Tank *impact, const vector<pair<float, Tank*> > &radius) {
  if(impact)
    impact->takeDamage(warhead.impactdamage(), Coord2(0, 0));
  
  for(int i = 0; i < radius.size(); i++)
    if(radius[i].first < warhead.radiusfalloff())
      radius[i].second->takeDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - radius[i].first), Coord2(0, 0));
}

void detonateBombardment(const IDBBombardmentAdjust &bombard, Coord2 pos, Coord direction, const GamePlayerContext &gpc) {
  for(int i = 0; i < bombard.warheads().size(); i++)
    detonateWarhead(bombard.warheads()[i], pos, direction, Coord2(0, 0), NULL, gpc, DamageFlags(1.0, false, true), true);
  
  for(int i = 0; i < bombard.projectiles().size(); i++)
    gpc.projpack->add(Projectile(pos, direction, bombard.projectiles()[i], gpc.gic->rng, DamageFlags(1.0, false, true)));
  
  for(int i = 0; i < bombard.effects().size(); i++) {
    for(int j = 0; j < bombard.effects()[i].quantity(); j++) {
      gpc.gic->effects->push_back(GfxIdb(pos.toFloat(), 0, makeAngle(direction.toFloat()), bombard.effects()[i]));
    }
  }
}

void deployProjectile(const IDBDeployAdjust &deploy, const DeployLocation &location, const GamePlayerContext &gpc, const DamageFlags &flags, vector<float> *tang) {
  
  int type = deploy.type();
  if(type == DT_NORMAL) {
    if(location.isTank())
      type = DT_FORWARD;
    else
      type = DT_CENTROID;
  }
  
  vector<pair<Coord2, Coord> > proji;
  
  for(int m = 0; m < deploy.multiple(); m++) {
    if(type == DT_FORWARD) {
      CHECK(location.isTank());
      CHECK(!tang);
      proji.push_back(make_pair(location.tank().getFiringPoint(), location.tank().pi.d));
    } else if(type == DT_REAR) {
      CHECK(location.isTank());
      CHECK(!tang);
      proji.push_back(make_pair(location.tank().getRearFiringPoint(), location.tank().pi.d + COORDPI));
    } else if(type == DT_CENTROID) {
      CHECK(!tang);
      proji.push_back(make_pair(location.pos(), location.d()));
    } else if(type == DT_MINEPATH) {
      CHECK(location.isTank());
      CHECK(!tang);
      proji.push_back(make_pair(location.tank().getMinePoint(gpc.gic->rng), location.tank().pi.d));
    } else if(type == DT_DIRECTED) {
      CHECK(!tang);
      int cid = gpc.gic->getClosestFoeId(location.pos(), gpc.owner_id());
      if(cid != -1) {
        CHECK(cid >= 0 && cid < gpc.gic->players.size());
        Coord2 dp = gpc.gic->players[cid]->pi.pos;
        dp -= location.pos();
        if(len(dp) <= Coord(deploy.directed_range()))
          proji.push_back(make_pair(location.pos() + normalize(dp) * deploy.directed_approach(), getAngle(dp)));
      }
      if(!proji.size())
        proji.push_back(make_pair(location.pos(), gpc.gic->rng->frand() * PI * 2));
    } else if(type == DT_REFLECTED) {
      CHECK(!tang);
      CHECK(!location.isTank());
      proji.push_back(make_pair(location.pos() + makeAngle(reflect(location.d(), location.impacted_ang())) * 0.01, reflect(location.d(), location.impacted_ang())));
    } else if(type == DT_ARC) {
      CHECK(!tang);
      CHECK(deploy.anglestddev() == 0);
      for(int i = 0; i < deploy.arc_units(); i++)
        proji.push_back(make_pair(location.pos(), location.d() + (Coord)deploy.arc_width() * (i * 2 + 1 - deploy.arc_units()) / deploy.arc_units() / 2));
    } else if(type == DT_VENGEANCE) {
      CHECK(!tang);
      proji.push_back(make_pair(location.pos(), location.vengeance()));
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
          proji.push_back(make_pair(location.pos(), Coord(ang[i])));
    } else {
      CHECK(0);
    }
  }
  
  CHECK(proji.size());
  for(int i = 0; i < proji.size(); i++)
    proji[i].second += Coord(deploy.anglestddev() * gpc.gic->rng->gaussian());
  
  for(int i = 0; i < proji.size(); i++)
    proji[i].second += deploy.anglemodifier();
  
  {
    vector<IDBDeployAdjust> idd = deploy.chain_deploy();
    for(int i = 0; i < idd.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        deployProjectile(idd[i], DeployLocation(proji[j].first, proji[j].second), gpc, flags);
  }
  
  {
    vector<IDBProjectileAdjust> idp = deploy.chain_projectile();
    for(int i = 0; i < idp.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        gpc.projpack->add(Projectile(proji[j].first, proji[j].second, idp[i], gpc.gic->rng, flags));
  }
  
  {
    vector<IDBWarheadAdjust> idw = deploy.chain_warhead();
    for(int i = 0; i < idw.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        detonateWarhead(idw[i], proji[j].first, NO_NORMAL, Coord2(0, 0), NULL, gpc, flags, true);
  }
  
  {
    vector<IDBEffectsAdjust> idw = deploy.chain_effects();
    for(int i = 0; i < idw.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        gpc.gic->effects->push_back(GfxIdb(proji[j].first.toFloat(), 0, makeAngle(proji[j].second.toFloat()), idw[i]));
  }
  
  {
    vector<IDBInstantAdjust> idw = deploy.chain_instant();
    for(int i = 0; i < idw.size(); i++)
      for(int j = 0; j < proji.size(); j++)
        triggerInstant(idw[i], proji[j].first, gpc, flags);
  }
}

void triggerInstant(const IDBInstantAdjust &instant, Coord2 pos, const GamePlayerContext &gpc, const DamageFlags &flags) {
  if(instant.type() == IT_TESLA) {
    Coord2 detpos;
    Tank *dettar = NULL;
    
    vector<pair<float, Tank *> > tt = gpc.gic->getAdjacency(pos);
    vector<Tank *> opts;
    for(int i = 0; i < tt.size(); i++)
      if(tt[i].first <= instant.tesla_radius() && tt[i].second->team != gpc.owner->team)
        opts.push_back(tt[i].second);
    if(gpc.gic->rng->frand() * (opts.size() + instant.tesla_unlockshares()) >= opts.size()) {
      detpos = pos + makeAngle(gpc.gic->rng->frand() * COORDPI * 2) * (gpc.gic->rng->frand() / 2 + 0.5) * instant.tesla_radius();
    } else {
      int rnt = gpc.gic->rng->choose(opts.size());
      
      vector<Coord4> v = opts[rnt]->getCurrentCollide();
      
      detpos = lerp(v[gpc.gic->rng->choose(v.size())], gpc.gic->rng->frand());
      dettar = opts[rnt];
    }
    
    for(int i = 0; i < 3; i++)
      gpc.gic->effects->push_back(GfxLightning(pos.toFloat(), detpos.toFloat()));
    
    vector<IDBWarheadAdjust> idw = instant.tesla_warhead();
    for(int i = 0; i < idw.size(); i++)
      detonateWarhead(idw[i], detpos, NO_NORMAL, Coord2(0, 0), dettar, gpc, flags, true);
  } else {
    CHECK(0);
  }
}

Team::Team() {
  weapons_enabled = true;
  color = Color(0, 0, 0);
  swap_colors = false;
}

GameImpactContext::GameImpactContext(vector<Tank> *players, vector<smart_ptr<GfxEffects> > *effects, Gamemap *gamemap, Rng *rng, Recorder *recorder) : players(ptrize(players)), effects(effects), gamemap(gamemap), rng(rng), recorder(recorder) { };

int GamePlayerContext::owner_id() const {
  vector<Tank *>::const_iterator itr = find(gic->players.begin(), gic->players.end(), owner);
  CHECK(itr != gic->players.end());
  return itr - gic->players.begin();
}

GamePlayerContext::GamePlayerContext(Tank *owner, ProjectilePack *projpack, const GameImpactContext &gic) : projpack(projpack), owner(owner), gic(&gic) { };

void adler(Adler32 *adl, const Team &team) {
  adler(adl, team.weapons_enabled);
  adler(adl, team.swap_colors);
}

void adler(Adler32 *adl, const DamageFlags &df) {
  adler(adl, df.damagecredit);
  adler(adl, df.killcredit);
  adler(adl, df.glory);
}
