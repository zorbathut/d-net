
#include "game_projectile.h"

#include <numeric>

#include "adler32_util.h"
#include "collide.h"
#include "game_tank.h"
#include "gfx.h"

using namespace std;

void Projectile::tick(const GameImpactContext &gic, int owner) {
  CHECK(live);
  CHECK(age != -1);
  last = now;
  //bool firsttick = (age == 0);
  
  age += Coord(1) / FPS;
  
  if(projtype.halflife() != -1 && age >= projtype.halflife() / 2 && gic.rng->frand() > pow(0.5f, 1 / (projtype.halflife() * FPS)))
    detonating = true;
  
  if(projtype.motion() == PM_NORMAL) {
    now.pi.pos += makeAngle(now.pi.d) * velocity / FPS;
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > projtype.missile_stabstart())
      missile_sidedist /= pow(projtype.missile_stabilization(), 1.f / FPS);
    
    now.pi.pos += missile_accel() + missile_backdrop() + missile_sidedrop();
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
    boomerang_abspos += makeAngle(boomerang_angle) * velocity / FPS;
    
    Coord2 mang = makeAngle(boomerang_angle) * velocity / FPS;
    mang.y *= boomerang_yfactor;
    now.pi.pos += rotate(mang, now.pi.d);
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
    int cid = gic.getClosestFoeId(now.pi.pos, owner);
    if(cid != -1) {
      Coord2 dp = gic.players[cid]->pi.pos;
      dp -= now.pi.pos;
      if(len(dp) < Coord(projtype.proximity_visibility()) && len(dp) > 0) {
        now.pi.pos += normalize(dp) * velocity / FPS;
      }
    }
  } else if(projtype.motion() == PM_DPS) {
    detonating = true;
  } else if(projtype.motion() == PM_HUNTER) {
    now.pi.pos += hk_drift;
    hk_drift *= 0.95;
    
    if(now.hunter_vel != 0)
      now.hunter_vel += velocity / FPS;
    
    int lockon = -1;
    Coord dist = Coord(1 << 30);
    for(int i = 0; i < gic.players.size(); i++) {
      if(!gic.players[i]->isLive())
        continue;
      if(gic.players[i]->team == gic.players[owner]->team)
        continue;
      Coord dang = getAngle(gic.players[i]->pi.pos - now.pi.pos);
      Coord diff = ang_dist(dang, now.pi.d);
      
      Coord val = diff * projtype.hunter_turnweight() + len(gic.players[i]->pi.pos - now.pi.pos);
      if(val < dist) {
        dist = val;
        lockon = i;
      }
    }
    
    if(lockon != -1) {
      Coord dang = getAngle(gic.players[lockon]->pi.pos - now.pi.pos);
      now.pi.d = ang_approach(now.pi.d, dang, projtype.hunter_rotation() / FPS);
      if(ang_dist(dang, now.pi.d) < projtype.hunter_rotation() && now.hunter_vel == 0)
        now.hunter_vel += velocity / FPS;
    }
    
    now.pi.pos += makeAngle(now.pi.d) * now.hunter_vel / FPS;
  } else if(projtype.motion() == PM_SINE) {
    now.sine_phase += 1.0 / sine_frequency / FPS * COORDPI * 2;
    now.pi.pos += makeAngle(now.pi.d) * velocity / FPS;
    now.pi.pos += makeAngle(now.pi.d + COORDPI / 2) * projtype.sine_width() / projtype.sine_frequency() * cfcos(now.sine_phase) / FPS; // hee hee
  } else if(projtype.motion() == PM_DELAY) {
    if(age >= projtype.delay_duration())
      detonating = true;
  } else if(projtype.motion() == PM_GENERATOR) {
    detonating = true; // this is pretty much all we do honestly
  } else {
    CHECK(0);
  }
  
  if(projtype.shape() == PS_ARROW) {
    now.arrow_spin += projtype.arrow_rotate() / FPS * (arrow_spin_parity * 2 - 1);
  }
  
  /*
  if(firsttick)
    now.pi.pos = lerp(last.pi.pos, now.pi.pos, gic.rng->frand());*/
  
  if(projtype.proximity() != -1) {
    Coord newdist = gic.getClosestFoeDistance(now.pi.pos, owner);
    if(newdist > closest_enemy_tank && newdist < projtype.proximity())
      detonating = true; // BOOM
    closest_enemy_tank = newdist;
  }
  
  if(projtype.proximity_visibility() != -1) {
    const Coord range = projtype.proximity_visibility();
    Coord closest = range * 2;
    for(int i = 0; i < gic.players.size(); i++) {
      if(!gic.players[i]->isLive())
        continue;
      closest = min(closest, len(gic.players[i]->pi.pos - now.pi.pos));
    }
    
    Coord goalvis;
    if(closest < range) {
      if(closest < range / 2) {
        goalvis = 1;
      } else {
        goalvis = (range - closest) / range * 2;
      }
    } else {
      goalvis = 0;
    }
    
    proxy_visibility = max(goalvis, approach(proxy_visibility, goalvis, Coord(1) / FPS));
  }
  
  now.distance_traveled += len(last.pi.pos - now.pi.pos);
}

