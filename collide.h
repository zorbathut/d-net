#ifndef DNET_COLLIDE
#define DNET_COLLIDE

#include <vector>
#include <map>
#include <set>
using namespace std;

#include "util.h"
#include "coord.h"

#define ENABLE_COLLIDE_DEBUG_VIS

struct CollideId {
public:
    int category;
    int bucket;
    int item;

    CollideId() { };
    CollideId(pair<int, int> catbuck, int in_item) : category(catbuck.first), bucket(catbuck.second), item(in_item) { };
    CollideId(const CollideId &rhs) : category(rhs.category), bucket(rhs.bucket), item(rhs.item) { };
};

inline bool operator<(const CollideId &lhs, const CollideId &rhs) {
    if(lhs.category != rhs.category) return lhs.category < rhs.category;
    if(lhs.bucket != rhs.bucket) return lhs.bucket < rhs.bucket;
    if(lhs.item != rhs.item) return lhs.item < rhs.item;
    return false;
}
inline bool operator>(const CollideId &lhs, const CollideId &rhs) {
    return rhs < lhs;
}

inline bool operator==(const CollideId &lhs, const CollideId &rhs) {
    if(lhs.category != rhs.category) return false;
    if(lhs.bucket != rhs.bucket) return false;
    if(lhs.item != rhs.item) return false;
    return true;
}
inline bool operator!=(const CollideId &lhs, const CollideId &rhs) {
    return !(lhs == rhs);
}

struct CollideData {
public:
    CollideId lhs;
    CollideId rhs;
    Coord2 loc;

    CollideData() { };
    CollideData(const CollideId &in_lhs, const CollideId &in_rhs, const Coord2 &in_loc) : lhs(in_lhs), rhs(in_rhs), loc(in_loc) { };
    CollideData(const CollideData &in_rhs) : lhs(in_rhs.lhs), rhs(in_rhs.rhs), loc(in_rhs.loc) { };
};

inline bool operator<(const CollideData &lhs, const CollideData &rhs) {
    if(lhs.lhs != rhs.lhs) return lhs.lhs < rhs.lhs;
    if(lhs.rhs != rhs.rhs) return lhs.rhs < rhs.rhs;
    if(lhs.loc != rhs.loc) return lhs.loc < rhs.loc;
    return false;
}
inline bool operator>(const CollideData &lhs, const CollideData &rhs) {
    return rhs < lhs;
}

class ColliderZone {
private:
    vector< vector< pair< int, pair< Coord4, Coord4 > > > > items;

    int players;
public:

    void addToken(int groupid, int token, const Coord4 &line, const Coord4 &direction);
    void process(vector<pair<Coord, CollideData> > *clds) const;

    ColliderZone();
    ColliderZone(int players);
};

class Collider {
public:

	void reset(int players, const Coord4 &bounds);

    void startToken( int toki );
	void token( const Coord4 &line, const Coord4 &direction );

	void addThingsToGroup( int category, int gid, bool log = false);
	void endAddThingsToGroup();

	void process();

    bool next();
    const CollideData &getData() const;

	Collider();
	~Collider();

	void render() const;

private:
	
    enum { CSTA_WAIT, CSTA_ADD, CSTA_PROCESSED };
    
	int state;
    bool log;

    int curcollide;
    
    ColliderZone zone;
    
    Coord4 cbounds;
    vector< CollideData > collides;

    int curpush;
    int curtoken;

    int players;

};

#endif
