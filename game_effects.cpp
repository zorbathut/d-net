
#include "game_effects.h"

#include "const.h"
#include "gfx.h"
#include "rng.h"
#include "itemdb.h"

using namespace std;

void GfxEffects::tick() {
  CHECK(life != -1);
  age += 1.f / FPS;
}
bool GfxEffects::dead() const {
  return age >= life;
}

Color GfxEffects::getColor() const {
  return color;
}
float GfxEffects::getAge() const {
  return age;
}
float GfxEffects::getAgeFactor() const {
  return age / life;
}

void GfxEffects::setBaseColor() const {
  setColor(color * (1.0 - getAgeFactor()));
}

GfxEffects::GfxEffects(float in_life, Color in_color, bool distributed) {
  life = in_life;
  color = in_color;
  age = 0;
}
GfxEffects::~GfxEffects() { };

class GfxEffectsPoint : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawPoint(pos + vel * getAge(), 0.5f);
  }

  GfxEffectsPoint(Float2 in_pos, Float2 in_vel, float life, Color color) : GfxEffects(life, color, false), pos(in_pos), vel(in_vel) { };

private:
  
  Float2 pos;
  Float2 vel;
};

smart_ptr<GfxEffects> GfxPoint(Float2 pos, Float2 vel, float life, Color color) {
  return smart_ptr<GfxEffects>(new GfxEffectsPoint(pos, vel, life, color)); }

class GfxEffectsCircle : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawCircle(center, radius, 0.1f);
  }

  GfxEffectsCircle(Float2 in_center, float in_radius, float life, Color color) : GfxEffects(life, color, false), center(in_center), radius(in_radius) { };

private:
  
  Float2 center;
  float radius;
};

smart_ptr<GfxEffects> GfxCircle(Float2 center, float radius, float life, Color color) {
  return smart_ptr<GfxEffects>(new GfxEffectsCircle(center, radius, life, color)); }

class GfxEffectsText : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawText(text, size, pos + vel * getAge());
  }

  GfxEffectsText(Float2 in_pos, Float2 in_vel, float in_size, string in_text, float life, Color color) : GfxEffects(life, color, false), pos(in_pos), vel(in_vel), size(in_size), text(in_text) { };

private:
  
  Float2 pos;
  Float2 vel;
  float size;
  string text;
};

smart_ptr<GfxEffects> GfxText(Float2 pos, Float2 vel, float size, string text, float life, Color color) {
  return smart_ptr<GfxEffects>(new GfxEffectsText(pos, vel, size, text, life, color)); }

class GfxEffectsPath : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawTransformedLinePath(path, ang_start + ang_vel * getAge() + ang_acc * getAge() * getAge() / 2, pos_start + pos_vel * getAge() + pos_acc * getAge() * getAge() / 2, 0.1f);
  }

  GfxEffectsPath(vector<Float2> in_path, Float2 in_pos_start, Float2 in_pos_vel, Float2 in_pos_acc, float in_ang_start, float in_ang_vel, float in_ang_acc, float life, Color color) : GfxEffects(life, color, false), path(in_path), pos_start(in_pos_start), pos_vel(in_pos_vel), pos_acc(in_pos_acc), ang_start(in_ang_start), ang_vel(in_ang_vel), ang_acc(in_ang_acc) { };

private:
  
  vector<Float2> path;
  Float2 pos_start;
  Float2 pos_vel;
  Float2 pos_acc;
  float ang_start;
  float ang_vel;
  float ang_acc;
};
  
smart_ptr<GfxEffects> GfxPath(vector<Float2> path, Float2 pos_start, Float2 pos_vel, Float2 pos_acc, float ang_start, float ang_vel, float ang_acc, float life, Color color) {
  return smart_ptr<GfxEffects>(new GfxEffectsPath(path, pos_start, pos_vel, pos_acc, ang_start, ang_vel, ang_acc, life, color)); }

class GfxEffectsPing : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawCircle(pos, radius_d * getAge(), thickness_d * getAge());
  }

  GfxEffectsPing(Float2 in_pos, float in_radius_d, float in_thickness_d, float life, Color color) : GfxEffects(life, color, false), pos(in_pos), radius_d(in_radius_d), thickness_d(in_thickness_d) { };

private:
  
  Float2 pos;
  float radius_d;
  float thickness_d;
};

smart_ptr<GfxEffects> GfxPing(Float2 pos, float radius_d, float thickness_d, float life, Color color) {
  return smart_ptr<GfxEffects>(new GfxEffectsPing(pos, radius_d, thickness_d, life, color)); }

class GfxEffectsBlast : public GfxEffects {
public:

  virtual void render() const {
    const float desarea = radius * radius * PI * getAgeFactor();
    const float desrad = sqrt(desarea / PI);
    const int vertx = 16;
    for(int i = 0; i < 5; i++) {
      if(!i) {
        setColor(bright * (1.0 - getAgeFactor()));
      } else {
        setColor(dim * (1.0 - getAgeFactor()));
      }
      const float chaos = len(makeAngle(1 * PI * 2 / vertx) - makeAngle(0)) * desrad / 2;
      drawBlast(center, desrad, chaos, vertx);
    }
  }

  GfxEffectsBlast(Float2 in_center, float in_radius, Color in_bright, Color in_dim) : GfxEffects(sqrt(in_radius) * 0.1, Color(1.0, 1.0, 1.0), false), center(in_center), radius(in_radius), bright(in_bright), dim(in_dim) { };

private:
  
