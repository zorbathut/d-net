
#include "game_effects.h"

#include "const.h"
#include "gfx.h"
#include "rng.h"

void GfxEffects::tick() {
  CHECK(life != -1);
  age++;
}
bool GfxEffects::dead() const {
  return ((float)age / FPS) >= life;
}

Color GfxEffects::getColor() const {
  return color;
}
float GfxEffects::getAge() const {
  return (float)age / FPS;
}
float GfxEffects::getAgeFactor() const {
  return ((float)age / FPS) / life;
}

void GfxEffects::setBaseColor() const {
  setColor(color * (1.0 - getAgeFactor()));
}

GfxEffects::GfxEffects(float in_life, Color in_color) {
  life = in_life;
  color = in_color;
  age = 0;
}
GfxEffects::~GfxEffects() { };

class GfxEffectsPoint : public GfxEffects {
public:

  virtual void render() const {
    setBaseColor();
    drawPoint(pos + vel * getAge(), 0.1f);
  }

  GfxEffectsPoint(Float2 in_pos, Float2 in_vel, float life, Color color) : GfxEffects(life, color), pos(in_pos), vel(in_vel) { };

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

  GfxEffectsCircle(Float2 in_center, float in_radius, float life, Color color) : GfxEffects(life, color), center(in_center), radius(in_radius) { };

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

  GfxEffectsText(Float2 in_pos, Float2 in_vel, float in_size, string in_text, float life, Color color) : GfxEffects(life, color), pos(in_pos), vel(in_vel), size(in_size), text(in_text) { };

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

  GfxEffectsPath(vector<Float2> in_path, Float2 in_pos_start, Float2 in_pos_vel, Float2 in_pos_acc, float in_ang_start, float in_ang_vel, float in_ang_acc, float life, Color color) : GfxEffects(life, color), path(in_path), pos_start(in_pos_start), pos_vel(in_pos_vel), pos_acc(in_pos_acc), ang_start(in_ang_start), ang_vel(in_ang_vel), ang_acc(in_ang_acc) { };

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

  GfxEffectsPing(Float2 in_pos, float in_radius_d, float in_thickness_d, float life, Color color) : GfxEffects(life, color), pos(in_pos), radius_d(in_radius_d), thickness_d(in_thickness_d) { };

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
      const float ofs = unsync_frand() * 2 * PI / vertx;
      const float chaos = len(makeAngle(1 * PI * 2 / vertx) - makeAngle(0)) * desrad / 2;
      vector<Float2> pex;
      for(int j = 0; j < vertx; j++)
        pex.push_back(center + makeAngle(j * PI * 2 / vertx + ofs) * desrad + Float2(unsync_symfrand(), unsync_symfrand()) * chaos);
      drawLineLoop(pex, 0.1);
    }
  }

  GfxEffectsBlast(Float2 in_center, float in_radius, Color in_bright, Color in_dim) : GfxEffects(sqrt(in_radius) * 0.1, Color(1.0, 1.0, 1.0)), center(in_center), radius(in_radius), bright(in_bright), dim(in_dim) { };

private:
  
  Float2 center;
  float radius;
  Color bright;
  Color dim;
};
  
smart_ptr<GfxEffects> GfxBlast(Float2 center, float radius, Color bright, Color dim) {
  return smart_ptr<GfxEffects>(new GfxEffectsBlast(center, radius, bright, dim)); }
