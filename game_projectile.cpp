
#include "game_projectile.h"

#include "collide.h"
#include "gfx.h"
#include "rng.h"
#include "game_tank.h"
#include "adler32_util.h"

using namespace std;

void Projectile::tick(vector<smart_ptr<GfxEffects> > *gfxe, const GameImpactContext &gic, int owner) {
  CHECK(live);
  CHECK(age != -1);
  last = now;
  age++;
  
  if(projtype.halflife() != -1 && age >= projtype.halflife() * FPS / 2 && gic.rng->frand() > pow(0.5f, 1 / (projtype.halflife() * FPS)))
    detonating = true;
  
  if(projtype.motion() == PM_NORMAL) {
    now.pi.pos += makeAngle(now.pi.d) * Coord(projtype.velocity()) / FPS;
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > projtype.missile_stabstart() * FPS)
      missile_sidedist /= pow(projtype.missile_stabilization(), 1.f / FPS);
    
    Coord2 mov = missile_accel() + missile_backdrop() + missile_sidedrop();
    
    for(int i = 0; i < 2; i++) { // generate particle effects
      float dir = unsync().frand() * 2 * PI;
      Float2 pv = makeAngle(dir) / 3;
      pv *= 1.0 - unsync().frand() * unsync().frand();
      pv += mov.toFloat();
      pv += missile_accel().toFloat() * -3 * abs(unsync().gaussian());
      pv *= FPS;
      gfxe->push_back(GfxPoint(rearspawn(now), pv, 0.17, Color(1.0, 0.9, 0.6)));
    }
    
    now.pi.pos += mov;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    now.airbrake_velocity *= pow(1.0 - projtype.airbrake_slowdown(), 1. / FPS);
    if(airbrake_liveness() <= 0)
      detonating = true;
    
    now.pi.pos += makeAngle(now.pi.d) * now.airbrake_velocity / FPS;
  } else if(projtype.motion() == PM_BOOMERANG) {
    Coord desang = getAngle(Coord2(-projtype.boomerang_intersection(), 0) - boomerang_abspos);
    while(desang < 0)
      desang += COORDPI * 2;
    Coord bchange = min(clamp((desang - boomerang_angle) * Coord(projtype.boomerang_convergence()) / FPS, Coord(0), Coord(projtype.boomerang_maxrotate()) / FPS), boomerang_lastchange);
    if(boomerang_angle > desang)
      bchange = 0;
    boomerang_lastchange = bchange;
    boomerang_angle += bchange;
    //dprintf("%p: %f, %f\n", this, boomerang_angle.toFloat(), bchange.toFloat());
    boomerang_abspos += makeAngle(boomerang_angle) * Coord(projtype.velocity()) / FPS;
    
    Coord2 mang = makeAngle(boomerang_angle) * Coord(projtype.velocity()) / FPS;
    mang.y *= boomerang_yfactor;
    now.pi.pos += rotate(mang, now.pi.d);
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
    int cid = gic.getClosestFoeId(now.pi.pos, owner);
    if(cid != -1) {
      Coord2 dp = gic.players[cid]->pos;
      dp -= now.pi.pos;
      if(len(dp) < Coord(projtype.proximity_visibility()) && len(dp) > 0) {
        now.pi.pos += normalize(dp) * Coord(projtype.velocity() / FPS);
      }
    }
  } else if(projtype.motion() == PM_DPS) {
    detonating = true;
  } else if(projtype.motion() == PM_HUNTER) {
    if(now.hunter_vel != 0)
      now.hunter_vel += projtype.velocity() / FPS;
    
    int lockon = -1;
    Coord dist = Coord(1 << 30);
    for(int i = 0; i < gic.players.size(); i++) {
      if(!gic.players[i]->isLive())
        continue;
      if(gic.players[i]->team == gic.players[owner]->team)
        continue;
      Coord dang = getAngle(gic.players[i]->pos - now.pi.pos);
      Coord diff = ang_dist(dang, now.pi.d);
      
      Coord val = diff * projtype.hunter_turnweight() + len(gic.players[i]->pos - now.pi.pos);
      if(val < dist) {
        dist = val;
        lockon = i;
      }
    }
    
    if(lockon != -1) {
      Coord dang = getAngle(gic.players[lockon]->pos - now.pi.pos);
      now.pi.d = ang_approach(now.pi.d, dang, projtype.hunter_rotation() / FPS);
      if(ang_dist(dang, now.pi.d) < projtype.hunter_rotation() / FPS && now.hunter_vel == 0)
        now.hunter_vel += projtype.velocity() / FPS;
    }
    
    now.pi.pos += makeAngle(now.pi.d) * now.hunter_vel;
  } else {
    CHECK(0);
  }
  
  if(projtype.shape() == PS_ARROW) {
    now.arrow_spin += projtype.arrow_rotate() / FPS * (arrow_spin_parity * 2 - 1);
  }
  
  if(projtype.proximity() != -1) {
    Coord newdist = gic.getClosestFoeDistance(now.pi.pos, owner);
    if(newdist > closest_enemy_tank && newdist < projtype.proximity())
      detonating = true; // BOOM
    closest_enemy_tank = newdist;
  }
  
  distance_traveled += len(last.pi.pos - now.pi.pos);
}

