
#include "game_tank.h"

#include "gfx.h"
#include "rng.h"

void Tank::init(IDBTankAdjust in_tank, Color in_color) {
  tank = in_tank;
  color = in_color;
  health = in_tank.maxHealth();
  framesSinceDamage = -1;
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
  drawLineLoop(tankverts, 0.2);
  
  setColor(small);
  drawLineLoop(smtankverts, 0.2);
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
  Coord2 xt = makeAngle(Coord(d));
  Coord2 yt = makeAngle(Coord(d) - COORDPI / 2);
  Coord2 best(0, 0);
  for(int i = 0; i < tank.vertices().size(); i++)
    if(tank.vertices()[i].x > best.x)
      best = tank.vertices()[i];
  return Coord2(pos.x + best.x * xt.x + best.y * xt.y, pos.y + best.y * yt.y + best.x * yt.x);
};

pair<float, float> Tank::getNextInertia(const Keystates &keys) const {
  
  float dl;
  float dr;
  if(keys.axmode == KSAX_TANK) {
    dl = prepower(deadzone(keys.ax[0], keys.ax[1], DEADZONE_ABSOLUTE, 0.2));
    dr = prepower(deadzone(keys.ax[1], keys.ax[0], DEADZONE_ABSOLUTE, 0.2));
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

void Tank::genEffects(const GameImpactContext &gic, ProjectilePack *projectiles, const Player *player, int id) {
  CHECK(gic.players[id] == this);
  
  if(spawnShards) {
    vector<Coord2> tv = getTankVertices(pos, d);
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
      float angtot = ang.back();
      float shift = frand() * PI * 2;
      for(int i = 0; i < ang.size(); i++) {
        ang[i] *= PI * 2 / angtot;
        ang[i] += shift;
        //dprintf("%f\n", ang[i]);
      }
      //dprintf("---");
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
      Float2 path_pos_vel = vel.toFloat();
      float path_ang_vel = gaussian() / 20;
      gic.effects->push_back(GfxPath(vf2, (centr + subcentroid).toFloat(), path_pos_vel * 60, -path_pos_vel * 60, 0, path_ang_vel * 60, -path_ang_vel * 60, 0.5, player->getFaction()->color));
    }
    
    for(int i = 0; i < ang.size(); i++)
      for(int j = 0; j < glory.shotspersplit(); j++)
        projectiles->add(Projectile(centr, ang[i] + gaussian_scaled(2) / 8, glory.projectile(), id));
    
    detonateWarhead(glory.core(), centr, NULL, gic.players[id], gic, 1.0, true);
    
    spawnShards = false;
  }
}

float Tank::getDPS() const {
  return damageTaken / framesSinceDamage * FPS;
}

float Tank::getDPH() const {
  return damageTaken / damageEvents;
}

float Tank::getDPC(int cycles) const {
  return damageTakenPreviousHits / cycles;
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