void Projectile::spawnEffects(vector<smart_ptr<GfxEffects> > *gfxe) const {
  for(int i = 0; i < projtype.burn_effects().size(); i++) {
    if(projtype.motion() == PM_MISSILE) {
      gfxe->push_back(GfxIdb(rearspawn(now), now.pi.d.toFloat(), (now.pi.pos - last.pi.pos).toFloat() * FPS, -missile_accel().toFloat() * FPS, projtype.burn_effects()[i]));
    } else {
      gfxe->push_back(GfxIdb(rearspawn(now), now.pi.d.toFloat(), (now.pi.pos - last.pi.pos).toFloat() * FPS, projtype.burn_effects()[i]));
    }
  }
}

void Projectile::render() const {
  CHECK(live);
  CHECK(age != -1);
  
  bool visible = true;
  
  if(projtype.shape() == PS_INVISIBLE) {
    visible = false;
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
    CHECK(projtype.proximity_visibility() == -1);
    setColor(projtype.color() * airbrake_liveness().toFloat());
  } else if(projtype.proximity_visibility() == -1) {
    setColor(projtype.color());
  } else if(projtype.proximity_visibility() != -1) {
    if(proxy_visibility == 0)
      visible = false;
    else
      setColor(projtype.color() * proxy_visibility.toFloat());
  }
  
  if(visible) {
    if(projtype.shape() == PS_LINE || projtype.shape() == PS_LINE_AIRBRAKE || projtype.shape() == PS_ARROW || projtype.shape() == PS_STAR || projtype.shape() == PS_DRONE || projtype.shape() == PS_ARCPIECE) {
      vector<Coord4> ps = polys(now);
      for(int i = 0; i < ps.size(); i++)
        drawLine(ps[i], projtype.visual_thickness());
    } else {
      CHECK(0);
    }
  }
  
  /*
  if(projtype.motion() == PM_HUNTER) {
    setColor(C::gray(1.0));
    for(float i = 0; i < PI * 2; i += 0.04)
      drawPoint(makeAngle(i + now.pi.d.toFloat()) * abs(PI - i) * projtype.hunter_turnweight() + now.pi.pos.toFloat(), 1);
  }*/
};