void Projectile::render(const vector<Coord2> &tankposes) const {
  CHECK(live);
  CHECK(age != -1);
  
  bool visible = true;
  
  if(projtype.shape() == PS_INVISIBLE) {
    visible = false;
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
    CHECK(projtype.proximity_visibility() == -1);
    setColor(projtype.color() * (1.0 - projtype.airbrake_life() / age));
  } else if(projtype.proximity_visibility() == -1) {
    setColor(projtype.color());
  } else if(projtype.proximity_visibility() != -1) {
    const float range = projtype.proximity_visibility();
    float closest = range * 2;
    for(int i = 0; i < tankposes.size(); i++)
      if(len(tankposes[i] - now.pi.pos).toFloat() < closest)
        closest = len(tankposes[i] - now.pi.pos).toFloat();
    if(closest < range) {
      if(closest < range / 2) {
        setColor(projtype.color());
      } else {
        setColor(projtype.color() * ((range - closest) / range * 2));
      }
    } else {
      visible = false;
    }
  }
  
  if(visible) {
    if(projtype.shape() == PS_LINE || projtype.shape() == PS_LINE_AIRBRAKE || projtype.shape() == PS_ARROW || projtype.shape() == PS_STAR || projtype.shape() == PS_DRONE) {
      vector<Coord4> ps = polys(now);
      for(int i = 0; i < ps.size(); i++)
        drawLine(ps[i], projtype.visual_thickness());
    } else {
      CHECK(0);
    }
  }
};

void Projectile::checksum(Adler32 *adl) const {
  adler(adl, projtype);
  adler(adl, age);
  adler(adl, live);
  adler(adl, detonating);
  adler(adl, damageflags);
  
  adler(adl, now);
  adler(adl, last);
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    adler(adl, missile_sidedist);
  } else if(projtype.motion() == PM_AIRBRAKE) {
  } else if(projtype.motion() == PM_BOOMERANG) {
    adler(adl, boomerang_abspos);
    adler(adl, boomerang_yfactor);
    adler(adl, boomerang_angle);
    adler(adl, boomerang_lastchange);
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
  } else if(projtype.motion() == PM_DPS) {
  } else if(projtype.motion() == PM_HUNTER) {
  } else {
    CHECK(0);
  }
  
  if(projtype.shape() == PS_LINE) {
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
  } else if(projtype.shape() == PS_ARROW) {
    adler(adl, arrow_spin_parity);
  } else if(projtype.shape() == PS_STAR) {
    adler(adl, star_facing);
  } else if(projtype.shape() == PS_DRONE) {
  } else if(projtype.shape() == PS_INVISIBLE) {
  } else {
    CHECK(0);
  }
}

void Projectile::firstCollide(Collider *collider, int owner, int id) const {
  if(projtype.motion() == PM_MINE) {
    CHECK(projtype.shape() == PS_STAR);
    vector<Coord4> ite = polys(now);
    for(int i = 0; i < ite.size(); i++)
      collider->addUnmovingToken(CollideId(CGR_STATPROJECTILE, owner, id), Coord4(ite[i].s(), ite[i].e()));
  }
}

