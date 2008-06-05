
#include "game_tank.h"

#include "collide.h"
#include "gfx.h"
#include "player.h"
#include "rng.h"
#include "adler32_util.h"

using namespace std;

void Tank::init(IDBTankAdjust in_tank, Color in_color) {
  tank = in_tank;
  color = in_color;
  health = in_tank.maxHealth();
  framesSinceDamage = -1;
  prerollFrames = 0;
  damageTakenPreviousHits = 0;
  damageEvents = 0;
  damageTaken = 0;
  has_vengeance_location = false;
  
  damageDealt = 0;
  kills = 0;
  
  glory_resistance = 0;
  glory_resist_boost_frames = 0;
  
  taunt_frames = 0;
}

void Tank::updateInertia(const Keystates &kst) {
  inertia = getNextInertia(kst);
}

void Tank::tick() {
  pi = getNextPosition();
  
  if(framesSinceDamage != -1)
    framesSinceDamage++;
  
  weaponCooldown--;
  
  {
    static const Coord resistance_approach = 0.95;
    static const Coord resistance_approach_per_sec = 0.95; // this is a fraction of how close it gets to the theoretical max
    static const Coord resistance_dapproach_per_sec = 0.75;
    static const Coord resistance_approach_per_frame = 1.0 - pow(1.0 - resistance_approach_per_sec, 1. / FPS);
    static const Coord resistance_dapproach_per_frame = 1.0 - pow(1.0 - resistance_dapproach_per_sec, 1. / FPS);
    if(glory_resist_boost_frames) {
      glory_resist_boost_frames--;
      glory_resistance = glory_resistance * (1.0 - resistance_approach_per_frame) + resistance_approach * resistance_approach_per_frame;
    } else {
      glory_resistance = glory_resistance * (1.0 - resistance_dapproach_per_frame);
    }
  }
};

void Tank::render(const vector<Team> &teams) const {
  if(!live)
    return;

  vector<Coord2> tankverts = getTankVertices(pi);
  vector<Coord2> smtankverts;
  Coord2 centr = getCentroid(tankverts);
  for(int i = 0; i < tankverts.size(); i++)
    smtankverts.push_back((tankverts[i] - centr) * Coord(0.5) + centr);
  
  Color main;
  Color small;
  
  main = color;
  small = teams[team].color;
  
  if(teams[team].swap_colors)
    swap(main, small);
  
  small = small * 0.5;
  
  setColor(main);
  drawLineLoop(tankverts, 0.75);
  
  setColor(small);
  drawLineLoop(smtankverts, 0.75);
  
  setColor(Color(1, 0.8, 0) * glory_resistance.toFloat() * 0.5);
  const float radi = 2;
  const float width = 1;
  drawPolygon(pi.pos, radi, width, 6, pi.d.toFloat());
  setColor(Color(1, 0.8, 0) * glory_resistance.toFloat() * 0.3);
  for(int i = 0; i < 6; i++)
    drawPolygon(pi.pos.toFloat() + makeAngle(i * 2 * PI / 6 + PI / 2 + pi.d.toFloat()) * cos(PI / 6) * radi * 2, radi, width, 6, pi.d.toFloat());
};

void Tank::checksum(Adler32 *adl) const {
  adler(adl, team);
  adler(adl, zone_current);
  adler(adl, zone_frames);
  adler(adl, pi);
  adler(adl, inertia);
  adler(adl, keys);
  adler(adl, weaponCooldown);
  adler_array(adl, weaponCooldownSubvals);
  adler(adl, health);
  adler(adl, spawnShards);
  adler(adl, live);
  adler(adl, tank);
  adler(adl, glory_resistance);
  adler(adl, glory_resist_boost_frames);
};