void Projectile::checksum(Adler32 *adl) const {
  //reg_adler_intermed(*adl);
  adler(adl, projtype);
  adler(adl, age);
  adler(adl, live);
  adler(adl, detonating);
  adler(adl, damageflags);
  adler(adl, first);
  
  //reg_adler_intermed(*adl);
  adler(adl, now);
  //reg_adler_intermed(*adl);
  adler(adl, last);
  //reg_adler_intermed(*adl);
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    adler(adl, missile_sidedist);
  } else if(projtype.motion() == PM_AIRBRAKE) {
  } else if(projtype.motion() == PM_BOOMERANG) {
    adler(adl, boomerang_abspos);
    adler(adl, boomerang_yfactor);
    adler(adl, boomerang_angle);
    adler(adl, boomerang_lastchange);
  } else if(projtype.motion() == PM_SINE) {
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
  } else if(projtype.motion() == PM_DPS) {
  } else if(projtype.motion() == PM_HUNTER) {
    adler(adl, hk_drift);
  } else if(projtype.motion() == PM_DELAY) {
  } else if(projtype.motion() == PM_GENERATOR) {
  } else {
    CHECK(0);
  }
  //reg_adler_intermed(*adl);
  
  if(projtype.shape() == PS_LINE) {
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
  } else if(projtype.shape() == PS_ARROW) {
    adler(adl, arrow_spin_parity);
  } else if(projtype.shape() == PS_STAR) {
    adler(adl, star_facing);
  } else if(projtype.shape() == PS_DRONE) {
  } else if(projtype.shape() == PS_ARCPIECE) {
  } else if(projtype.shape() == PS_INVISIBLE) {
  } else {
    CHECK(0);
  }
  //reg_adler_intermed(*adl);
  
  if(projtype.proximity_visibility() != -1)
    adler(adl, proxy_visibility);
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
  if(projtype.motion() == PM_MINE || projtype.motion() == PM_DELAY) {
    // this is kind of a special case ATM
  } else if(projtype.no_intersection()) {
    collider->addPointToken(CollideId(CGR_NOINTPROJECTILE, owner, id), now.pi.pos, last.pi.pos - now.pi.pos);
  } else {
    int pt = CGR_PROJECTILE;
    
    if(projtype.motion() == PM_SPIDERMINE)
      pt = CGR_NOINTPROJECTILE;
    
    vector<Coord4> ps = polys(now);
    vector<Coord4> psl = polys(last);
    CHECK(ps.size() == psl.size());
    for(int i = 0; i < ps.size(); i++) {
      collider->addNormalToken(CollideId(pt, owner, id), psl[i], ps[i] - psl[i]);
    }
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

void Projectile::trigger(Coord t, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted) {
  if(!live)
    return;
  
  Coord2 pos = lerp(last.pi.pos, now.pi.pos, t);
  
  if(projtype.motion() == PM_DPS) {
    CHECK(!projtype.chain_deploy().size());
    CHECK(!projtype.chain_effects().size());

    if(first) {
      vector<IDBWarheadAdjust> idwi = projtype.dps_instant_warhead();
      for(int i = 0; i < idwi.size(); i++)
        detonateWarhead(idwi[i], pos, 0, Coord2(0, 0), NULL, gpc, damageflags, false);
      
      gpc.gic->effects->push_back(GfxIonBlast(pos.toFloat(), projtype.dps_duration(), projtype.chain_warhead()[0].radiusfalloff().toFloat(), projtype.dps_visuals()));
    }
    
    if(age > projtype.dps_duration()) {
      live = false;
    } else {
      int alen = int(projtype.dps_duration() * FPS);
      int shares = alen * (alen + 1) / 2;
      int tshares = alen - round(age * FPS).toInt() + 1;
      vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
      for(int i = 0; i < idw.size(); i++)
        detonateWarhead(idw[i].multiply((Coord)tshares / shares), pos, 0, Coord2(0, 0), NULL, gpc, damageflags, false);
      detonating = false;
    }
  } else if(projtype.motion() == PM_GENERATOR) {
    if(gpc.gic->rng->frand() < pow(projtype.generator_falloff(), age) * projtype.generator_per_second() / FPS)
      triggerstandard(pos, normal, target, gpc, impacted);
    
    if(age >= projtype.generator_duration())
      live = false;
  } else {
    triggerstandard(pos, normal, target, gpc, impacted);
    
    if(!projtype.penetrating() || !target)
      live = false; // otherwise we just keep on truckin'
  }
  
  first = false;
};

void Projectile::triggerstandard(Coord2 pos, Coord normal, Tank *target, const GamePlayerContext &gpc, bool impacted) {
  vector<IDBWarheadAdjust> idw = projtype.chain_warhead();
  for(int i = 0; i < idw.size(); i++)
    detonateWarhead(idw[i], pos, normal, (now.pi.pos - last.pi.pos) * FPS, target, gpc, damageflags, impacted);
  
  vector<IDBDeployAdjust> idd = projtype.chain_deploy();
  for(int i = 0; i < idd.size(); i++)
    deployProjectile(idd[i], DeployLocation(pos, getAngle(now.pi.pos - last.pi.pos), normal), gpc, damageflags, NULL);
  
  vector<IDBEffectsAdjust> ide = projtype.chain_effects();
  for(int i = 0; i < ide.size(); i++)
    gpc.gic->effects->push_back(GfxIdb(pos.toFloat(), normal.toFloat(), (now.pi.pos - last.pi.pos).toFloat(), ide[i]));
}

void Projectile::frame_spawns(const GamePlayerContext &gpc) {
  vector<IDBDeployAdjust> idd = projtype.poly_deploy();
  if(idd.size()) {
    vector<Coord4> poly = polys(now);
    vector<Coord> pl;
    for(int i = 0; i < poly.size(); i++)
      pl.push_back(len(poly[i].s() - poly[i].e()));
    Coord pl_tot = accumulate(pl.begin(), pl.end(), Coord(0));
    
    for(int i = 0; i < idd.size(); i++) {
      Coord spawnpoint = gpc.gic->rng->cfrand() * pl_tot;
      //dprintf("sp %f, first %f\n", spawnpoint.toFloat(), pl[0].toFloat());
      int spawnstart;
      for(spawnstart = 0; spawnstart < poly.size(); spawnstart++) {
        if(pl[spawnstart] >= spawnpoint)
          break;
        spawnpoint -= pl[spawnstart];
      }
      //dprintf("sp %f, first %f\n", spawnpoint.toFloat(), pl[0].toFloat());
      CHECK(spawnstart < poly.size());
      Coord2 pt = lerp(poly[spawnstart].s(), poly[spawnstart].e(), spawnpoint / pl[spawnstart]);
      deployProjectile(idd[i], DeployLocation(pt, now.pi.d), gpc, damageflags);
    }
  }
}

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
  return makeAngle(now.pi.d) * velocity / FPS * age;
}
Coord2 Projectile::missile_backdrop() const {
  return -makeAngle(now.pi.d) * Coord(projtype.missile_backlaunch()) / FPS;
}
Coord2 Projectile::missile_sidedrop() const {
  return makeAngle(now.pi.d - COORDPI / 2) * Coord(missile_sidedist);
}

