#ifndef DNET_COLOR
#define DNET_COLOR

#include <string>

using namespace std;
 
struct Color {
public:
  float r, g, b;

  float getHue() const;
  float getSaturation() const;
  float getValue() const;

  Color();
  Color(float in_r, float in_g, float in_b);
};

Color colorFromString(const string &str);

Color operator+(const Color &lhs, const Color &rhs);
Color operator*(const Color &lhs, float rhs);
Color operator/(const Color &lhs, float rhs);

const Color &operator+=(Color &lhs, const Color &rhs);
const Color &operator*=(Color &lhs, float rhs);

bool operator==(const Color &lhs, const Color &rhs);
bool operator!=(const Color &lhs, const Color &rhs);

namespace C {
  inline Color gray(float v) { return Color(v, v, v); }
  
  const Color active_text = Color(0.7, 1.0, 0.6);
  const Color inactive_text = Color(0.7, 0.7, 0.4);
  const Color box_border = Color(0.4, 0.4, 0.4);
  
  const Color red = Color(1.0, 0.0, 0.0);
  const Color black = Color(0.0, 0.0, 0.0);
  
  const Color positive = Color(0.1, 1.0, 0.1);
  const Color negative = Color(1.0, 0.1, 0.1);
}

#endif
