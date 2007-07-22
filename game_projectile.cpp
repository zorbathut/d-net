
#include "game_projectile.h"

#include "collide.h"
#include "gfx.h"
#include "rng.h"
#include "game_tank.h"

using namespace std;

void Projectile::tick(vector<smart_ptr<GfxEffects> > *gfxe, const GameImpactContext &gic, int owner) {
  CHECK(live);
  CHECK(age != -1);
  distance_traveled += len(movement()).toFloat();
  pos += movement();
  lasttail = nexttail();
  age++;
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > projtype.missile_stabstart() * FPS)
      missile_sidedist /= pow(projtype.missile_stabilization(), 1.f / FPS);
    for(int i = 0; i < 2; i++) { // generate particle effects
      float dir = unsync().frand() * 2 * PI;
      Float2 pv = makeAngle(dir) / 3;
      pv *= 1.0 - unsync().frand() * unsync().frand();
      pv += movement().toFloat();
      pv += missile_accel().toFloat() * -3 * abs(unsync().gaussian());
      pv *= 60; // this is an actual 60, converting it from my previous movement-per-frame to movement-per-second
      gfxe->push_back(GfxPoint(pos.toFloat() + lasttail.toFloat() - movement().toFloat(), pv, 0.17, Color(1.0, 0.9, 0.6)));
    }
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity *= 0.95;
    if(airbrake_liveness() <= 0)
      detonating = true;
  } else if(projtype.motion() == PM_MINE || projtype.motion() == PM_SPIDERMINE) {
    if(age >= projtype.halflife() * FPS / 2 && gic.rng->frand() > pow(0.5f, 1 / (projtype.halflife() * FPS)))
      detonating = true;
    if(projtype.motion() == PM_SPIDERMINE) {
      spider_vector = Coord(SV_NONE);
      int cid = gic.getClosestFoeId(pos, owner);
      if(cid != -1) {
        Coord2 dp = gic.players[cid]->pos;
        dp -= pos;
        if(len(dp) < Coord(projtype.mine_visibility()) && len(dp) > 0) {
          spider_vector = getAngle(dp);
        }
      }
    }
  } else if(projtype.motion() == PM_DPS) {
    detonating = true;
  } else {
    CHECK(0);
  }
  
  if(projtype.proximity()) {
    float newdist = gic.getClosestFoeDistance(pos, owner);
    if(newdist > closest_enemy_tank && newdist < projtype.chain_warhead()[0].radiusfalloff())
      detonating = true; // BOOM
    closest_enemy_tank = newdist;
  }
}

void Projectile::render(const vector<Coord2> &tankposes) const {
  CHECK(live);
  CHECK(age != -1);
  if(projtype.motion() == PM_NORMAL) {
    setColor(projtype.color());
  } else if(projtype.motion() == PM_MISSILE) {
    setColor(projtype.color());
  } else if(projtype.motion() == PM_AIRBRAKE) {
    setColor(projtype.color() * airbrake_liveness());
  } else if(projtype.motion() == PM_MINE || projtype.motion() == PM_SPIDERMINE) {
    const float radarrange = projtype.mine_visibility();
    float closest = 1000;
    for(int i = 0; i < tankposes.size(); i++)
      if(len(tankposes[i] - pos).toFloat() < closest)
        closest = len(tankposes[i] - pos).toFloat();
    if(closest < radarrange) {
      if(closest < radarrange / 2) {
        setColor(C::gray(1.0));
      } else {
        setColor(C::gray((radarrange - closest) / radarrange * 2));
      }
      drawLineLoop(mine_polys(), 0.1);
    }
    return;
  } else if(projtype.motion() == PM_DPS) {
  } else {
    CHECK(0);
  }
  drawLine(Coord4(pos, pos + lasttail), projtype.thickness_visual());
};

void Projectile::firstCollide(Collider *collider, int owner, int id) const {
  if(projtype.motion() == PM_MINE) {
    vector<Coord2> ite = mine_polys();
    for(int i = 0; i < ite.size(); i++)
      collider->addUnmovingToken(CollideId(CGR_STATPROJECTILE, owner, id), Coord4(ite[i], ite[(i + 1) % ite.size()]));
  }
}