Coord Projectile::airbrake_liveness() const {
  return 1 - age / projtype.airbrake_life();
}

vector<Coord4> Projectile::polys(const ProjPostState &stat) const {
  vector<Coord4> rv;
  if(projtype.shape() == PS_LINE) {
    Coord maxlen = max(0, stat.distance_traveled);
    Coord locald = stat.pi.d;
    if(projtype.motion() == PM_SINE) { // we have to do something special to account for the sine movement - this should technically probably be another shape, but, eh
      locald = getAngle(makeAngle(now.pi.d) * velocity + makeAngle(now.pi.d + COORDPI / 2) * projtype.sine_width() / projtype.sine_frequency() * cfcos(now.sine_phase));
    }
    rv.push_back(Coord4(stat.pi.pos, stat.pi.pos - makeAngle(locald) * min(Coord(projtype.line_length()), maxlen)));
  } else if(projtype.shape() == PS_LINE_AIRBRAKE) {
    Coord maxlen = max(0, stat.distance_traveled);
    rv.push_back(Coord4(stat.pi.pos, stat.pi.pos - makeAngle(stat.pi.d) * min(stat.airbrake_velocity / FPS + projtype.line_airbrake_lengthaddition(), maxlen)));
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
  } else if(projtype.shape() == PS_ARCPIECE) {
    Coord arcrad = (Coord)projtype.arc_width() / projtype.arc_units() / 2;
    Coord2 origin = stat.pi.pos - makeAngle(stat.pi.d) * stat.distance_traveled;
    rv.push_back(Coord4(origin + makeAngle(stat.pi.d - arcrad) * stat.distance_traveled, origin + makeAngle(stat.pi.d + arcrad) * stat.distance_traveled));
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
  velocity = -20;
}
Projectile::Projectile(const Coord2 &in_pos, Coord in_d, const IDBProjectileAdjust &projtype, Rng *rng, const DamageFlags &damageflags) : projtype(projtype), damageflags(damageflags) {
  now.pi.pos = in_pos;
  now.pi.d = in_d;
  age = 0;
  first = true;
  live = true;
  detonating = false;
  now.distance_traveled = 0;
  closest_enemy_tank = 1e20; // no
  
  if(projtype.motion() == PM_NORMAL || projtype.motion() == PM_AIRBRAKE || projtype.motion() == PM_MISSILE || projtype.motion() == PM_BOOMERANG || projtype.motion() == PM_SPIDERMINE || projtype.motion() == PM_HUNTER || projtype.motion() == PM_SINE) {
    velocity = projtype.velocity() + projtype.velocity_stddev() * rng->sym_frand();
  } else {
    velocity = -50; // lulz
  }
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = rng->gaussian() * projtype.missile_sidelaunch() / FPS;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    now.airbrake_velocity = (rng->gaussian_scaled(2) / 4 + 1) * velocity;
  } else if(projtype.motion() == PM_BOOMERANG) {
    boomerang_abspos = Coord2(0, 0);
    boomerang_yfactor = cfcos(Coord(rng->frand()) * COORDPI / 4 + COORDPI / 8 * 3);
    boomerang_angle = 0;
    boomerang_lastchange = 1000000;
  } else if(projtype.motion() == PM_SINE) {
    now.sine_phase = rng->cfrand() * COORDPI * 2;
    sine_frequency = projtype.sine_frequency() + projtype.sine_frequency_stddev() * rng->sym_frand();
  } else if(projtype.motion() == PM_MINE) {
  } else if(projtype.motion() == PM_SPIDERMINE) {
  } else if(projtype.motion() == PM_DPS) {
  } else if(projtype.motion() == PM_HUNTER) {
    hk_drift = Coord2(rng->sym_frand(), rng->sym_frand());
    hk_drift *= abs(hk_drift.y);
    hk_drift = rotate(hk_drift, in_d);
    hk_drift /= 2;
  } else if(projtype.motion() == PM_DELAY) {
  } else if(projtype.motion() == PM_GENERATOR) {
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
  } else if(projtype.shape() == PS_ARCPIECE) {
    CHECK(projtype.motion() == PM_NORMAL || projtype.motion() == PM_AIRBRAKE);
  } else if(projtype.shape() == PS_INVISIBLE) {
  } else {
    CHECK(0);
  }
  
  if(projtype.proximity_visibility() != -1)
    proxy_visibility = 1;
  
  last = now;
}

