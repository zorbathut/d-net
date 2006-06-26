
#include "game.h"

#include "ai.h"
#include "args.h"
#include "debug.h"
#include "gfx.h"
#include "player.h"

#include <numeric>

using namespace std;

DEFINE_bool(verboseCollisions, false, "Verbose collisions");
DEFINE_bool(debugGraphics, false, "Enable various debug graphics");
DECLARE_int(rounds_per_store);

void dealDamage(float dmg, Tank *target, Tank *owner, float damagecredit, bool killcredit) {
  if(target->team == owner->team)
    return; // friendly fire exception
  if(target->takeDamage(dmg) && killcredit)
    owner->player->addKill();
  owner->player->addDamage(dmg * damagecredit);
};

void detonateWarhead(const IDBWarheadAdjust &warhead, Coord2 pos, Tank *impact, Tank *owner, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm, float damagecredit, bool killcredit) {
  
  if(impact)
    dealDamage(warhead.impactdamage(), impact, owner, damagecredit, killcredit);
  
  for(int i = 0; i < adjacency.size(); i++) {
    if(adjacency[i].first < warhead.radiusfalloff())
      dealDamage(warhead.radiusdamage() / warhead.radiusfalloff() * ( warhead.radiusfalloff() - adjacency[i].first), adjacency[i].second, owner, damagecredit, killcredit);
  }
  
  GfxEffects ngfe;
  ngfe.point_pos = pos.toFloat();
  ngfe.life = 6;
  ngfe.type = GfxEffects::EFFECT_POINT;
  ngfe.color = Color(1.0, 1.0, 1.0);
  for( int i = 0; i < 6; i++ ) {
    float dir = frand() * 2 * PI;
    ngfe.point_vel = makeAngle(dir) / 3;
    ngfe.point_vel *= 1.0 - frand() * frand();
    gfxe->push_back( ngfe );
  }
  
  if(warhead.radiusfalloff() > 0) {
    GfxEffects dbgf;
    dbgf.type = GfxEffects::EFFECT_CIRCLE;
    dbgf.circle_center = pos.toFloat();
    dbgf.circle_radius = warhead.radiusfalloff();
    dbgf.life = 5;
    gfxe->push_back(dbgf);
  }
  
  if(warhead.wallremovalradius() > 0 && frand() < warhead.wallremovalchance()) {
    gm->removeWalls(pos, warhead.wallremovalradius());
  }

};

void GfxEffects::move() {
  CHECK(life != -1);
  age++;
}
void GfxEffects::render() const {
  CHECK(life != -1);
  float apercent = 1.0f - (float)age / life;
  setColor(color * apercent);
  if(type == EFFECT_LINE) {
    drawLine(line_pos + line_vel * age, 0.1f);
  } else if(type == EFFECT_POINT) {
    drawPoint(point_pos.x + point_vel.x * age, point_pos.y + point_vel.y * age, 0.1f);
  } else if(type == EFFECT_CIRCLE) {
    drawCircle(circle_center, circle_radius, 0.1f);
  } else if(type == EFFECT_TEXT) {
    drawText(text_data, text_size, text_pos + text_vel * age);
  } else if(type == EFFECT_PATH) {
    drawTransformedLinePath(path_path, path_ang_start + path_ang_vel * age + path_ang_acc * age * age / 2, path_pos_start + path_pos_vel * age + path_pos_acc * age * age / 2, 0.1f);
  } else if(type == EFFECT_PING) {
    drawCircle(ping_pos, ping_radius_d * age / 60, ping_thickness_d * age / 60);
  } else {
    CHECK(0);
  }
}
bool GfxEffects::dead() const {
  return age >= life;
}

GfxEffects::GfxEffects() {
  age = 0;
  life = -1;
  color = Color(1.0, 1.0, 1.0);
}

Team::Team() {
  weapons_enabled = true;
  color = Color(0, 0, 0);
  swap_colors = false;
}

void Tank::init(Player *in_player) {
  CHECK(in_player);
  CHECK(!player);
  player = in_player;
  health = player->getTank().maxHealth();
  initted = true;
  framesSinceDamage = -1;
  damageTaken = 0;
}

void Tank::tick(const Keystates &kst) {
  
  pair<Coord2, float> newpos = getDeltaAfterMovement( kst, pos, d );
  
  pos = newpos.first;
  d = newpos.second;
  
  if(framesSinceDamage != -1)
    framesSinceDamage++;
  
};

void Tank::render() const {
  if( !live )
    return;

  vector<Coord2> tankverts = getTankVertices(pos, d);
  vector<Coord2> smtankverts;
  Coord2 centr = getCentroid(tankverts);
  for(int i = 0; i < tankverts.size(); i++)
    smtankverts.push_back((tankverts[i] - centr) * Coord(0.5) + centr);
  
  Color main;
  Color small;
  
  main = player->getFaction()->color;
  small = team->color;
  
  if(team->swap_colors)
    swap(main, small);
  
  small = small * 0.5;
  
  setColor(main);
  drawLineLoop(tankverts, 0.2);
  
  setColor(small);
  drawLineLoop(smtankverts, 0.2);
};