vector<Coord4> Tank::getCurrentCollide() const {
  if(!live)
    return vector<Coord4>();

  vector<Coord2> tankpts = getTankVertices(pi);
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

vector<Coord4> Tank::getNextCollide() const {
  if(!live)
    return vector<Coord4>();

  vector<Coord2> tankpts = getTankVertices(getNextPosition());
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

void Tank::addCollision(Collider *collider, int owner) const {
  
  if(!live)
    return;

  vector<Coord2> tankpts = getTankVertices(pi);
  vector<Coord2> newtankpts = getTankVertices(getNextPosition());
  CHECK(tankpts.size() == newtankpts.size());
  for(int i = 0; i < newtankpts.size(); i++)
    newtankpts[i] -= tankpts[i];
  for(int i = 0; i < tankpts.size(); i++)
    collider->addNormalToken(CollideId(CGR_TANK, owner, 0), Coord4(tankpts[i], tankpts[(i + 1) % tankpts.size()]), Coord4(newtankpts[i], newtankpts[(i + 1) % tankpts.size()]));
};

vector<Coord2> Tank::getTankVertices(const CPosInfo &cpi) const {
  return tank.getTankVertices(cpi);
};

Coord2 Tank::getFiringPoint() const {
  return worldFromLocal(tank.firepoint());
};
Coord2 Tank::getRearFiringPoint() const {
  return worldFromLocal(tank.rearfirepoint());
};
Coord2 Tank::getMinePoint(Rng *rng) const {
  Coord tlen = 0;
  const vector<Coord2> &minepath = tank.minepath();
  for(int i = 0; i < minepath.size() - 1; i++)
    tlen += len(minepath[i] - minepath[i+1]);
  CHECK(tlen > 0);
  tlen = rng->cfrand() * tlen;
  for(int i = 0; i < minepath.size() - 1; i++) {
    if(tlen >= len(minepath[i] - minepath[i+1])) {
      tlen -= len(minepath[i] - minepath[i+1]);
    } else {
      tlen /= len(minepath[i] - minepath[i+1]);
      return worldFromLocal(lerp(minepath[i], minepath[i+1], tlen));
    }
  }
  CHECK(0);
}

pair<Coord2, Coord> Tank::getNextInertia(const Keystates &keys) const {
  
  Coord precisionmult = 1.0;
  if(keys.precision.down)
    precisionmult *= 0.2;
  
  Coord dl;
  Coord dr;
  if(keys.axmode == KSAX_TANK) {
    dl = prepower(deadzone(keys.ax[0], keys.ax[1], DEADZONE_CENTER, 0.2)) * precisionmult;
    dr = prepower(deadzone(keys.ax[1], keys.ax[0], DEADZONE_CENTER, 0.2)) * precisionmult;
  } else if(keys.axmode == KSAX_ABSOLUTE || keys.axmode == KSAX_STEERING) {
    Coord dd;
    Coord dv;
    if(keys.axmode == KSAX_ABSOLUTE) {
      Coord xpd = deadzone(keys.ax[0], keys.ax[1], DEADZONE_CENTER, 0.2);
      Coord ypd = deadzone(keys.ax[1], keys.ax[0], DEADZONE_CENTER, 0.2);
      if(xpd == 0 && ypd == 0) {
        dv = dd = 0;
      } else {
        Coord desdir = getAngle(Coord2(xpd, -ypd));
        desdir -= pi.d;
        desdir += 2 * PI;
        if(desdir > PI)
          desdir -= 2 * PI;
        dd = desdir / (tank.turnSpeed() / FPS);
        if(dd < -1)
          dd = -1;
        if(dd > 1)
          dd = 1;
        dv = min(sqrt(xpd * xpd + ypd * ypd), Coord(1));
        // Various states (these numbers are wrong):
        // abs(desdir) / PI
        // 0 .. 0.333 - drive forwards
        // 0.333 .. 0.666 - do not drive
        // 0.666 .. 1.0 - drive backwards, don't turn
        if(abs(desdir) < PI / 3) {
          ;
        } else if(abs(desdir) < PI / 3 * 2) {
          dv = 0; // if we're near right angles, stop
        } else if(abs(desdir) < PI / 10 * 9) {
          dv = -dv;   // if we're merely backwards, go backwards
        } else {  // if we're straight backwards, don't turn
          dv = -dv;
          dd = 0;
        }
        dv *= precisionmult;
        dd *= precisionmult;
      }
    } else {
      dd = prepower(deadzone(keys.ax[0], keys.ax[1], DEADZONE_ABSOLUTE, 0.2)) * precisionmult;
      dv = prepower(deadzone(keys.ax[1], keys.ax[0], DEADZONE_ABSOLUTE, 0.2)) * precisionmult;
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
    Coord d00, d01, d02, d10, d11, d12;
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
      Coord intens = abs(dv);
      dl = (d02 * intens + d01 * (1 - intens));
      dr = (d12 * intens + d11 * (1 - intens));
    } else {
      Coord intens = abs(dv);
      dl = (d00 * intens + d01 * (1 - intens));
      dr = (d10 * intens + d11 * (1 - intens));
    }
  }
 
  Coord dv = (dr + dl) / 2;
  Coord dd = (dl - dr) / 2;
  
  // More random thoughts:
  // 1 and 0 should stay 1 and 0
  // 0.5 and 0 should stay 0.5 and 0
  // 0.5 and 0.5 should scale up (to 0.707 both?)
  
  Coord2 ult = makeAngle(getAngle(Coord2(dv, dd)));
  Coord dif = 1 / (abs(ult.x) + abs(ult.y));
  dv /= dif;
  dd /= dif;
  
  if(dv > 1) dv = 1;
  if(dd > 1) dd = 1;
  if(dv < -1) dv = -1;
  if(dd < -1) dd = -1;

  CHECK(dv >= -1 && dv <= 1);
  CHECK(dd >= -1 && dd <= 1);
  
  if(dv < 0)
    dv *= 0.75; // drive backwards more slowly

  Coord turn = Coord(tank.turnSpeed()) * Coord(dd);
  Coord2 speed = makeAngle(Coord(pi.d) + turn / FPS) * Coord(tank.maxSpeed()) * dv;
  
  speed = approach(inertia.first, speed, 250 / Coord(tank.mass()) / FPS * Coord(tank.maxSpeed()), 2.5 / FPS * Coord(tank.maxSpeed()));
  turn = approach(inertia.second, turn, 250 / Coord(tank.mass()) / FPS * Coord(tank.turnSpeed()), 2.5 / FPS * Coord(tank.turnSpeed()));
  
  return make_pair(speed, turn);
}

CPosInfo Tank::getNextPosition() const {
  return CPosInfo(pi.pos + inertia.first / FPS, mod(pi.d + inertia.second / FPS + COORDPI * 2, COORDPI * 2));
}

bool Tank::takeDamage(Coord damage, const Coord2 &target) {
  health -= damage;
  
  damageEvents++;
  
  // We halve the first "damage" to do a better job of estimating damage.
  if(framesSinceDamage == -1) {
    framesSinceDamage = 0;
    damageTaken += damage.toFloat() / 2;
  } else {
    damageTaken += damage.toFloat();
  }
  
  has_vengeance_location = true;
  vengeance_location = target;
  
  if(health <= 0 && live) {
    live = false;
    spawnShards = true;
    return true;
  }
  return false;
};

void Tank::genEffects(const GameImpactContext &gic, ProjectilePack *projectiles, const Player *player) {
  StackString sst("genEffects");
  
  if(spawnShards) {
    vector<Coord2> tv = getTankVertices(pi);
    Coord2 centr = getCentroid(tv);
    Coord tva = getArea(tv);
    
    for(int i = 0; i < tv.size(); i++)
      tv[i] -= centr;
    
    const IDBGloryAdjust &glory = player->getGlory();
    
    vector<float> ang;
    deployProjectile(glory.core(), launchData(), GamePlayerContext(this, projectiles, gic), DamageFlags(1.0, true, true), &ang);
    CHECK(ang.size() >= 2);
    
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
      if(thischunk.size() != 1)
        dprintf("Multiple chunks: %d\n", thischunk.size());
      for(int i = 0; i < thischunk.size(); i++)
        chunks.push_back(thischunk[i]);
    }
    
    for(int i = 0; i < chunks.size(); i++) {
      Coord2 subcentroid = getCentroid(chunks[i]);
      vector<Float2> vf2;
      for(int j = 0; j < chunks[i].size(); j++)
        vf2.push_back((chunks[i][j] - subcentroid).toFloat());
      Coord2 vel = normalize(subcentroid) / 10 * tva / max(getArea(chunks[i]), 1);
      Float2 path_pos_vel = vel.toFloat();
      float path_ang_vel = gic.rng->gaussian() / 20;
      gic.effects->push_back(GfxPath(vf2, (centr + subcentroid).toFloat(), path_pos_vel * FPS, -path_pos_vel * FPS, 0, path_ang_vel * FPS, -path_ang_vel * FPS, 1, player->getFaction()->color));
    }
    
    {
      vector<IDBDeployAdjust> vd = glory.blast();
      for(int i = 0; i < vd.size(); i++)
        deployProjectile(vd[i], launchData(), GamePlayerContext(this, projectiles, gic), DamageFlags(1.0, true, true));
    }
    
    spawnShards = false;
  }
}

float Tank::getDPS() const {
  return damageTaken / (framesSinceDamage + prerollFrames) * FPS;
}

float Tank::getDPH() const {
  if(damageEvents == 0)
    return -1;
  return damageTaken / damageEvents;
}

float Tank::getDPC(int cycles) const {
  return damageTakenPreviousHits / cycles;
}

int Tank::dumpDamageframes() const {
  CHECK(prerollFrames == 0);
  return framesSinceDamage;
}

void Tank::insertDamageframes(int dfr) {
  CHECK(prerollFrames == 0);
  CHECK(framesSinceDamage == 0 || framesSinceDamage == -1);
  prerollFrames = dfr;
  framesSinceDamage = -1;
}

bool Tank::hasTakenDamage() const {
  return damageTaken != 0;
}

void Tank::addCycle() {
  damageTakenPreviousHits += damageTaken;
  damageTaken = 0;
}

Coord Tank::getGloryResistance() const {
  return glory_resistance;
}
void Tank::addDamage(Coord amount) {
  damageDealt += amount;
}
void Tank::addKill() {
  kills++;
}

void Tank::addAccumulatedScores(Player *player) {
  player->accumulateStats(kills, damageDealt);
  damageDealt = 0;
  kills = 0;
}

DeployLocation Tank::launchData() const {
  return DeployLocation(this);
}

Color Tank::getColor() const {
  return color;
}

Coord Tank::getHealth() const {
  return health;
}
bool Tank::isLive() const {
  return live;
}

Coord2 Tank::get_vengeance_location() const {
  CHECK(has_vengeance_location);
  return vengeance_location;
}

void Tank::respawn(Coord2 in_pos, Coord in_d) {
  IDBTankAdjust old_tank = tank;
  Color old_color = color;
  int old_team = team;
  *this = Tank();
  init(old_tank, old_color);
  pi.pos = in_pos;
  pi.d = in_d;
  team = old_team;
}

void Tank::tryToFire(Button keys[SIMUL_WEAPONS], Player *player, ProjectilePack *projectiles, int id, const GameImpactContext &gic, vector<pair<string, float> > *status_text) {
  if(weaponCooldown <= 0) {
    StackString sst(StringPrintf("Firetesting"));
    // The player can fire, so let's find out if he does
    
    // weaponCooldownSubvals is maintained in here.
    // Every "fire" attempt, we find the weapon with the lowest subval. We subtract that from all active weapons (thereby making that value 0),
    // then we add the seconds-until-next-shot to that one. Any non-active weapons are clamped to 0 on the theory that the player is hammering
    // that button and really wants it to fire.
    
    Coord rlev = 1 << 20; // uh, no
    int curfire = -1;
    for(int j = 0; j < SIMUL_WEAPONS; j++) {
      if(keys[j].down) {
        if(rlev > weaponCooldownSubvals[j]) {
          rlev = weaponCooldownSubvals[j];
          curfire = j;
        }
      } else {
        weaponCooldownSubvals[j] = 0;
      }
    }
    CHECK(rlev >= 0);
    
    if(curfire != -1) {
      // We're firing something!
      
      for(int j = 0; j < SIMUL_WEAPONS; j++) {
        weaponCooldownSubvals[j] = max(weaponCooldownSubvals[j] - rlev, 0);
      }
      
      weaponCooldownSubvals[curfire] = FPS / player->getWeapon(curfire).firerate();
      
      // Blam!
      IDBWeaponAdjust weapon = player->getWeapon(curfire);
      
      pair<int, int> wcd = weapon.simulateWeaponFiring(gic.rng);
      weaponCooldown = wcd.second;
      
      string ammo_display;
      
      for(int i = 0; i < wcd.first; i++) {
        deployProjectile(weapon.launcher().deploy(), launchData(), GamePlayerContext(this, projectiles, gic), DamageFlags(1.0, true, false));
        
        // hack here to detect weapon out-of-ammo
        string lastname = informalNameFromIDB(weapon);
        player->shotFired(curfire);
        if(informalNameFromIDB(weapon) != lastname) {
          status_text->push_back(make_pair(informalNameFromIDB(weapon), 2));
          break;
        }
        
        {
          string slv = StringPrintf("%d", player->shotsLeft(curfire));
          if(count(slv.begin(), slv.end(), '0') == slv.size() - 1)
            ammo_display = slv;
        }
      }
      
      if(weapon.glory_resistance())
        glory_resist_boost_frames = weaponCooldown;
      
      if(ammo_display.size())
        status_text->push_back(make_pair(ammo_display, 1));
    }
  }
}

void Tank::megaboostHealth() {
  health = 1000000000;
}
void Tank::setDead() {
  live = false;
}

Tank::Tank() : tank(NULL, IDBAdjustment()) /* do not fucking use this */ {
  pi = CPosInfo(Coord2(0, 0), 0);
  has_vengeance_location = false;
  live = true;
  spawnShards = false;
  health = -47283;
  inertia = make_pair(Coord2(0.f, 0.f), 0.f);
  weaponCooldown = 0;
  for(int i = 0; i < ARRAY_SIZE(weaponCooldownSubvals); i++)
    weaponCooldownSubvals[i] = 0;
  zone_current = -1;
  zone_frames = 0;
  team = -12345;
}

Coord2 Tank::worldFromLocal(const Coord2 &coord) const {
  Coord2 xt = makeAngle(Coord(pi.d));
  Coord2 yt = makeAngle(Coord(pi.d) - COORDPI / 2);
  return Coord2(pi.pos.x + coord.x * xt.x + coord.y * xt.y, pi.pos.y + coord.y * yt.y + coord.x * yt.x);
}
