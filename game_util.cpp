
#include "game_util.h"

#include "game_tank.h"
#include "rng.h"
#include "gfx.h"

void dealDamage(float dmg, TPP target, TPP owner, float damagecredit, bool killcredit) {
  if(target.tank()->team == owner.tank()->team)
    return; // friendly fire exception
  if(target.tank()->takeDamage(dmg) && killcredit)
    owner.player()->addKill();
  owner.player()->addDamage(dmg * damagecredit);
};

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, TPP impact, TPP owner, const vector<pair<float, TPP> > &adjacency, vector<smart_ptr<GfxEffects> > *gfxe, Gamemap *gm, float damagecredit, bool killcredit) {
  
  if(impact)
    dealDamage(warhead.impactdamage(), impact, owner, damagecredit, killcredit);
  
  for(int i = 0; i < adjacency.size(); i++) {
    if(adjacency[i].first < warhead.radiusfalloff())
      dealDamage(warhead.radiusdamage() / warhead.radiusfalloff() * (warhead.radiusfalloff() - adjacency[i].first), adjacency[i].second, owner, damagecredit, killcredit);
  }
  
  for(int i = 0; i < 6; i++)
    gfxe->push_back(GfxPoint(pos.toFloat(),  (makeAngle(frand() * 2 * PI) * 20) * (1.0 - frand() * frand()), 0.1, Color(1.0, 1.0, 1.0)));
  
  if(warhead.radiusfalloff() > 0)
    gfxe->push_back(GfxBlast(pos.toFloat(), warhead.radiusfalloff(), warhead.radiuscolor_bright(), warhead.radiuscolor_dim()));
  
  if(warhead.wallremovalradius() > 0 && frand() < warhead.wallremovalchance()) {
    gm->removeWalls(pos, warhead.wallremovalradius());
  }

};

Team::Team() {
  weapons_enabled = true;
  color = Color(0, 0, 0);
  swap_colors = false;
}

void Projectile::tick(vector<smart_ptr<GfxEffects> > *gfxe) {
  CHECK(live);
  CHECK(age != -1);
  pos += movement();
  lasttail = nexttail();
  age++;
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > 10)
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
      live = false;
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
void Projectile::addCollision(Collider *collider) const {
  CHECK(live);
  if(projtype.motion() == PM_MINE) {
    vector<Coord2> ite = mine_polys();
    for(int i = 0; i < ite.size(); i++)
      collider->token(Coord4(ite[i], ite[(i + 1) % ite.size()]), Coord4(0, 0, 0, 0));
  } else {
    collider->token(Coord4(pos, pos + lasttail), Coord4(movement(), movement() + nexttail()));
  }
};

Coord2 Projectile::warheadposition() const {
  return pos;
}

void Projectile::impact(Coord2 pos, TPP target, const vector<pair<float, TPP> > &adjacency, vector<smart_ptr<GfxEffects> > *gfxe, Gamemap *gm, const vector<TPP> &players) {
  if(!live)
    return;
  
  detonateWarhead(projtype.warhead(), pos, target, players[owner], adjacency, gfxe, gm, 1.0, true);

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
    return makeAngle(Coord(d)) * Coord(projtype.velocity());
  } else if(projtype.motion() == PM_MISSILE) {
    return missile_accel() + missile_backdrop() + missile_sidedrop();
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return Coord2(makeAngle(d) * airbrake_velocity);
  } else if(projtype.motion() == PM_MINE) {
    return Coord2(0, 0);
  } else {
    CHECK(0);
  }
  
}

Coord2 Projectile::nexttail() const {
  if(projtype.motion() == PM_NORMAL) {
    return -movement();
  } else if(projtype.motion() == PM_MISSILE) {
    return Coord2(makeAngle(d) * -2);
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return Coord2(-makeAngle(d) * (airbrake_velocity + 2));
  } else if(projtype.motion() == PM_MINE) {
    return Coord2(0, 0);
  } else {
    CHECK(0);
  }
}

Coord2 Projectile::missile_accel() const {
  return makeAngle(Coord(d)) * Coord(projtype.velocity()) * age / 60;
}
Coord2 Projectile::missile_backdrop() const {
  return makeAngle(Coord(d)) / 120;
}
Coord2 Projectile::missile_sidedrop() const {
  return makeAngle(Coord(d) - COORDPI / 2) * Coord(missile_sidedist);
}

float Projectile::airbrake_liveness() const {
  return 1.0 - (age / 60.0);
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
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = gaussian() * 0.25;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity = (gaussian_scaled(2) / 4 + 1) * projtype.velocity();
  } else if(projtype.motion() == PM_MINE) {
    mine_facing = frand() * 2 * PI;
  } else if(projtype.motion() == PM_INSTANT) {
  } else {
    CHECK(0);
  }
}