Projectile::ProjPostState::ProjPostState() {
  pi.pos = Coord2(0, 0);
  pi.d = 0;
  airbrake_velocity = 0;
  arrow_spin = 0;
  hunter_vel = 0;
}

void adler(Adler32 *adl, const Projectile::ProjPostState &pps) {
  adler(adl, pps.pi);
  adler(adl, pps.airbrake_velocity);
  adler(adl, pps.arrow_spin);
  adler(adl, pps.hunter_vel);
  adler(adl, pps.distance_traveled);
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

void ProjectilePack::tick(Collider *collide, int owner, const GameImpactContext &gic) {
  set<int> existnow;
  for(map<int, Projectile>::iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr)
    existnow.insert(itr->first);
    
  for(map<int, Projectile>::iterator itr = projectiles.begin(); itr != projectiles.end(); ) {
    if(!existnow.count(itr->first)) {
      ++itr;
      continue;
    }
    
    // the logic here is kind of grim, sorry about this
    if(itr->second.isLive() && itr->second.isDetonating())
      itr->second.trigger(1, NO_NORMAL, NULL, GamePlayerContext(gic.players[owner], this, gic), false);
    if(itr->second.isLive()) {
      if(!count(newitems.begin(), newitems.end(), itr->first)) {  // we make sure we do collisions before ticks
        itr->second.frame_spawns(GamePlayerContext(gic.players[owner], this, gic));
        itr->second.tick(gic, owner);
      }
      if(itr->second.isLive() && itr->second.isDetonating())
        itr->second.trigger(1, NO_NORMAL, NULL, GamePlayerContext(gic.players[owner], this, gic), false);
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

void ProjectilePack::spawnEffects(vector<smart_ptr<GfxEffects> > *gfxe) const {
  for(map<int, Projectile>::const_iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr) {
    if(itr->second.isLive())
      itr->second.spawnEffects(gfxe);
  }
}

void ProjectilePack::cleanup(Collider *collide, int owner) {
  for(map<int, Projectile>::iterator itr = projectiles.begin(); itr != projectiles.end(); ) {
    if(itr->second.isLive()) {
      ++itr;
      continue;
    }
    itr->second.collideCleanup(collide, owner, itr->first);
    aid.push_back(itr->first);
    map<int, Projectile>::iterator titr = itr;
    ++itr;
    projectiles.erase(titr);
  }
}

void ProjectilePack::render() const {
  for(map<int, Projectile>::const_iterator itr = projectiles.begin(); itr != projectiles.end(); ++itr)
    itr->second.render();
}

void ProjectilePack::checksum(Adler32 *adl) const {
  adler(adl, projectiles);
  adler(adl, aid);
  adler(adl, newitems);
}