void Projectile::addCollision(Collider *collider, int owner, int id) const {
  CHECK(live);
  if(projtype.motion() == PM_MINE) {
    // this is kind of a special case ATM
  } else if(projtype.no_intersection()) {
    collider->addPointToken(CollideId(CGR_NOINTPROJECTILE, owner, id), now.pi.pos, last.pi.pos - now.pi.pos);
  } else {
    int pt = CGR_PROJECTILE;
    
    if(projtype.motion() == PM_SPIDERMINE)
      pt = CGR_NOINTPROJECTILE;
    
    vector<Coord4> ps = polys(now);
    vector<Coord4> psl = polys(last);
    for(int i = 0; i < ps.size(); i++)
      collider->addNormalToken(CollideId(pt, owner, id), ps[i], ps[i] - psl[i]);
  }
}

void Projectile::collideCleanup(Collider *collider, int owner, int id) const {
  if(projtype.motion() == PM_MINE) {
    CHECK(projtype.shape() == PS_STAR);
    collider->dumpGroup(CollideId(CGR_STATPROJECTILE, owner, id));
  }
}

Coord2 Projectile::warheadposition() const {
  return now.pi.pos;
}

void Projectile::detonate(Coord2 pos, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted) {
  if(!live)
    return;
  
  if(projtype.motion() != PM_DPS) {
    vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
    for(int i = 0; i < idw.size(); i++)
      detonateWarhead(idw[i], pos, normal, (now.pi.pos - last.pi.pos) * FPS, target, gpc, damageflags, impacted);
    
    vector<IDBDeployAdjust> idd = projtype.chain_deploy();
    for(int i = 0; i < idd.size(); i++)
      deployProjectile(idd[i], DeployLocation(pos, getAngle(now.pi.pos - last.pi.pos)), gpc, damageflags, NULL);
    
    vector<IDBEffectsAdjust> ide = projtype.chain_effects();
    for(int i = 0; i < ide.size(); i++)
      gpc.gic->effects->push_back(GfxIdb(pos.toFloat(), normal.toFloat(), (now.pi.pos - last.pi.pos).toFloat(), ide[i]));
    
    live = false;
  } else if(projtype.motion() == PM_DPS) {
    CHECK(!projtype.chain_deploy().size());
    CHECK(!projtype.chain_effects().size());
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

Coord2 Projectile::missile_accel() const {
  return makeAngle(Coord(now.pi.d)) * Coord(projtype.velocity() / FPS) * age / FPS;
}
Coord2 Projectile::missile_backdrop() const {
  return -makeAngle(Coord(now.pi.d)) * Coord(projtype.missile_backlaunch()) / FPS;
}
Coord2 Projectile::missile_sidedrop() const {
  return makeAngle(Coord(now.pi.d) - COORDPI / 2) * Coord(missile_sidedist);
}

Coord Projectile::airbrake_liveness() const {
  return 1 - (Coord(age) / FPS / projtype.airbrake_life());
}

vector<Coord4> Projectile::polys(const ProjPostState &stat) const {
  vector<Coord4> rv;
  if(projtype.shape() == PS_LINE) {
    Coord maxlen = max(0, distance_traveled - 0.1);
    rv.push_back(Coord4(stat.pi.pos, stat.pi.pos - makeAngle(stat.pi.d) * -min(Coord(projtype.line_length()), maxlen)));
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
    Coord maxlen = max(0, distance_traveled - 0.1);
    rv.push_back(Coord4(stat.pi.pos, stat.pi.pos - makeAngle(stat.pi.d) * -min(stat.airbrake_velocity / FPS + 2, maxlen)));
  } else if(projtype.shape() == PS_ARROW) {
    Coord2 l = stat.pi.pos + rotate(Coord2(projtype.arrow_width() / 2, projtype.arrow_height() / 2), Coord(stat.arrow_spin));
    Coord2 c = stat.pi.pos + rotate(Coord2(0, -projtype.arrow_height() / 2), Coord(stat.arrow_spin));
    Coord2 r = stat.pi.pos + rotate(Coord2(-projtype.arrow_width() / 2, projtype.arrow_height() / 2), Coord(stat.arrow_spin));
    rv.push_back(Coord4(l, c));
    rv.push_back(Coord4(c, r));
  } else if(projtype.shape() == PS_STAR) {
    const int rad = projtype.star_spikes() * 2;
    CHECK(projtype.star_radius() > 0);
    Coord2 lpt = Coord2(0, 0);
    for(int i = 0; i < rad + 1; i++) {
      Coord expfact = (i % 2);
      expfact *= 3;
      expfact += 1;
      expfact /= 4;
      Coord2 tpt = Coord2(makeAngle(i * 2 * PI / rad + star_facing) * projtype.star_radius() * expfact);
      if(lpt != Coord2(0, 0))
        rv.push_back(Coord4(lpt + stat.pi.pos, tpt + stat.pi.pos));
      lpt = tpt;
    }
  } else if(projtype.shape() == PS_INVISIBLE) {
  } else if(projtype.shape() == PS_DRONE) {
    Coord2 opt = stat.pi.pos + makeAngle(stat.pi.d) * projtype.drone_spike();
    Coord2 lpt = opt;
    for(int i = 1; i < 4; i++) {
      Coord2 npt = stat.pi.pos + makeAngle(stat.pi.d + COORDPI * i / 2) * projtype.drone_radius();
      rv.push_back(Coord4(lpt, npt));
      lpt = npt;
    }
    rv.push_back(Coord4(lpt, opt));
  } else {
    CHECK(0);
  }
  return rv;
}

Float2 Projectile::rearspawn(const ProjPostState &stat) const {
  if(projtype.shape() == PS_LINE) {
    return (stat.pi.pos - makeAngle(stat.pi.d) * projtype.line_length()).toFloat();
  } else if(projtype.shape() == PS_DRONE) {
    return (stat.pi.pos - makeAngle(stat.pi.d) * projtype.drone_radius()).toFloat();
  } else {
    CHECK(0);
  }
}

Projectile::Projectile() : projtype(NULL, IDBAdjustment()), damageflags(0.0, false, false) {
  live = false;
  age = -1;
}
Projectile::Projectile(const Coord2 &in_pos, Coord in_d, const IDBProjectileAdjust &projtype, Rng *rng, const DamageFlags &damageflags) : projtype(projtype), damageflags(damageflags) {
  now.pi.pos = in_pos;
  now.pi.d = in_d;
  age = 0;
  live = true;
  detonating = false;
  distance_traveled = 0;
  closest_enemy_tank = 1e20; // no
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = rng->gaussian() * projtype.missile_sidelaunch() / FPS;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    now.airbrake_velocity = (rng->gaussian_scaled(2) / 4 + 1) * projtype.velocity();
  } else if(projtype.motion() == PM_BOOMERANG) {
    boomerang_abspos = Coord2(0, 0);
    boomerang_yfactor = cfcos(Coord(rng->frand()) * COORDPI / 4 + COORDPI / 8 * 3);
    boomerang_angle = 0;
    boomerang_lastchange = 1000000;
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
  } else if(projtype.motion() == PM_DPS) {
  } else if(projtype.motion() == PM_HUNTER) {
  } else {
    CHECK(0);
  }
  
  if(projtype.shape() == PS_LINE || projtype.shape() == PS_LINE_AIRBRAKE) {
  } else if(projtype.shape() == PS_ARROW) {
    arrow_spin_parity = rng->frand() < 0.5;
    now.arrow_spin = in_d.toFloat() + PI / 2;
  } else if(projtype.shape() == PS_STAR) {
    star_facing = rng->frand() * 2 * PI;
  } else if(projtype.shape() == PS_DRONE) {
    now.hunter_vel = 0;
  } else if(projtype.shape() == PS_INVISIBLE) {
  } else {
    CHECK(0);
  }
  
  last = now;
}

Projectile::ProjPostState::ProjPostState() {
  pi.pos = Coord2(0, 0);
  pi.d = 0;
  airbrake_velocity = 0;
  arrow_spin = 0;
}

void adler(Adler32 *adl, const Projectile::ProjPostState &pps) {
  adler(adl, pps.pi);
  adler(adl, pps.airbrake_velocity);
  adler(adl, pps.arrow_spin);
  adler(adl, pps.hunter_vel);
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

void ProjectilePack::checksum(Adler32 *adl) const {
  adler(adl, projectiles);
  adler(adl, aid);
  adler(adl, newitems);
  adler(adl, cleanup);
}