vector<Coord4> Tank::getCurrentCollide() const {
  if( !live )
    return vector<Coord4>();

  vector<Coord2> tankpts = getTankVertices( pos, d );
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

vector<Coord4> Tank::getNextCollide(const Keystates &keys) const {
  if( !live )
    return vector<Coord4>();

  pair<Coord2, float> newpos = getDeltaAfterMovement( keys, pos, d );
  vector<Coord2> tankpts = getTankVertices( newpos.first, newpos.second );
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

void Tank::addCollision(Collider *collider, const Keystates &keys) const {
  
  if( !live )
    return;

  vector<Coord2> tankpts = getTankVertices( pos, d );
  pair<Coord2, float> newpos = getDeltaAfterMovement( keys, pos, d );
  vector<Coord2> newtankpts = getTankVertices( newpos.first, newpos.second );
  for( int i = 0; i < newtankpts.size(); i++ )
    newtankpts[i] -= tankpts[i];
  for( int i = 0; i < 3; i++ )
    collider->token(Coord4(tankpts[i], tankpts[(i + 1) % 3]), Coord4(newtankpts[i], newtankpts[(i + 1) % 3]));
};

const float tank_width = 5;
const float tank_length = tank_width*1.3;

const Coord tank_coords[3][2] =  {
  {Coord(-tank_length / 3), Coord(-tank_width / 2)},
  {Coord(-tank_length / 3), Coord(tank_width / 2)},
  {Coord(tank_length * 2 / 3), Coord(0)}
};

vector<Coord2> Tank::getTankVertices( Coord2 pos, float td ) const {
  Coord2 xt = makeAngle(Coord(td));
  Coord2 yt = makeAngle(Coord(td) - COORDPI / 2);
  vector<Coord2> rv;
  for( int i = 0; i < 3; i++ )
    rv.push_back(Coord2(pos.x + tank_coords[i][ 0 ] * xt.x + tank_coords[i][ 1 ] * xt.y, pos.y + tank_coords[i][ 1 ] * yt.y + tank_coords[i][ 0 ] * yt.x));
  return rv;
};

Coord2 Tank::getFiringPoint() const {
  Coord2 xt = makeAngle(Coord(d));
  Coord2 yt = makeAngle(Coord(d) - COORDPI / 2);
  return Coord2( pos.x + tank_coords[ 2 ][ 0 ] * xt.x + tank_coords[ 2 ][ 1 ] * xt.y, pos.y + tank_coords[ 2 ][ 1 ] * yt.y + tank_coords[ 2 ][ 0 ] * yt.x );
};

pair<Coord2, float> Tank::getDeltaAfterMovement( const Keystates &keys, Coord2 pos, float d ) const {
  
  float dl;
  float dr;
   if(keys.axmode == KSAX_TANK) {
    dl = deadzone(keys.ax[0], keys.ax[1], 0.2, 0);
    dr = deadzone(keys.ax[1], keys.ax[0], 0.2, 0);
   } else if(keys.axmode == KSAX_ABSOLUTE || keys.axmode == KSAX_STEERING) {
    float dd;
    float dv;
    if(keys.axmode == KSAX_ABSOLUTE) {
      float xpd = deadzone(keys.ax[0], keys.ax[1], 0, 0.2);
      float ypd = deadzone(keys.ax[1], keys.ax[0], 0, 0.2);
      if(xpd == 0 && ypd == 0) {
        dv = dd = 0;
      } else {
        float desdir = atan2(-ypd, xpd);
        desdir -= d;
        desdir += 2 * PI;
        if(desdir > PI)
          desdir -= 2 * PI;
        dd = desdir / player->getTank().turnSpeed() / FPS;
        if(dd < -1)
          dd = -1;
        if(dd > 1)
          dd = 1;
        dv = min(sqrt(xpd * xpd + ypd * ypd), 1.f);
        // Various states:
        // abs(desdir) / PI
        // 0 .. 0.333 - drive forwards
        // 0.333 .. 0.666 - do not drive
        // 0.666 .. 1.0 - drive backwards, don't turn
        if(abs(desdir) < PI / 3)
          ;
        else if(abs(desdir) < PI / 3 * 2)
          dv = 0; // if we're near right angles, stop
        else if(abs(desdir) < PI / 10 * 9)
          dv = -dv;   // if we're merely backwards, go backwards
        else {  // if we're straight backwards, don't turn
          dv = -dv;
          dd = 0;
        }
      }
    } else {
      dd = deadzone(keys.ax[0], keys.ax[1], 0.2, 0);
      dv = deadzone(keys.ax[1], keys.ax[0], 0.2, 0);
    }
    // What aspects do we want here?
    // If dv is zero, we turn at full speed.
    // If dd is zero, we move at full speed.
    // For neutral:
    //  1  1
    //  0  0
    // -1 -1
    // For left turn:
    //  0  1
    // -1  1
    // -1  0
    // For right turn:
    //  1  0
    //  1 -1
    //  0 -1
    float d00, d01, d02, d10, d11, d12;
    if(dd <= 0) {
      d00 = 1 + dd;
      d10 = 1;
      d01 = dd;
      d11 = -dd;
      d02 = -1;
      d12 = -1 - dd;
    } else {
      d00 = 1;
      d10 = 1 - dd;
      d01 = dd;
      d11 = -dd;
      d02 = -1 + dd;
      d12 = -1;
    }
    if(dv <= 0) {
      float intens = abs(dv);
      dl = (d02 * intens + d01 * (1 - intens));
      dr = (d12 * intens + d11 * (1 - intens));
    } else {
      float intens = abs(dv);
      dl = (d00 * intens + d01 * (1 - intens));
      dr = (d10 * intens + d11 * (1 - intens));
    }
  }

  float dv = (dr + dl) / 2;
  float dd = (dl - dr) / 2;
  
  // More random thoughts:
  // 1 and 0 should stay 1 and 0
  // 0.5 and 0 should stay 0.5 and 0
  // 0.5 and 0.5 should scale up (to 0.707 both?)
  
  Float2 ult = makeAngle(getAngle(Float2(dv, dd)));
  float dif = 1 / (abs(ult.x) + abs(ult.y));
  dv /= dif;
  dd /= dif;
  
  if(dv > 1) dv = 1;
  if(dd > 1) dd = 1;
  if(dv < -1) dv = -1;
  if(dd < -1) dd = -1;

  CHECK(dv >= -1 && dv <= 1);
  CHECK(dd >= -1 && dd <= 1);
  
  Coord cdv(dv);

  pos += makeAngle(Coord(d)) * Coord(player->getTank().maxSpeed() / FPS) * cdv;

  d += player->getTank().turnSpeed() / FPS * dd;
  d += 2*PI;
  d = fmod( d, 2*(float)PI );
  
  return make_pair( pos, d );
  
}

bool Tank::takeDamage( float damage ) {
  health -= damage;
  damageTaken += damage;
  if(framesSinceDamage == -1)
    framesSinceDamage = 0;
  if( health <= 0 && live ) {
    live = false;
    spawnShards = true;
    return true;
  }
  return false;
};

void Tank::genEffects(vector<GfxEffects> *gfxe, vector<Projectile> *projectiles) {
  if( spawnShards ) {
    vector<Coord2> tv = getTankVertices( pos, d );
    Coord2 centr = getCentroid(tv);
    Coord tva = getArea(tv);
    
    for(int i = 0; i < tv.size(); i++)
      tv[i] -= centr;
    
    const IDBGloryAdjust &glory = player->getGlory();
    
    vector<float> ang;
    {
      int ct = int(frand() * (glory.maxsplits() - glory.minsplits() + 1)) + glory.minsplits();
      CHECK(ct <= glory.maxsplits() && ct >= glory.minsplits());
      for(int i = 0; i < ct; i++)
        ang.push_back(frand() * (glory.maxsplitsize() - glory.minsplitsize()) + glory.minsplitsize());
      for(int i = 1; i < ang.size(); i++)
        ang[i] += ang[i - 1];
      float angtot = accumulate(ang.begin(), ang.end(), 0.0f);
      float shift = frand() * PI * 2;
      for(int i = 0; i < ang.size(); i++) {
        ang[i] *= PI * 2 / angtot;
        ang[i] += shift;
      }
    }
    
    vector<vector<Coord2> > chunks;
    for(int i = 0; i < ang.size(); i++) {
      int j = (i + 1) % ang.size();
      float ned = ang[j];
      if(ned > ang[i])
        ned -= 2 * PI;
      vector<Coord2> intersecty;
      intersecty.push_back(Coord2(0, 0));
      float kang = ang[i];
      CHECK(kang - ned < PI * 2);
      do {
        intersecty.push_back(makeAngle(Coord(kang)) * 100);
        kang -= 0.5;
        if(kang - 0.25 < ned)
          kang = ned;
      } while(kang > ned);
      intersecty.push_back(makeAngle(Coord(ned)) * 100);
      reverse(intersecty.begin(), intersecty.end());
      vector<vector<Coord2> > thischunk = getDifference(tv, intersecty);
      if(thischunk.size() != 1) {
        float tvs = getArea(tv).toFloat();
        dprintf("tvs is %f\n", tvs);
        // Sometimes precision hates us and we end up with a tiny minichunk left over
        for(int i = 0; i < thischunk.size(); i++) {
          dprintf("%f\n", getArea(thischunk[i]).toFloat());
          if(getArea(thischunk[i]) < Coord(0.00001)) {
            thischunk.erase(thischunk.begin() + i);
            i--;
          }
        }
      }
      CHECK(thischunk.size() == 1);
      chunks.push_back(thischunk[0]);
    }
    
    for(int i = 0; i < chunks.size(); i++) {
      Coord2 subcentroid = getCentroid(chunks[i]);
      vector<Float2> vf2;
      for(int j = 0; j < chunks[i].size(); j++)
        vf2.push_back((chunks[i][j] - subcentroid).toFloat());
      Coord2 vel = normalize(subcentroid) / 10 * tva / getArea(chunks[i]);
      GfxEffects ngfe;
      ngfe.type = GfxEffects::EFFECT_PATH;
      ngfe.path_path = vf2;
      ngfe.path_pos_start = (centr + subcentroid).toFloat();
      ngfe.path_pos_vel = vel.toFloat();
      ngfe.path_pos_acc = -ngfe.path_pos_vel / 30;
      ngfe.path_ang_start = 0;
      ngfe.path_ang_vel = gaussian() / 20;
      ngfe.path_ang_acc = -ngfe.path_ang_vel / 30;
      ngfe.life = 30;
      ngfe.color = player->getFaction()->color;
      gfxe->push_back(ngfe);
    }
    
    for(int i = 0; i < ang.size(); i++)
      for(int j = 0; j < glory.shotspersplit(); j++)
        projectiles->push_back(Projectile(centr, ang[i] + gaussian_scaled(2) / 8, glory.projectile(), this));
    
    spawnShards = false;
  }
}

Tank::Tank() {
  pos = Coord2(0, 0);
  d = 0;
  live = true;
  spawnShards = false;
  health = -47283;
  player = NULL;
  initted = false;
  weaponCooldown = 0;
  memset(weaponCooldownSubvals, 0, sizeof(weaponCooldownSubvals)); // if you're not using IEEE floats, get a new computer.
  zone_current = -1;
  zone_frames = 0;
}

void Projectile::tick(vector<GfxEffects> *gfxe) {
  CHECK(live);
  CHECK(age != -1);
  pos += movement();
  lasttail = nexttail();
  age++;
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    if(age > 10)
      missile_sidedist /= 1.2;
    GfxEffects ngfe;
    ngfe.point_pos = pos.toFloat() + lasttail.toFloat() - movement().toFloat(); // projectiles get a free tick ATM
    ngfe.life = 10;
    ngfe.type = GfxEffects::EFFECT_POINT;
    ngfe.color = projtype.color();
    for( int i = 0; i < 2; i++ ) {
      float dir = frand() * 2 * PI;
      ngfe.point_vel = makeAngle(dir) / 3;
      ngfe.point_vel *= 1.0 - frand() * frand();
      ngfe.point_vel += movement().toFloat();
      ngfe.point_vel += missile_accel().toFloat() * -3 * abs(gaussian());
      ngfe.color = Color(1.0, 0.9, 0.6);
      gfxe->push_back(ngfe);
    }
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity *= 0.95;
    if(airbrake_liveness() <= 0)
      live = false;
  } else {
    CHECK(0);
  }
}

void Projectile::render() const {
  CHECK(live);
  CHECK(age != -1);
  if(projtype.motion() == PM_NORMAL) {
    setColor(projtype.color());
  } else if(projtype.motion() == PM_MISSILE) {
    setColor(projtype.color());
  } else if(projtype.motion() == PM_AIRBRAKE) {
    setColor(projtype.color() * airbrake_liveness());
  } else {
    CHECK(0);
  }
  drawLine(Coord4(pos, pos + lasttail), projtype.width());
};
void Projectile::addCollision( Collider *collider ) const {
  CHECK(live);
  collider->token( Coord4( pos, pos + lasttail ), Coord4( movement(), movement() + nexttail() ) );
};
void Projectile::impact(Coord2 pos, Tank *target, const vector<pair<float, Tank *> > &adjacency, vector<GfxEffects> *gfxe, Gamemap *gm) {
  if(!live)
    return;
  
  detonateWarhead(projtype.warhead(), pos, target, owner, adjacency, gfxe, gm, 1.0, true);

  live = false;
};

bool Projectile::isLive() const {
  return live;
}

Coord2 Projectile::movement() const {
  if(projtype.motion() == PM_NORMAL) {
    return makeAngle(Coord(d)) * Coord(projtype.velocity());
  } else if(projtype.motion() == PM_MISSILE) {
    return missile_accel() + missile_backdrop() + missile_sidedrop();
  } else if(projtype.motion() == PM_AIRBRAKE) {
    return Coord2(makeAngle(d) * airbrake_velocity);
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

Projectile::Projectile() : projtype(NULL, NULL) {
  live = false;
  age = -1;
}
Projectile::Projectile(const Coord2 &in_pos, float in_d, const IDBProjectileAdjust &in_projtype, Tank *in_owner) : projtype(in_projtype) {
  pos = in_pos;
  d = in_d;
  owner = in_owner;
  age = 0;
  live = true;
  lasttail = Coord2(0, 0);
  
  if(projtype.motion() == PM_NORMAL) {
  } else if(projtype.motion() == PM_MISSILE) {
    missile_sidedist = gaussian() * 0.25;
  } else if(projtype.motion() == PM_AIRBRAKE) {
    airbrake_velocity = (gaussian_scaled(2) / 4 + 1) * projtype.velocity();
  } else {
    CHECK(0);
  }
}

// returns center and width/height
pair<Float2, Float2> getMapZoom(const Coord4 &mapbounds) {
  Float4 bounds = mapbounds.toFloat();
  pair<Float2, Float2> rv;
  rv.first.x = (bounds.sx + bounds.ex) / 2;
  rv.first.y = (bounds.sy + bounds.ey) / 2;
  rv.second.x = bounds.ex - bounds.sx;
  rv.second.y = bounds.ey - bounds.sy;
  rv.second.x *= 1.1;
  rv.second.y *= 1.1;
  return rv;
}

void goCloser(float *cur, const float *now, const float dist) {
  if(*cur < *now) {
    *cur = min(*cur + dist, *now);
  } else {
    *cur = max(*cur - dist, *now);
  }
}

void doInterp(float *curcenter, const float *nowcenter, float *curzoom, const float *nowzoom, float *curspeed) {
  if(*curcenter != *nowcenter || *curzoom != *nowzoom) {
    *curspeed = min(*curspeed + 0.0000002, 0.001);
    goCloser(curcenter, nowcenter, *nowzoom * *curspeed);
    goCloser(curzoom, nowzoom, *nowzoom * *curspeed);
  } else {
    *curspeed = max(*curspeed - 0.000002, 0.);
  }
}

bool Game::runTick( const vector< Keystates > &rkeys ) {
  StackString sst("Frame runtick");
  
  if(!ffwd && FLAGS_verboseCollisions)
    dprintf("Ticking\n");
  
  frameNm++;
  
  vector< Keystates > keys = rkeys;
  if(frameNm < frameNmToStart && freezeUntilStart) {
    for(int i = 0; i < keys.size(); i++) {
      if(keys[i].accept.push || keys[i].fire[0].push) {
        GfxEffects ngfe;
        ngfe.type = GfxEffects::EFFECT_PING;
        ngfe.ping_pos = tanks[i].pos.toFloat();
        ngfe.ping_radius_d = 200;
        ngfe.ping_thickness_d = 8;
        ngfe.life = 30;
        ngfe.color = tanks[i].player->getFaction()->color;
        gfxeffects.push_back(ngfe);
      }
      keys[i].nullMove();
      for(int j = 0; j < SIMUL_WEAPONS; j++)
        keys[i].fire[j] = Button();
    }
  }
  
  if(gamemode == GMODE_DEMO) {
    // EVERYONE IS INVINCIBLE
    for(int i = 0; i < tanks.size(); i++) {
      tanks[i].health = 1000000000;
    }
  }
  
  // first we deal with moving the tanks around
  // we shuffle the player order randomly, then move tanks in that order
  // if the player can't move where they want to, they simply don't move
  // I am assuming there will be no tanks that can move further than their entire length per frame.
  
  // Second, we feed the player start/end positions into the old collision system, along with all the projectiles
  // And then we do our collision system solely on projectile/* intersections.
  
  // I think this works.
  
  const Coord4 gmb = gamemap.getBounds();
  vector<int> teamids;
  {
    map<Team*, int> fez;
    for(int i = 0; i < tanks.size(); i++) {
      if(!fez.count(tanks[i].team))
        fez[tanks[i].team] = fez.size();
      teamids.push_back(fez[tanks[i].team]);
    }
  }
  
  {
    StackString sst("Player movement collider");
    
    collider.resetNonwalls(COM_PLAYER, gamemap.getBounds(), teamids);
    
    {
      StackString sst("Adding walls");
    
      gamemap.updateCollide(&collider);
    }
    
    for(int j = 0; j < tanks.size(); j++) {
      if(!tanks[j].live)
        continue;
      StackString sst(StringPrintf("Adding player %d, status live %d", j, tanks[j].live));
      //CHECK(inPath(tanks[j].pos, gamemap.getCollide()[0]));
      if(!isInside(gmb, tanks[j].pos)) {
        StackString sst("Critical error, running tests");
        dprintf("%s vs %s\n", tanks[j].pos.rawstr().c_str(), gmb.rawstr().c_str());
        gamemap.checkConsistency();
        CHECK(0);
      }
      collider.addThingsToGroup(CGR_PLAYER, j);
      collider.startToken(0);
      tanks[j].addCollision(&collider, keys[j]);
      collider.endAddThingsToGroup();
    }
    
    collider.processSimple();
    
    CHECK(!collider.next());
    
    collider.finishProcess();
    
    vector<int> playerorder;
    {
      vector<int> tanksleft;
      for(int i = 0; i < tanks.size(); i++)
        if(tanks[i].live)
          tanksleft.push_back(i);
      // TODO: turn this into random-shuffle using my deterministic seed
      while(tanksleft.size()) {
        int pt = int(frand() * tanksleft.size());
        CHECK(pt >= 0 && pt < tanksleft.size());
        playerorder.push_back(tanksleft[pt]);
        tanksleft.erase(tanksleft.begin() + pt);
      }
    }
    
    for(int i = 0; i < playerorder.size(); i++) {
      
      CHECK(count(playerorder.begin(), playerorder.end(), playerorder[i]) == 1);
      CHECK(playerorder[i] >= 0 && playerorder[i] < tanks.size());
      
      vector<Coord4> newpos = tanks[playerorder[i]].getNextCollide(keys[playerorder[i]]);
      
      if(collider.checkSimpleCollision(CGR_PLAYER, playerorder[i], newpos)) {
        keys[playerorder[i]].nullMove();
      } else {
        StackString sst(StringPrintf("Moving player %d, status live %d", playerorder[i], tanks[playerorder[i]].live));
        //CHECK(inPath(tanks[playerorder[i]].getDeltaAfterMovement(keys[playerorder[i]], tanks[playerorder[i]].pos, tanks[playerorder[i]].d).first, gamemap.getCollide()[0]));
        CHECK(isInside(gmb, tanks[playerorder[i]].getDeltaAfterMovement(keys[playerorder[i]], tanks[playerorder[i]].pos, tanks[playerorder[i]].d).first));
        collider.clearGroup(CGR_PLAYER, playerorder[i]);
        collider.addThingsToGroup(CGR_PLAYER, playerorder[i]);
        collider.startToken(0);
        for(int j = 0; j < newpos.size(); j++)
          collider.token(newpos[j], Coord4(0, 0, 0, 0));
        collider.endAddThingsToGroup();
      }

    }
    
  }
  
  {
    StackString sst("Main collider");
    
    collider.resetNonwalls(COM_PROJECTILE, gmb, teamids);
    
    // stuff!
    /*
    {
      string ope;
      for(int i = 0; i < stopped.size(); i++) {
        CHECK(stopped[i] + notstopped[i] == 1);
        ope += stopped[i] + '0';
      }
      dprintf("%s\n", ope.c_str());
    }
    */
    
    gamemap.updateCollide(&collider);
    
    for(int j = 0; j < tanks.size(); j++) {
      collider.addThingsToGroup(CGR_PLAYER, j);
      collider.startToken(0);
      tanks[j].addCollision(&collider, keys[j]);
      collider.endAddThingsToGroup();
    }
  
    for( int j = 0; j < projectiles.size(); j++ ) {
      collider.addThingsToGroup(CGR_PROJECTILE, j);
      for( int k = 0; k < projectiles[ j ].size(); k++ ) {
        collider.startToken(k);
        projectiles[ j ][ k ].addCollision( &collider );
      }
      collider.endAddThingsToGroup();
    }
    
    collider.processMotion();
    
    while( collider.next() ) {
      //dprintf( "Collision!\n" );
      //dprintf( "Timestamp %f\n", collider.getCurrentTimestamp().toFloat() );
      //dprintf( "%d,%d,%d vs %d,%d,%d\n", collider.getLhs().first.first, collider.getLhs().first.second, collider.getLhs().second, collider.getRhs().first.first, collider.getRhs().first.second, collider.getRhs().second );
      CollideId lhs = collider.getData().lhs;
      CollideId rhs = collider.getData().rhs;
      if( lhs > rhs ) swap( lhs, rhs );
      if( lhs.category == CGR_WALL && rhs.category == CGR_WALL ) {
        // wall-wall collision, wtf?
        CHECK(0);
      } else if( lhs.category == CGR_WALL && rhs.category == CGR_PLAYER ) {
        // wall-tank collision, should never happen
        CHECK(0);
      } else if( lhs.category == CGR_WALL && rhs.category == CGR_PROJECTILE ) {
        // wall-projectile collision - kill projectile
        projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
      } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PLAYER ) {
        // tank-tank collision, should never happen
        CHECK(0);
      } else if( lhs.category == CGR_PLAYER && rhs.category == CGR_PROJECTILE ) {
        // tank-projectile collision - kill projectile, do damage
        projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, &tanks[ lhs.bucket ], genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
      } else if( lhs.category == CGR_PROJECTILE && rhs.category == CGR_PROJECTILE ) {
        // projectile-projectile collision - kill both projectiles
        // also do radius damage, and do it fairly dammit
        bool lft = frand() < 0.5;
        
        if(lft)
          projectiles[ lhs.bucket ][ lhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
        
        projectiles[ rhs.bucket ][ rhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
        
        if(!lft)
          projectiles[ lhs.bucket ][ lhs.item ].impact(collider.getData().loc, NULL, genTankDistance(collider.getData().loc), &gfxeffects, &gamemap);
        
      } else {
        // nothing meaningful, should totally never happen, what the hell is going on here, who are you, and why are you in my apartment
        CHECK(0);
      }
    }
    
    collider.finishProcess();
  }

  {
    vector<vector<Projectile> > newProjectiles(projectiles.size());
    for(int j = 0; j < projectiles.size(); j++) {
      for(int k = 0; k < projectiles[ j ].size(); k++) {
        if(projectiles[j][k].isLive()) {
          projectiles[j][k].tick(&gfxeffects);
          if(!projectiles[j][k].isLive())   // in case it dies in its tick
            continue;
          newProjectiles[j].push_back(projectiles[j][k]);
        }
      }
    }
    projectiles.swap(newProjectiles);
  }
  
  for(int j = 0; j < bombards.size(); j++) {
    CHECK(bombards[j].state >= 0 && bombards[j].state < BombardmentState::BS_LAST);
    if(bombards[j].state == BombardmentState::BS_OFF) {
      if(!tanks[j].live) {
        // if the player is dead and the bombard isn't initialized
        bombards[j].loc = tanks[j].pos;
        bombards[j].state = BombardmentState::BS_SPAWNING;
        bombards[j].timer = 60 * 6;
      }
    } else if(bombards[j].state == BombardmentState::BS_SPAWNING) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0)
        bombards[j].state = BombardmentState::BS_ACTIVE;
    } else if(bombards[j].state == BombardmentState::BS_ACTIVE) {
      bombards[j].loc.x += Coord(deadzone(keys[j].udlrax[0], keys[j].udlrax[1], 0, 0.2));
      bombards[j].loc.y += Coord(-deadzone(keys[j].udlrax[1], keys[j].udlrax[0], 0, 0.2));
      bombards[j].loc.x = max(bombards[j].loc.x, gmb.sx);
      bombards[j].loc.y = max(bombards[j].loc.y, gmb.sy);
      bombards[j].loc.x = min(bombards[j].loc.x, gmb.ex);
      bombards[j].loc.y = min(bombards[j].loc.y, gmb.ey);
      CHECK(SIMUL_WEAPONS == 2);
      if(keys[j].fire[0].down || keys[j].fire[1].down) {
        bombards[j].state = BombardmentState::BS_FIRING;
        bombards[j].timer = tanks[j].player->getBombardment().lockdelay();
      }
    } else if(bombards[j].state == BombardmentState::BS_FIRING) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0) {
        detonateWarhead(tanks[j].player->getBombardment().warhead(), bombards[j].loc, NULL, &tanks[j], genTankDistance(bombards[j].loc), &gfxeffects, &gamemap, 1.0, false);
        bombards[j].state = BombardmentState::BS_COOLDOWN;
        bombards[j].timer = tanks[j].player->getBombardment().unlockdelay();
      }
    } else if(bombards[j].state == BombardmentState::BS_COOLDOWN) {
      bombards[j].timer--;
      if(bombards[j].timer <= 0)
        bombards[j].state = BombardmentState::BS_ACTIVE;
    } else {
      CHECK(0);
    }
  }  
  
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].tick(keys[i]);

  for(int i = 0; i < tanks.size(); i++) {
    StackString sst(StringPrintf("Player weaponry %d", i));
    if(tanks[i].live) {
      for(int j = 0; j < SIMUL_WEAPONS; j++) {
        if(keys[i].change[j].push) {
          StackString sst(StringPrintf("Switching"));
          tanks[i].player->cycleWeapon(j);
          addTankStatusText(i, tanks[i].player->getWeapon(j).name(), 2);
        }
      }
    
      if(tanks[i].weaponCooldown <= 0 && tanks[i].team->weapons_enabled && frameNm >= frameNmToStart && frameNmToStart != -1) {
        StackString sst(StringPrintf("Firetesting"));
        // The player can fire, so let's find out if he does
        
        // weaponCooldownSubvals is maintained in here.
        // Every "fire" attempt, we find the weapon with the lowest subval. We subtract that from all active weapons (thereby making that value 0),
        // then we add the seconds-until-next-shot to that one. Any non-active weapons are clamped to 0 on the theory that the player is hammering
        // that button and really wants it to fire.
        
        float rlev = 1e20; // uh, no
        int curfire = -1;
        for(int j = 0; j < SIMUL_WEAPONS; j++) {
          if(keys[i].fire[j].down) {
            if(rlev > tanks[i].weaponCooldownSubvals[j]) {
              rlev = tanks[i].weaponCooldownSubvals[j];
              curfire = j;
            }
          } else {
            tanks[i].weaponCooldownSubvals[j] = 0;
          }
        }
        CHECK(rlev >= 0);
        
        if(curfire != -1) {
          // We're firing something!
          
          for(int j = 0; j < SIMUL_WEAPONS; j++) {
            tanks[i].weaponCooldownSubvals[j] = max(tanks[i].weaponCooldownSubvals[j] - rlev, (float)0);
          }
          
          tanks[i].weaponCooldownSubvals[curfire] = FPS / tanks[i].player->getWeapon(curfire).firerate();
          
          // Blam!
          IDBWeaponAdjust weapon = tanks[i].player->getWeapon(curfire);
          
          projectiles[i].push_back(Projectile(tanks[i].getFiringPoint(), tanks[i].d + weapon.deploy().anglestddev() * gaussian(), weapon.projectile(), &tanks[i]));
          tanks[i].weaponCooldown = weapon.framesForCooldown();
          // hack here to detect weapon out-of-ammo
          string lastname = weapon.name();
          firepowerSpent += tanks[i].player->shotFired(curfire);
          if(weapon.name() != lastname) {
            addTankStatusText(i, weapon.name(), 2);
          }
          
          {
            string slv = StringPrintf("%d", tanks[i].player->shotsLeft(curfire));
            if(count(slv.begin(), slv.end(), '0') == slv.size() - 1)
              addTankStatusText(i, slv, 0.5);
          }
        }
      }
    }
  }

  {
    vector< GfxEffects > neffects;
    for( int i = 0; i < gfxeffects.size(); i++ ) {
      gfxeffects[i].move();
      if( !gfxeffects[i].dead() )
        neffects.push_back( gfxeffects[i] );
    }
    swap( neffects, gfxeffects );
  }

  for( int i = 0; i < tanks.size(); i++ ) {
    tanks[i].weaponCooldown--;
    tanks[i].genEffects(&gfxeffects, &projectiles[i]);
    if(tanks[i].live) {
      int inzone = -1;
      for(int j = 0; j < zones.size(); j++)
        if(inPath(tanks[i].pos, zones[j].first))
          inzone = j;
      if(tanks[i].zone_current != inzone) {
        tanks[i].zone_current = inzone;
        tanks[i].zone_frames = 0;
      }
      tanks[i].zone_frames++;
    }
  }
  
  // This is a bit ugly - this only happens in choice mode
  if(zones.size() == 4) {
    for(int i = 0; i < tanks.size(); i++) {
      if(tanks[i].zone_current != -1 && (tanks[i].zone_frames - 1) % 60 == 0 && tanks[i].team == &teams[4]) {
        int secleft = (181 - tanks[i].zone_frames) / 60;
        if(secleft) {
          addTankStatusText(i, StringPrintf("%d second%s to join team", secleft, (secleft == 1 ? "" : "s")), 1);
        } else {
          addTankStatusText(i, StringPrintf("team joined"), 1);
        }
      }
      if(tanks[i].zone_current != -1 && tanks[i].zone_frames > 180 && tanks[i].team == &teams[4]) {
        tanks[i].team = &teams[tanks[i].zone_current];
        if(frameNmToStart == -1)
          frameNmToStart = frameNm + 60 * 6;
      }
    }
  }
  
  #if 0 // This hideous hack produces pretty yet deadly fireworks
  {   
    static Tank boomy;
    static Player boomyplay;
    static FactionState boomyfact;
    boomy.spawnShards = true;
    boomy.player = &boomyplay;
    boomyplay.glory = defaultGlory();
    float border = 40;
    boomy.pos.x = Coord(frand()) * (gmb.x_span() - border * 2) + gmb.sx + border;
    boomy.pos.y = Coord(frand()) * (gmb.y_span() - border * 2) + gmb.sy + border;
    boomy.d = 0;
    boomyplay.faction = &boomyfact;
    boomyfact.color = Color(1.0, 1.0, 1.0);
    boomy.genEffects(&gfxeffects, &projectiles[0]);
  }
  #endif
  
  {
    set<Team *> liveteams;
    for(int i = 0; i < tanks.size(); i++) {
      if(tanks[i].live)
        liveteams.insert(tanks[i].team);
    }
    if(liveteams.size() <= 1) {
      if(zones.size() != 4 || liveteams.size() != 1 || liveteams.count(&teams[4]) == 0)   // nasty hackery for choice mode
        framesSinceOneLeft++;
    }
  }

  {
    pair<Float2, Float2> z = getMapZoom(gamemap.getBounds());
    
    doInterp(&zoom_center.x, &z.first.x, &zoom_size.x, &z.second.x, &zoom_speed.x);
    doInterp(&zoom_center.y, &z.first.y, &zoom_size.y, &z.second.y, &zoom_speed.y);
  }

  if(framesSinceOneLeft / FPS >= 3 && gamemode != GMODE_TEST && gamemode != GMODE_DEMO) {
    if(zones.size() == 0) {
      int winplayer = -1;
      for(int i = 0; i < tanks.size(); i++) {
        if(tanks[i].live) {
          tanks[i].player->addWin();
          CHECK(winplayer == -1);
          winplayer = i;
        }
      }  
      if(winplayer == -1)
        wins->push_back(NULL);
      else
        wins->push_back(tanks[winplayer].player->getFaction());
    } else if(zones.size() == 4) {
    } else {
      CHECK(0);
    }
    return true;
  } else {
    return false;
  }

};