void Projectile::addCollision(Collider *collider, int owner, int id) const {
  CHECK(live);
  if(projtype.motion() == PM_MINE || projtype.motion() == PM_DPS) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
    vector<Coord2> ite = mine_polys();
    for(int i = 0; i < ite.size(); i++)
      collider->addNormalToken(CollideId(CGR_NOINTPROJECTILE, owner, id), Coord4(ite[i], ite[(i + 1) % ite.size()]), Coord4(movement(), movement()));
  } else {
    if(projtype.no_intersection()) {
      collider->addPointToken(CollideId(CGR_NOINTPROJECTILE, owner, id), pos, movement());
    } else {
      collider->addNormalToken(CollideId(CGR_PROJECTILE, owner, id), Coord4(pos, pos + lasttail), Coord4(movement(), movement() + nexttail() - lasttail));
    }
  }
}

void Projectile::collideCleanup(Collider *collider, int owner, int id) const {
  if(projtype.motion() == PM_MINE) {
    collider->dumpGroup(CollideId(CGR_STATPROJECTILE, owner, id));
  }
}

Coord2 Projectile::warheadposition() const {
  return pos;
}

void Projectile::detonate(Coord2 pos, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted) {
  if(!live)
    return;
  
  if(projtype.motion() != PM_DPS) {
    vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
    for(int i = 0; i < idw.size(); i++)
      detonateWarhead(idw[i], pos, normal, movement() * FPS, target, gpc, damageflags, impacted);
  
    live = false;
  } else if(projtype.motion() == PM_DPS) {
    int alen = int(projtype.dps_duration() * FPS);
    if(age > alen) {
      live = false;
    } else {
      int shares = alen * (alen + 1) / 2;
      int tshares = alen - age + 1;
      vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
      for(int i = 0; i < idw.size(); i++)
        detonateWarhead(idw[i].multiply((float)tshares / shares), pos, 0, Coord2(0, 0), NULL, gpc, damageflags, false);
      detonating = false;
    }
  } else {
    CHECK(0);
  }
};

bool Projectile::isLive() const {
  return live;
}

bool Projectile::isDetonating() const {
  return detonating;
}

bool Projectile::isConsumed() const {
  return !isLive() || isDetonating();
}

float Projectile::durability() const {
  return projtype.durability();
}

Coord2 Projectile::movement() const {
  if(projtype.motion() == PM_NORMAL) {
    return makeAngle(Coord(d)) * Coord(projtype.velocity() / FPS);
  } else if(projtype.motion() == PM_MISSILE) {
    return missile_accel() + missile_backdrop() + missile_sidedrop();
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return makeAngle(d) * Coord(airbrake_velocity) / FPS;
  } else if(projtype.motion() == PM_MINE || projtype.motion() == PM_DPS) {
    return Coord2(0, 0);
  } else if(projtype.motion() == PM_SPIDERMINE) {
    if(spider_vector == Coord(SV_NONE))
      return Coord2(0, 0);
    else
      return makeAngle(spider_vector) * Coord(projtype.velocity() / FPS);
  } else {
    CHECK(0);
  }
}

Coord2 Projectile::nexttail() const {
  float maxlen = max(0., distance_traveled - 0.1);
  if(projtype.motion() == PM_NORMAL) {
    return makeAngle(d) * -Coord(min(projtype.length(), maxlen));
  } else if(projtype.motion() == PM_MISSILE) {
    return makeAngle(d) * -Coord(min(projtype.length(), maxlen));
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return makeAngle(d) * -Coord(min(airbrake_velocity / FPS + 2, maxlen));
  } else if(projtype.motion() == PM_MINE || projtype.motion() == PM_DPS || projtype.motion() == PM_SPIDERMINE) {
    return Coord2(0, 0);
  } else {
    CHECK(0);
  }
}

