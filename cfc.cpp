
#include "cfc.h"

#include "coord.h"

using namespace std;

CFC::CFC(const Coord &c) {
  v = c.toFloat();
}

CFC2::CFC2(const Coord2 &c) {
  v = c.toFloat();
}

CFC4::CFC4(const Coord4 &c) {
  v = c.toFloat();
}