void Game::ai(const vector<Ai *> &ais) const {
  CHECK(ais.size() == tanks.size());
  for(int i = 0; i < ais.size(); i++) {
    if(ais[i]) {
      if(tanks[i].live)
        ais[i]->updateGame(tanks, i);
      else
        ais[i]->updateBombardment(tanks, bombards[i].loc);
    }
  }
}

void drawCirclePieces(const Coord2 &cloc, float solidity, float rad) {
  Float2 loc = cloc.toFloat();
  for(int i = 0; i < 3; i++) {
    float core = PI / 2 * 3 + (PI * 2 / 3 * i);
    float span = PI * 2 / 6 + PI * 2 / 6 * solidity;
    vector<Float2> path;
    for(int k = 0; k <= 10; k++)
      path.push_back(makeAngle(core - span / 2 + span / 10 * k) * rad + loc);
    drawLinePath(path, 0.1);
  }
}

void drawCrosses(const Coord2 &cloc, float rad) {
  Float2 loc = cloc.toFloat();
  for(int i = 0; i < 3; i++) {
    float core = PI / 2 * 3 + (PI * 2 / 3 * i);
    drawLine(makeAngle(core) * rad * 1.25 + loc, makeAngle(core) * rad * 0.75 + loc, 0.1);
  }
}

