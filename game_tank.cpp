
#include "game_tank.h"

#include "collide.h"
#include "gfx.h"
#include "player.h"
#include "rng.h"

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
  
  damageDealt = 0;
  kills = 0;
}

void Tank::tick(const Keystates &kst) {
  pair<Coord2, float> newpos = getNextPosition(kst);
  
  pos = newpos.first;
  d = newpos.second;
  
  inertia = getNextInertia(kst);
  
  if(framesSinceDamage != -1)
    framesSinceDamage++;
  
  weaponCooldown--;
};

void Tank::render(const vector<Team> &teams) const {
  if(!live)
    return;

  vector<Coord2> tankverts = getTankVertices(pos, d);
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
  drawLineLoop(tankverts, 0.5);
  
  setColor(small);
  drawLineLoop(smtankverts, 0.5);
};

vector<Coord4> Tank::getCurrentCollide() const {
  if(!live)
    return vector<Coord4>();

  vector<Coord2> tankpts = getTankVertices(pos, d);
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

vector<Coord4> Tank::getNextCollide(const Keystates &keys) const {
  if(!live)
    return vector<Coord4>();

  pair<Coord2, float> newpos = getNextPosition(keys);
  vector<Coord2> tankpts = getTankVertices(newpos.first, newpos.second);
  vector<Coord4> rv;
  for(int i = 0; i < tankpts.size(); i++) {
    int j = (i + 1) % tankpts.size();
    rv.push_back(Coord4(tankpts[i], tankpts[j]));
  }
  return rv;
};

void Tank::addCollision(Collider *collider, const Keystates &keys, int owner) const {
  
  if(!live)
    return;

  vector<Coord2> tankpts = getTankVertices(pos, d);
  pair<Coord2, float> newpos = getNextPosition(keys);
  vector<Coord2> newtankpts = getTankVertices(newpos.first, newpos.second);
  CHECK(tankpts.size() == newtankpts.size());
  for(int i = 0; i < newtankpts.size(); i++)
    newtankpts[i] -= tankpts[i];
  for(int i = 0; i < tankpts.size(); i++)
    collider->addToken(CollideId(CGR_TANK, owner, 0), Coord4(tankpts[i], tankpts[(i + 1) % tankpts.size()]), Coord4(newtankpts[i], newtankpts[(i + 1) % tankpts.size()]));
};

vector<Coord2> Tank::getTankVertices(Coord2 pos, float td) const {
  Coord2 xt = makeAngle(Coord(td));
  Coord2 yt = makeAngle(Coord(td) - COORDPI / 2);
  vector<Coord2> rv;
  for(int i = 0; i < tank.vertices().size(); i++)
    rv.push_back(Coord2(pos.x + tank.vertices()[i].x * xt.x + tank.vertices()[i].y * xt.y, pos.y + tank.vertices()[i].y * yt.y + tank.vertices()[i].x * yt.x));
  return rv;
};

Coord2 Tank::getFiringPoint() const {
  return worldFromLocal(tank.firepoint());
};
Coord2 Tank::getMinePoint(Rng *rng) const {
  Coord tlen = 0;
  const vector<Coord2> &minepath = tank.minepath();
  for(int i = 0; i < minepath.size() - 1; i++)
    tlen += len(minepath[i] - minepath[i+1]);
  CHECK(tlen > 0);
  tlen = Coord(rng->frand() * tlen.toFloat());
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

pair<float, float> Tank::getNextInertia(const Keystates &keys) const {
  
  float dl;
  float dr;
  if(keys.axmode == KSAX_TANK) {
    dl = prepower(deadzone(keys.ax[0], keys.ax[1], DEADZONE_CENTER, 0.2));
    dr = prepower(deadzone(keys.ax[1], keys.ax[0], DEADZONE_CENTER, 0.2));
  } else if(keys.axmode == KSAX_ABSOLUTE || keys.axmode == KSAX_STEERING) {
    float dd;
    float dv;
    if(keys.axmode == KSAX_ABSOLUTE) {
      float xpd = deadzone(keys.ax[0], keys.ax[1], DEADZONE_CENTER, 0.2);
      float ypd = deadzone(keys.ax[1], keys.ax[0], DEADZONE_CENTER, 0.2);
      if(xpd == 0 && ypd == 0) {
        dv = dd = 0;
      } else {
        float desdir = atan2(-ypd, xpd);
        desdir -= d;
        desdir += 2 * PI;
        if(desdir > PI)
          desdir -= 2 * PI;
        dd = desdir / (tank.turnSpeed() / FPS);
        if(dd < -1)
          dd = -1;
        if(dd > 1)
          dd = 1;
        dv = min(sqrt(xpd * xpd + ypd * ypd), 1.f);
        // Various states (these numbers are wrong):
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
      dd = prepower(deadzone(keys.ax[0], keys.ax[1], DEADZONE_ABSOLUTE, 0.2));
      dv = prepower(deadzone(keys.ax[1], keys.ax[0], DEADZONE_ABSOLUTE, 0.2));
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

  dl = approach(inertia.first, dl, 500 / tank.mass() / FPS);  // 50 tons is 1/10 sec
  dr = approach(inertia.second, dr, 500 / tank.mass() / FPS);
  
  return make_pair(dl, dr);
}

pair<Coord2, float> Tank::getNextPosition(const Keystates &keys) const {
  
  Coord2 npos = pos;
  float nd = d;
  
  pair<float, float> inert = getNextInertia(keys);
  
  float dl = inert.first;
  float dr = inert.second;

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

  npos += makeAngle(Coord(nd)) * Coord(tank.maxSpeed() / FPS) * cdv;

  nd += tank.turnSpeed() / FPS * dd;
  nd += 2*PI;
  nd = fmod(nd, 2*(float)PI);
  
  return make_pair(npos, nd);
  
}

bool Tank::takeDamage(float damage) {
  health -= damage;
  
  damageEvents++;
  
  // We halve the first "damage" to do a better job of estimating damage.
  if(framesSinceDamage == -1) {
    framesSinceDamage = 0;
    damageTaken += damage / 2;
  } else {
    damageTaken += damage;
  }
  
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
    vector<Coord2> tv = getTankVertices(pos, d);
    Coord2 centr = getCentroid(tv);
    Coord tva = getArea(tv);
    
    for(int i = 0; i < tv.size(); i++)
      tv[i] -= centr;
    
    const IDBGloryAdjust &glory = player->getGlory();
    
    vector<float> ang;
    deployProjectile(glory.core(), launchData(), GamePlayerContext(this, projectiles, gic), &ang);
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
      CHECK(thischunk.size() == 1);
      chunks.push_back(thischunk[0]);
    }
    
    for(int i = 0; i < chunks.size(); i++) {
      Coord2 subcentroid = getCentroid(chunks[i]);
      vector<Float2> vf2;
      for(int j = 0; j < chunks[i].size(); j++)
        vf2.push_back((chunks[i][j] - subcentroid).toFloat());
      Coord2 vel = normalize(subcentroid) / 10 * tva / getArea(chunks[i]);
      Float2 path_pos_vel = vel.toFloat();
      float path_ang_vel = gic.rng->gaussian() / 20;
      gic.effects->push_back(GfxPath(vf2, (centr + subcentroid).toFloat(), path_pos_vel * 60, -path_pos_vel * 60, 0, path_ang_vel * 60, -path_ang_vel * 60, 0.5, player->getFaction()->color));
    }
    
    {
      vector<IDBDeployAdjust> vd = glory.blast();
      for(int i = 0; i < vd.size(); i++)
        deployProjectile(vd[i], launchData(), GamePlayerContext(this, projectiles, gic));
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

void Tank::addDamage(float amount) {
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

float Tank::getHealth() const {
  return health;
}
bool Tank::isLive() const {
  return live;
}

void Tank::respawn(Coord2 in_pos, float in_d, int in_team) {
  IDBTankAdjust old_tank = tank;
  Color old_color = color;
  *this = Tank();
  init(old_tank, old_color);
  pos = in_pos;
  d = in_d;
  team = in_team;
}

void Tank::tryToFire(Button keys[SIMUL_WEAPONS], Player *player, ProjectilePack *projectiles, int id, const GameImpactContext &gic, vector<pair<string, float> > *status_text, float *firepowerSpent) {
  if(weaponCooldown <= 0) {
    StackString sst(StringPrintf("Firetesting"));
    // The player can fire, so let's find out if he does
    
    // weaponCooldownSubvals is maintained in here.
    // Every "fire" attempt, we find the weapon with the lowest subval. We subtract that from all active weapons (thereby making that value 0),
    // then we add the seconds-until-next-shot to that one. Any non-active weapons are clamped to 0 on the theory that the player is hammering
    // that button and really wants it to fire.
    
    float rlev = 1e20; // uh, no
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
        weaponCooldownSubvals[j] = max(weaponCooldownSubvals[j] - rlev, (float)0);
      }
      
      weaponCooldownSubvals[curfire] = FPS / player->getWeapon(curfire).firerate();
      
      // Blam!
      IDBWeaponAdjust weapon = player->getWeapon(curfire);
      
      deployProjectile(weapon.launcher().deploy(), launchData(), GamePlayerContext(this, projectiles, gic));
      
      weaponCooldown = weapon.framesForCooldown(gic.rng);
      // hack here to detect weapon out-of-ammo
      string lastname = weapon.name();
      *firepowerSpent += player->shotFired(curfire);
      if(weapon.name() != lastname) {
        status_text->push_back(make_pair(weapon.name(), 2));
      }
      
      {
        string slv = StringPrintf("%d", player->shotsLeft(curfire));
        if(count(slv.begin(), slv.end(), '0') == slv.size() - 1)
          status_text->push_back(make_pair(slv, 1));
      }
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
  pos = Coord2(0, 0);
  d = 0;
  live = true;
  spawnShards = false;
  health = -47283;
  inertia = make_pair(0.f, 0.f);
  weaponCooldown = 0;
  memset(weaponCooldownSubvals, 0, sizeof(weaponCooldownSubvals)); // if you're not using IEEE floats, get a new computer.
  zone_current = -1;
  zone_frames = 0;
}

Coord2 Tank::worldFromLocal(const Coord2 &coord) const {
  Coord2 xt = makeAngle(Coord(d));
  Coord2 yt = makeAngle(Coord(d) - COORDPI / 2);
  return Coord2(pos.x + coord.x * xt.x + coord.y * xt.y, pos.y + coord.y * yt.y + coord.x * yt.x);
}
