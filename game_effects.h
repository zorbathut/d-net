#ifndef DNET_GAME_EFFECTS
#define DNET_GAME_EFFECTS

#include "color.h"
#include "float.h"
#include "util.h"
#include "smartptr.h"

using namespace std;
  
// there's a lot of redundancy here, but ATM I don't care
// I'm not really sure whether inheritance will honestly be better thanks to allocation overhead, though
class GfxEffects {
public:

  void tick();
  bool dead() const;

  virtual void render() const = 0;

  virtual ~GfxEffects();

protected:
  
  Color getColor() const;
  float getAge() const;
  float getAgeFactor() const;

  void setBaseColor() const;
  
  GfxEffects(float life, Color color);

private:
  
  float life;
  int age;
  Color color;
};

smart_ptr<GfxEffects> GfxPoint(Float2 pos, Float2 vel, float life, Color color);
smart_ptr<GfxEffects> GfxCircle(Float2 center, float radius, float life, Color color);
smart_ptr<GfxEffects> GfxText(Float2 pos, Float2 vel, float size, string text, float life, Color color);
smart_ptr<GfxEffects> GfxPath(vector<Float2> path, Float2 pos_start, Float2 pos_vel, Float2 pos_acc, float ang_start, float ang_vel, float ang_acc, float life, Color color);
smart_ptr<GfxEffects> GfxPing(Float2 pos, float radius_d, float thickness_d, float life, Color color);
smart_ptr<GfxEffects> GfxBlast(Float2 center, float radius, Color bright, Color dim);

class IDBEffectsAdjust;
smart_ptr<GfxEffects> GfxIdb(Float2 center, float normal, Float2 velocity, const IDBEffectsAdjust &effect);
smart_ptr<GfxEffects> GfxIdb(Float2 center, float normal, Float2 velocity, Float2 force, const IDBEffectsAdjust &effect);

#endif