Coord2 Projectile::missile_accel() const {
  return makeAngle(Coord(d)) * Coord(projtype.velocity() / FPS) * age / FPS;
}
Coord2 Projectile::missile_backdrop() const {
  return -makeAngle(Coord(d)) * Coord(projtype.missile_backlaunch()) / FPS;
}
Coord2 Projectile::missile_sidedrop() const {
  return makeAngle(Coord(d) - COORDPI / 2) * Coord(missile_sidedist);
}

float Projectile::airbrake_liveness() const {
  return 1.0 - (age / float(FPS) / projtype.airbrake_life());
}

vector<Coord2> Projectile::mine_polys() const {
  vector<Coord2> rv;
  const int rad = projtype.mine_spikes() * 2;
  for(int i = 0; i < rad; i++) {
    float expfact = (i % 2);
    expfact *= 3;
    expfact += 1;
    expfact /= 4;
    rv.push_back(Coord2(makeAngle(i * 2 * PI / rad + mine_facing) * projtype.radius_physical() * expfact) + pos);
  }
  return rv;
}

Projectile::Projectile() : projtype(NULL, IDBAdjustment()), damageflags(0.0, false, false) {
  live = false;
  age = -1;
}
Projectile::Projectile(const Coord2 &in_pos, Coord in_d, const IDBProjectileAdjust &projtype, Rng *rng, const DamageFlags &damageflags) : projtype(projtype), damageflags(damageflags) {
  pos = in_pos;
  d = in_d;
  age = 0;
  live = true;
  detonating = false;
  lasttail = Coord2(0, 0);
  distance_traveled = 0;
  closest_enemy_tank = 1e20; // no
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = rng->gaussian() * projtype.missile_sidelaunch() / FPS;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity = (rng->gaussian_scaled(2) / 4 + 1) * projtype.velocity();
  } else if(projtype.motion() == PM_MINE || projtype.motion() == PM_SPIDERMINE) {
    mine_facing = rng->frand() * 2 * PI;
  } else if(projtype.motion() == PM_DPS) {
  } else {
    CHECK(0);
  }
}

Projectile &ProjectilePack::find(int id) {
  CHECK(projectiles.count(id));
  return projectiles[id];
}
void ProjectilePack::add(const Projectile &proj) {
  int did = -1;
  if(aid.size()) {
    did = aid.back();
    aid.pop_back();
  } else {
    did = projectiles.size();
  }
  CHECK(!projectiles.count(did));
  projectiles[did] = proj;
  newitems.push_back(did);
}

void ProjectilePack::updateCollisions(Collider *collider, int owner) {
  for(int i = 0; i < newitems.size(); i++)
    projectiles[newitems[i]].firstCollide(collider, owner, newitems[i]);
  newitems.clear();
  
  for(map<int, Projectile>::const_iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr) {
    itr->second.addCollision(collider, owner, itr->first);
  }
}

void ProjectilePack::tick(vector<smart_ptr<GfxEffects> > *gfxe, Collider *collide, int owner, const GameImpactContext &gic) {
  for(map<int, Projectile>::iterator itr = projectiles.begin(); itr != projectiles.end(); ) {
    // the logic here is kind of grim, sorry about this
    if(itr->second.isLive() && itr->second.isDetonating())
      itr->second.detonate(itr->second.warheadposition(), NO_NORMAL, NULL, GamePlayerContext(gic.players[owner], this, gic), false);
    if(itr->second.isLive()) {
      if(!count(newitems.begin(), newitems.end(), itr->first))  // we make sure we do collisions before ticks
        itr->second.tick(gfxe, gic, owner);
      if(itr->second.isLive() && itr->second.isDetonating())
        itr->second.detonate(itr->second.warheadposition(), NO_NORMAL, NULL, GamePlayerContext(gic.players[owner], this, gic), false);
      if(itr->second.isLive()) {
        ++itr;
        continue;
      }
    }
    
    CHECK(!itr->second.isLive());
    itr->second.collideCleanup(collide, owner, itr->first);
    aid.push_back(itr->first);
    map<int, Projectile>::iterator titr = itr;
    ++itr;
    projectiles.erase(titr);
  }
}

void ProjectilePack::render(const vector<Coord2> &tankpos) const {
  for(map<int, Projectile>::const_iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr)
    itr->second.render(tankpos);
}