  Float2 center;
  float radius;
  Color bright;
  Color dim;
};
  
smart_ptr<GfxEffects> GfxBlast(Float2 center, float radius, Color bright, Color dim) {
  return smart_ptr<GfxEffects>(new GfxEffectsBlast(center, radius, bright, dim)); }
  
class GfxEffectsIonBlast : public GfxEffects {
public:
    
  virtual void render() const {
    for(int i = 0; i < visuals.size(); i++) {
      setColor(visuals[i].second * pow(1.0 - getAgeFactor(), 1.0));
      for(int j = 0; j < visuals[i].first; j++) {
        float rad = unsync().frand() * radius;
        drawBlast(center, rad, rad * 0.1, 16);
      }
    }
  }
  
  GfxEffectsIonBlast(Float2 center, float duration, float radius, const vector<pair<int, Color> > &visuals) : GfxEffects(duration, Color(0, 0, 0), false), center(center), radius(radius), visuals(visuals) {
  };
  
private:
  
  Float2 center;
  float radius;
  vector<pair<int, Color> > visuals;
};

smart_ptr<GfxEffects> GfxIonBlast(Float2 center, float duration, float radius, const vector<pair<int, Color> > &visuals) {
  return smart_ptr<GfxEffects>(new GfxEffectsIonBlast(center, duration, radius, visuals)); }

float dpp = 0.1;
void genlofs(vector<float> *lofs, int s, int e) {
  float ale = dpp * (e - s) - abs((*lofs)[s] - (*lofs)[e]);
  float opt = ((*lofs)[s] + (*lofs)[e]) / 2;
  (*lofs)[(s + e) / 2] = unsync().gaussian_scaled(10) * ale + opt;
  if(e - s > 2) {
    genlofs(lofs, s, (s + e) / 2);
    genlofs(lofs, (s + e) / 2, e);
  }
}

class GfxEffectsLightning : public GfxEffects {
public:
    
  virtual void render() const {
    setBaseColor();
    CHECK(lofs.size());
    for(int i = 0; i < lofs.size() - 1; i++)
      drawLine(rotate(Float2((float)i / lofs.size(), lofs[i]) * l, d) + start, rotate(Float2((float)(i + 1) / lofs.size(), lofs[i + 1]) * l, d) + start, 0.5);
  }
  
  GfxEffectsLightning(Float2 start, Float2 end) : GfxEffects(0.8, Color(0.2, 0.3, 0.9), false), start(start) {
    lofs.resize(17);
    lofs[0] = 0;
    lofs[16] = 0;
    genlofs(&lofs, 0, 16);
    
    d = getAngle(end - start);
    l = len(end - start);
  };
  
private:
  
  Float2 start;
  float d;
  float l;
  vector<float> lofs;
};
  
smart_ptr<GfxEffects> GfxLightning(Float2 start, Float2 end) {
  return smart_ptr<GfxEffects>(new GfxEffectsLightning(start, end)); }

class GfxEffectsIdbParticle : public GfxEffects {
public:
    
  virtual void render() const {
    setBaseColor();
    if(effect.particle_slowdown() == 1) {
      drawPoint(center + velocity * getAge(), effect.particle_radius());
    } else {
      drawPoint(center + velocity * (pow(effect.particle_slowdown(), getAge()) / log(effect.particle_slowdown()) - 1 / log(effect.particle_slowdown())), effect.particle_radius());  // calculus FTW
    }
  }
  
  GfxEffectsIdbParticle(Float2 in_center, float normal, Float2 in_inertia, Float2 in_force, const IDBEffectsAdjust &effect) : GfxEffects(effect.particle_lifetime(), effect.particle_color(), effect.particle_distribute()), center(in_center), effect(effect) {
    CHECK(effect.type() == IDBEffects::EFT_PARTICLE);
    velocity = in_inertia * effect.particle_multiple_inertia() + reflect(in_inertia, normal) * effect.particle_multiple_reflect() + in_force * effect.particle_multiple_force() + makeAngle(unsync().frand() * 2 * PI) * unsync().gaussian() * effect.particle_spread();
    if(effect.particle_distribute())
      center -= in_inertia * unsync().frand() / FPS;
  };
  
private:
  
  Float2 center;
  Float2 velocity;
  IDBEffectsAdjust effect;
};

smart_ptr<GfxEffects> GfxIdb(Float2 center, float normal, Float2 inertia, const IDBEffectsAdjust &effect) {
  CHECK(effect.type() != IDBEffects::EFT_PARTICLE || effect.particle_multiple_force() == 0);
  return GfxIdb(center, normal, inertia, Float2(0, 0), effect);
}

smart_ptr<GfxEffects> GfxIdb(Float2 center, float normal, Float2 inertia, Float2 force, const IDBEffectsAdjust &effect) {
  if(effect.type() == IDBEffects::EFT_PARTICLE) {
    return smart_ptr<GfxEffects>(new GfxEffectsIdbParticle(center, normal, inertia, force, effect));
  } else if(effect.type() == IDBEffects::EFT_IONBLAST) {
    return GfxIonBlast(center, effect.ionblast_duration(), effect.ionblast_radius(), effect.ionblast_visuals());
  } else {
    CHECK(0);
  }
}