void Game::renderToScreen() const {

  // Set up zooming for everything that happens in gamespace
  {
    const float availScreen = ((gamemode == GMODE_TEST || gamemode == GMODE_DEMO)? 1.0 : 0.9);
    float pzoom = max(zoom_size.y / availScreen, zoom_size.x / getAspect());
    Float2 origin(zoom_center.x - pzoom * getAspect() / 2, zoom_center.y - pzoom * (1.0 - availScreen / 2));
    setZoom(origin.x, origin.y, origin.y + pzoom);
  }
  
  // Tanks
  for( int i = 0; i < tanks.size(); i++ ) {
    tanks[i].render();
  }
  
  // Projectiles, graphics effects, and bombardments
  for( int i = 0; i < projectiles.size(); i++ )
    for( int j = 0; j < projectiles[i].size(); j++ )
      projectiles[i][ j ].render();
  for( int i = 0; i < gfxeffects.size(); i++ )
    gfxeffects[i].render();
  for(int i = 0; i < bombards.size(); i++) {
    if(bombards[i].state == BombardmentState::BS_OFF) {
    } else if(bombards[i].state == BombardmentState::BS_SPAWNING) {
    } else if(bombards[i].state == BombardmentState::BS_ACTIVE) {
      setColor(tanks[i].player->getFaction()->color * 0.5);
      drawCirclePieces(bombards[i].loc, 0.3, 4);
      drawCrosses(bombards[i].loc, 4);
    } else if(bombards[i].state == BombardmentState::BS_FIRING) {
      setColor(tanks[i].player->getFaction()->color * 0.25);
      drawCirclePieces(bombards[i].loc, 0.3, 4);
      drawCrosses(bombards[i].loc, 4);
      setColor(Color(1.0, 1.0, 1.0));
      float ps = (float)bombards[i].timer / tanks[i].player->getBombardment().lockdelay();
      drawCirclePieces(bombards[i].loc, 1 - ps, 4 * ps);
    } else if(bombards[i].state == BombardmentState::BS_COOLDOWN) {
      setColor(tanks[i].player->getFaction()->color * 0.25);
      drawCirclePieces(bombards[i].loc, 0.3, 4);
      drawCrosses(bombards[i].loc, 4);
      float ps = (float)bombards[i].timer / tanks[i].player->getBombardment().unlockdelay();
      drawCirclePieces(bombards[i].loc, ps, 4 * (1 - ps));
    } else {
      CHECK(0);
    }
  }
  
  // Game map and collider, if we're drawing one
  gamemap.render();
  collider.render();
  
  // This is where we draw the zones
  for(int i = 0; i < zones.size(); i++) {
    setColor(zones[i].second * 0.3);
    drawLineLoop(zones[i].first, 1.0);
  }
  
  // Here's the text for choice mode
  if(gamemode == GMODE_CHOICE) {
    vector<vector<string> > zonenames;
    {
      vector<string> foo;
      
      foo.push_back("No Bonuses");
      foo.push_back("No Penalties");
      zonenames.push_back(foo);
      foo.clear();
      
      foo.push_back("Small Bonuses");
      foo.push_back("No Penalties");
      zonenames.push_back(foo);
      foo.clear();
      
      foo.push_back("Medium Bonuses");
      foo.push_back("Small Penalties");
      zonenames.push_back(foo);
      foo.clear();
      
      foo.push_back("Large Bonuses");
      foo.push_back("Medium Penalties");
      zonenames.push_back(foo);
      foo.clear();
    }
      
    for(int i = 0; i < zones.size(); i++) {
      setColor(zones[i].second);
      Float2 pos = getCentroid(zones[i].first).toFloat();
      pos.x *= 2;
      pos.y *= 1.4;
      drawJustifiedMultiText(zonenames[i], 10, 2, pos, TEXT_CENTER, TEXT_CENTER);
    }
  }
  
  // Here is the DPS numbers
  if(gamemode == GMODE_DEMO) {
    setColor(1.0, 1.0, 1.0);
    for(int i = 0; i < tanks.size(); i++) {
      if(tanks[i].framesSinceDamage > 0) {
        drawJustifiedText(StringPrintf("%.2f DPS", tanks[i].damageTaken / tanks[i].framesSinceDamage * FPS), 10, tanks[i].pos.x.toFloat() - 5, tanks[i].pos.y.toFloat() - 5, TEXT_MAX, TEXT_MAX);
      }
    }
  }
  
  // Here's everything outside gamespace
  if(gamemode != GMODE_TEST && gamemode != GMODE_DEMO) {
    setZoom( 0, 0, 100 );
    
    // Player health
    {
      setColor(1.0, 1.0, 1.0);
      drawLine(Float4(0, 10, (400./3.), 10), 0.1);
      for(int i = 0; i < tanks.size(); i++) {
        setColor(1.0, 1.0, 1.0);
        float loffset = (400./3.) / tanks.size() * i;
        float roffset = (400./3.) / tanks.size() * ( i + 1 );
        if(i)
          drawLine(Float4(loffset, 0, loffset, 10), 0.1);
        if(tanks[i].live) {
          setColor(tanks[i].player->getFaction()->color);
          float barl = loffset + 1;
          float bare = (roffset - 1) - (loffset + 1);
          bare /= tanks[i].player->getTank().maxHealth();
          bare *= tanks[i].health;
          drawShadedRect(Float4(barl, 2, barl + bare, 7), 0.1, 2);
          
          string ammotext[SIMUL_WEAPONS];
          for(int j = 0; j < SIMUL_WEAPONS; j++) {
            if(tanks[i].player->shotsLeft(j) == UNLIMITED_AMMO) {
              ammotext[j] = "inf";
            } else {
              ammotext[j] = StringPrintf("%d", tanks[i].player->shotsLeft(j));
            }
          }
          
          CHECK(SIMUL_WEAPONS == 2);
          drawJustifiedText(ammotext[0], 1.5, loffset + 1, 7.75, TEXT_MIN, TEXT_MIN);
          drawJustifiedText(ammotext[1], 1.5, roffset - 1, 7.75, TEXT_MAX, TEXT_MIN);
        }
      }
    }
    
    // The giant overlay text for countdowns
    if(frameNmToStart == -1) {
      setColor(1.0, 1.0, 1.0);
      drawJustifiedText("Choose team", 8, 133.3 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
    } else if(frameNm < frameNmToStart) {
      setColor(1.0, 1.0, 1.0);
      int fleft = frameNmToStart - frameNm;
      int s;
      if(frameNm % 60 < 5) {
        s = 15;
      } else if(frameNm % 30 < 5) {
        s = 12;
      } else {
        s = 8;
      }
      drawJustifiedText(StringPrintf("Ready %d.%02d", fleft / 60, fleft % 60), s, 133.3 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
    } else if(frameNm < frameNmToStart + 60) {
      float dens = (240.0 - frameNm) / 60;
      setColor(dens, dens, dens);
      drawJustifiedText("GO", 40, 133.3 / 2, 100.0 / 2, TEXT_CENTER, TEXT_CENTER);
    }
    
    // Our win ticker
    if(wins) {
      /*
      vector<const IDBFaction *> genExampleFacts(const vector<Tank> &plays, int ct);
      static vector<const IDBFaction *> fact = genExampleFacts(tanks, 5000);
      wins->swap(fact);*/
      
      setZoom(0, 0, 1);
      
      const float iconwidth = 0.02;
      const float iconborder = 0.001;
      const float comboborder=0.0015;
      const float lineborder = iconborder * 2;
      const float lineextra = 0.005;
      
      Float2 spos(0.01, 0.11);
      
      for(int i = 0; i < wins->size(); i += 6) {
        map<const IDBFaction *, int> fc;
        int smax = 0;
        for(int j = 0; j < i; j++) {
          fc[(*wins)[j]]++;
          smax = max(smax, fc[(*wins)[j]]);
        }
        fc.erase(NULL);
        
        int winrup = wins->size() / 6 * 6 + 6;
        
        float width = 0.02;
        if(fc.size())
          width += iconwidth * fc.size() + lineborder * 2;
        width += iconwidth * (winrup - i) + lineborder * 2 * ((winrup - i) / 6);
        
        if(width >= 1.33)
          continue;
        
        float hei = min(iconwidth, iconwidth * 6 / smax);
        
        if(i) {
          for(map<const IDBFaction *, int>::iterator itr = fc.begin(); itr != fc.end(); itr++) {
            setColor(itr->first->color);
            for(int j = 0; j < itr->second; j++)
              drawDvec2(itr->first->icon, Float4(spos.x + comboborder, spos.y + comboborder + hei * j, spos.x + iconwidth - comboborder, spos.y + iconwidth - comboborder + hei * j), 10, 0.0002);
            float linehei = spos.y + hei * (itr->second - 1) + iconwidth;
            drawLine(spos.x + iconborder + iconborder, linehei, spos.x + iconwidth - iconborder - iconborder, linehei, 0.0002);
            spos.x += iconwidth;
          }
          spos.x += lineborder;
          drawLine(spos.x, spos.y - lineextra, spos.x, spos.y + iconwidth + lineextra, 0.0002);
          spos.x += lineborder;
        }
            
        for(int j = i; j < wins->size(); j++) {
          if((*wins)[j]) {
            setColor((*wins)[j]->color);
            drawDvec2((*wins)[j]->icon, Float4(spos.x + iconborder, spos.y + iconborder, spos.x + iconwidth - iconborder, spos.y + iconwidth - iconborder), 10, 0.0002);
          } else {
            setColor(Color(0.5, 0.5, 0.5));
            drawLine(Float4(spos.x + iconwidth - iconborder * 2, spos.y + iconborder * 2, spos.x + iconborder * 2, spos.y + iconwidth - iconborder * 2), 0.0002);
          }
          spos.x += iconwidth;
          if(j % FLAGS_rounds_per_store == FLAGS_rounds_per_store - 1) {
            setColor(Color(1.0, 1.0, 1.0));
            spos.x += lineborder;
            drawLine(spos.x, spos.y - lineextra, spos.x, spos.y + iconwidth + lineextra, 0.0002);
            spos.x += lineborder;
          }
        }
        
        break;
      }
      
      //wins->swap(fact);
    }
  }
  
};

/*
vector<const IDBFaction *> genExampleFacts(const vector<Tank> &plays, int ct) {
  vector<const IDBFaction *> feet;
  for(int i = 0; i < ct; i++) {
    int rv = rand() % (plays.size() + 1);
    if(rv == plays.size()) {
      if(rand() % 3 == 0) {
        feet.push_back(NULL);
      } else {
        i--;
        continue;
      }
    } else
      feet.push_back(plays[rv].player->getFaction());
  }
  return feet;
}*/

int Game::winningTeam() const {
  Team *winteam = NULL;
  for(int i = 0; i < tanks.size(); i++) {
    if(tanks[i].live) {
      CHECK(winteam == NULL || winteam == tanks[i].team);
      winteam = tanks[i].team;
    }
  }
  if(!winteam)
    return -1;
  return winteam - &teams[0];
}

vector<int> Game::teamBreakdown() const {
  vector<int> teamsize(teams.size());
  for(int i = 0; i < tanks.size(); i++)
    teamsize[tanks[i].team - &teams[0]]++;
  return teamsize;
}

int Game::frameCount() const {
  return frameNm;
}

Game::Game() {
  gamemode = GMODE_LAST;
}

void Game::initCommon(const vector<Player*> &in_playerdata, const Level &lev) {
  CHECK(gamemode >= 0 && gamemode < GMODE_LAST);
  
  tanks.clear();
  bombards.clear();
  zones.clear();
  teams.clear();
  projectiles.clear();
  gfxeffects.clear();
  
  tanks.resize(in_playerdata.size());
  bombards.resize(in_playerdata.size());
  
  wins = NULL;
  
  gamemap = Gamemap(lev);
  
  for(int i = 0; i < tanks.size(); i++) {
    CHECK(in_playerdata[i]);
    tanks[i].init(in_playerdata[i]);
  }
  {
    // place tanks
    CHECK(lev.playerStarts.count(tanks.size()));
    vector<pair<Coord2, float> > pstart = lev.playerStarts.find(tanks.size())->second;
    for(int i = 0; i < tanks.size(); i++) {
      int loc = int(frand() * pstart.size());
      CHECK(loc >= 0 && loc < pstart.size());
      tanks[i].pos = Coord2(pstart[loc].first);
      tanks[i].d = pstart[loc].second;
      pstart.erase(pstart.begin() + loc);
    }
  }

  frameNm = 0;
  framesSinceOneLeft = 0;
  firepowerSpent = 0;
  
  projectiles.resize(in_playerdata.size());
  
  pair<Float2, Float2> z = getMapZoom(gamemap.getBounds());
  zoom_center = z.first;
  zoom_size = z.second;
  
  zoom_speed = Float2(0, 0);

  collider = Collider(tanks.size());
  
  teams.resize(tanks.size());
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].team = &teams[i];
  
  frameNmToStart = -1000;
  freezeUntilStart = false;
}

void Game::initStandard(vector<Player> *in_playerdata, const Level &lev, vector<const IDBFaction *> *in_wins) {
  gamemode = GMODE_STANDARD;
  
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++)
    playerdata.push_back(&(*in_playerdata)[i]);
  initCommon(playerdata, lev);
  
  CHECK(in_wins);
  wins = in_wins;
  
  frameNmToStart = 180;
  freezeUntilStart = true;
};

