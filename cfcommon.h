#ifndef DNET_CFCOMMON
#define DNET_CFCOMMON

#include "const.h"
#include <cmath>
using namespace std;



/*************
 * Fast sin/cos
 */

#define SIN_TABLE_SIZE 360
extern float sin_table[SIN_TABLE_SIZE + 1];

inline float dsin(float in) {
  return sin_table[int(in * (2 * SIN_TABLE_SIZE / PI) + 0.5f)];
}

inline float fsin(float in) {
  if(in < 0 || in >= PI * 5 / 2) {
    in = fmod(in, PI * 2);
    if(in < 0)
      in = in + PI * 2;
    return fsin(in);
  }
  
  if(in < PI / 2) {
    return dsin(in);
  } else if(in < PI) {
    return dsin(PI - in);
  } else if(in < PI * 3 / 2) {
    return -dsin(in - PI);
  } else if(in < PI * 2) {
    return -dsin(PI * 2 - in);
  } else if(in < PI * 5 / 2) {
    return dsin(in - PI * 2);
  } else {
    return fsin(in - PI * 2);   // this should never happen, but I'm keeping CHECK out of this file for compiler reasons
  }
}
inline float fcos(float in) {
  return fsin(in + PI / 2);
}

#endif
