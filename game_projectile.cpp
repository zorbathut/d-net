
#include "game_projectile.h"

#include "collide.h"
#include "gfx.h"
#include "rng.h"

using namespace std;

void Projectile::tick(vector<smart_ptr<GfxEffects> > *gfxe) {
  CHECK(live);
  CHECK(age != -1);
  distance += len(movement()).toFloat();
  pos += movement();
  lasttail = nexttail();
  age++;
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > FPS / 6)
      missile_sidedist /= 1.2;
    for(int i = 0; i < 2; i++) {
      float dir = frand() * 2 * PI;
      Float2 pv = makeAngle(dir) / 3;
      pv *= 1.0 - frand() * frand();
      pv += movement().toFloat();
      pv += missile_accel().toFloat() * -3 * abs(gaussian());
      pv *= 60; // this is an actual 60, converting it from my previous movement-per-frame to movement-per-second
      gfxe->push_back(GfxPoint(pos.toFloat() + lasttail.toFloat() - movement().toFloat(), pv, 0.17, Color(1.0, 0.9, 0.6)));
    }
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity *= 0.95;
    if(airbrake_liveness() <= 0)
      detonating = true;
  } else if(projtype.motion() == PM_MINE) {
    if(frand() > pow(0.5f, 1 / (projtype.halflife() * FPS)))
      detonating = true;
  } else {
    CHECK(0);
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
  } else if(projtype.motion() == PM_MINE) {
    const float radarrange = 30;
    float closest = 1000;
    for(int i = 0; i < tankposes.size(); i++)
      if(len(tankposes[i] - pos).toFloat() < closest)
        closest = len(tankposes[i] - pos).toFloat();
    if(closest < radarrange) {
      setColor(C::gray((radarrange - closest) / radarrange));
      drawLineLoop(mine_polys(), 0.1);
    }
    return;
  } else {
    CHECK(0);
  }
  drawLine(Coord4(pos, pos + lasttail), projtype.thickness_visual());
};

void Projectile::firstCollide(Collider *collider, int id) const {
  if(projtype.motion() == PM_MINE) {
    vector<Coord2> ite = mine_polys();
    for(int i = 0; i < ite.size(); i++)
      collider->addToken(CollideId(CGR_STATPROJECTILE, owner, id), Coord4(ite[i], ite[(i + 1) % ite.size()]), Coord4(0, 0, 0, 0));
    //collider->markPersistent(CollideId(CGR_STATPROJECTILE, owner, id));
  }
}

void Projectile::addCollision(Collider *collider, int id) const {
  CHECK(live);
  if(projtype.motion() == PM_MINE) {
  } else {
    collider->addToken(CollideId(CGR_PROJECTILE, owner, id), Coord4(pos, pos + lasttail), Coord4(movement(), movement() + nexttail() - lasttail));
  }
}

void Projectile::collideCleanup(Collider *collider, int id) const {
  if(projtype.motion() == PM_MINE) {
    collider->dumpGroup(CollideId(CGR_STATPROJECTILE, owner, id));
  }
}

Coord2 Projectile::warheadposition() const {
  return pos;
}

void Projectile::detonate(Coord2 pos, Tank *target, const GamePlayerContext &gpc, bool impacted) {
  CHECK(gpc.owner == owner);
  if(!live)
    return;
  
  vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
  for(int i = 0; i < idw.size(); i++)
    detonateWarhead(idw[i], pos, movement() * FPS, target, gpc, 1.0, true, impacted);

  live = false;
};

bool Projectile::isLive() const {
  return live;
}

bool Projectile::isDetonating() const {
  return detonating;
}

Coord2 Projectile::movement() const {
  if(projtype.motion() == PM_NORMAL) {
    return makeAngle(Coord(d)) * Coord(projtype.velocity() / FPS);
  } else if(projtype.motion() == PM_MISSILE) {
    return missile_accel() + missile_backdrop() + missile_sidedrop();
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return Coord2(makeAngle(d) * airbrake_velocity / FPS);
  } else if(projtype.motion() == PM_MINE) {
    return Coord2(0, 0);
  } else {
    CHECK(0);
  }
}

Coord2 Projectile::nexttail() const {
  float maxlen = max(0., distance - 0.1);
  if(projtype.motion() == PM_NORMAL) {
    return Coord2(makeAngle(d) * -min(projtype.length(), maxlen));
  } else if(projtype.motion() == PM_MISSILE) {
    return Coord2(makeAngle(d) * -min(projtype.length(), maxlen));
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return Coord2(makeAngle(d) * -min(airbrake_velocity / FPS + 2, maxlen));
  } else if(projtype.motion() == PM_MINE) {
    return Coord2(0, 0);
  } else {
    CHECK(0);
  }
}

Coord2 Projectile::missile_accel() const {
  return makeAngle(Coord(d)) * Coord(projtype.velocity() / FPS) * age / FPS;
}
Coord2 Projectile::missile_backdrop() const {
  return makeAngle(Coord(d)) / 120;
}
Coord2 Projectile::missile_sidedrop() const {
  return makeAngle(Coord(d) - COORDPI / 2) * Coord(missile_sidedist);
}

float Projectile::airbrake_liveness() const {
  return 1.0 - (age / float(FPS) / projtype.airbrake_life());
}

vector<Coord2> Projectile::mine_polys() const {
  vector<Coord2> rv;
  const int rad = 12;
  for(int i = 0; i < rad; i++) {
    float expfact = (i % 2);
    expfact *= 3;
    expfact += 1;
    expfact /= 4;
    rv.push_back(Coord2(makeAngle(i * 2 * PI / rad + mine_facing) * projtype.radius_physical() * expfact) + pos);
  }
  return rv;
}

Projectile::Projectile() : projtype(NULL, IDBAdjustment()) {
  live = false;
  age = -1;
}
Projectile::Projectile(const Coord2 &in_pos, float in_d, const IDBProjectileAdjust &in_projtype, int in_owner) : projtype(in_projtype) {
  pos = in_pos;
  d = in_d;
  owner = in_owner;
  age = 0;
  live = true;
  detonating = false;
  lasttail = Coord2(0, 0);
  distance = 0;
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = gaussian() * 0.25;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity = (gaussian_scaled(2) / 4 + 1) * projtype.velocity();
  } else if(projtype.motion() == PM_MINE) {
    mine_facing = frand() * 2 * PI;
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
    projectiles[newitems[i]].firstCollide(collider, newitems[i]);
  newitems.clear();
  
  for(map<int, Projectile>::const_iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr) {
    itr->second.addCollision(collider, itr->first);
  }
}

void ProjectilePack::tick(vector<smart_ptr<GfxEffects> > *gfxe, Collider *collide, int id, const GameImpactContext &gic) {
  for(map<int, Projectile>::iterator itr = projectiles.begin(); itr != projectiles.end(); ) {
    // the logic here is kind of grim, sorry about this
    if(itr->second.isLive() && itr->second.isDetonating())
      itr->second.detonate(itr->second.warheadposition(), NULL, GamePlayerContext(id, this, gic), false);
    if(itr->second.isLive()) {
      itr->second.tick(gfxe);
      if(itr->second.isLive() && itr->second.isDetonating())
        itr->second.detonate(itr->second.warheadposition(), NULL, GamePlayerContext(id, this, gic), false);
      if(itr->second.isLive()) {
        ++itr;
        continue;
      }
    }
    
    CHECK(!itr->second.isLive());
    itr->second.collideCleanup(collide, itr->first);
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