void Game::initChoice(vector<Player> *in_playerdata) {
  gamemode = GMODE_CHOICE;
  
  Level lev = loadLevel("data/levels_special/choice_4.dv2");
  lev.playerStarts.clear();
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++) {
    float ang = PI * 2 * i / in_playerdata->size();
    lev.playerStarts[in_playerdata->size()].push_back(make_pair(makeAngle(Coord(ang)) * 20, ang));
    playerdata.push_back(&(*in_playerdata)[i]);
  }
  
  initCommon(playerdata, lev);
  
  teams.resize(5);
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].team = &teams[4];
  
  vector<vector<Coord2> > paths;
  {
    const float dist = 160;
    vector<Coord2> cut;
    cut.push_back(Coord2(0, dist));
    cut.push_back(Coord2(-dist, 0));
    cut.push_back(Coord2(0, -dist));
    cut.push_back(Coord2(dist, 0));
    vector<Coord2> lpi = lev.paths[0];
    reverse(lpi.begin(), lpi.end());
    paths = getDifference(lpi, cut);
  }
  
  Color zonecol[4] = {Color(0.6, 0.6, 0.6), Color(0.3, 0.3, 1.0), Color(0, 1.0, 0), Color(1.0, 0, 0)};
  CHECK(paths.size() == (sizeof(zonecol) / sizeof(*zonecol)));
  for(int i = 0; i < paths.size(); i++) {
    zones.push_back(make_pair(paths[i], zonecol[i]));
    teams[i].color = zonecol[i];
    teams[i].swap_colors = true;
  }
  
  teams[4].weapons_enabled = false;
  
  frameNmToStart = -1;
  freezeUntilStart = false;
}

