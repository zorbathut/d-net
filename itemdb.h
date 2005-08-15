#ifndef DNET_ITEMDB
#define DNET_ITEMDB

class ProjectileClass {
public:
    int velocity;
    int damage;
};

class Weapon {
public:
    int firerate;
    float costpershot;
    ProjectileClass *projectile;
};

void initItemdb();

#endif