void Game::initTest(Player *in_playerdata, const Float4 &bounds) {
  gamemode = GMODE_TEST;
  
  Level lev;
  
  {
    vector<Coord2> path;
    path.push_back(Coord2(bounds.sx, bounds.sy));
    path.push_back(Coord2(bounds.sx, bounds.ey));
    path.push_back(Coord2(bounds.ex, bounds.ey));
    path.push_back(Coord2(bounds.ex, bounds.sy));
    lev.paths.push_back(path);
  }
  
  {
    lev.playersValid.insert(1);
    lev.playerStarts[1].push_back(make_pair(bounds.midpoint(), PI / 2 * 3));
  }
  
  vector<Player*> playerdata;
  playerdata.push_back(in_playerdata);
  initCommon(playerdata, lev);
}

void Game::initDemo(vector<Player> *in_playerdata, float boxradi, const float *xps, const float *yps) {
  gamemode = GMODE_DEMO;
  
  Level lev;
  
  {
    vector<Coord2> path;
    path.push_back(Coord2(-boxradi, -boxradi));
    path.push_back(Coord2(-boxradi, boxradi));
    path.push_back(Coord2(boxradi, boxradi));
    path.push_back(Coord2(boxradi, -boxradi));
    lev.paths.push_back(path);
  }
  
  {
    lev.playersValid.insert(in_playerdata->size());
    for(int i = 0; i < in_playerdata->size(); i++)
      lev.playerStarts[in_playerdata->size()].push_back(make_pair(Coord2(0, 0), PI / 2 * 3));
    // these get pretty much ignored anyway
  }
  
  vector<Player*> playerdata;
  for(int i = 0; i < in_playerdata->size(); i++)
    playerdata.push_back(&(*in_playerdata)[i]);
  
  initCommon(playerdata, lev);
  
  for(int i = 0; i < tanks.size(); i++)
    tanks[i].pos = Coord2(xps[i], yps[i]);
}

vector<pair<float, Tank *> > Game::genTankDistance(const Coord2 &center) {
  vector<pair<float, Tank *> > rv;
  for(int i = 0; i < tanks.size(); i++) {
    if(tanks[i].live) {
      vector<Coord2> tv = tanks[i].getTankVertices(tanks[i].pos, tanks[i].d);
      if(inPath(center, tv)) {
        rv.push_back(make_pair(0, &tanks[i]));
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
      rv.push_back(make_pair(closest, &tanks[i]));
    }
  }
  return rv;
}

void Game::addTankStatusText(int tankid, const string &text, float duration) {
  GfxEffects nge;
  nge.type = GfxEffects::EFFECT_TEXT;
  nge.life = int(duration * 60);
  nge.text_pos = tanks[tankid].pos.toFloat() + Float2(4, -4);
  nge.text_vel = Float2(0, -0.1);
  nge.text_size = 2.5;
  nge.text_data = text;
  gfxeffects.push_back(nge);
}
