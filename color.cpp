
#include "color.h"

#include "parse.h"

using namespace std;

Color::Color() { };
Color::Color(float in_r, float in_g, float in_b) :
  r(in_r), g(in_g), b(in_b) { };
  
float Color::getHue() const {
  float mx = max(r, max(g, b));
  float mn = min(r, min(g, b));
  if(mx == mn)
    return -1; // wheeee
  if(mx == r && g >= b) return 60 * (g - b) / (mx - mn) + 0;
  if(mx == r && g < b) return 60 * (g - b) / (mx - mn) + 360;
  if(mx == g) return 60 * (b - r) / (mx - mn) + 120;
  if(mx == b) return 60 * (r - g) / (mx - mn) + 240;
  CHECK(0);
}

Color colorFromString(const string &str) {
  if(str.size() == 3) {
    // 08f style color
    return Color(fromHex(str[0]) / 15., fromHex(str[1]) / 15., fromHex(str[2]) / 15.);
  } else {
    // 1.0, 0.3, 0.194 style color
    vector<string> toki = tokenize(str, " ,");
    CHECK(toki.size() == 3);
    return Color(atof(toki[0].c_str()), atof(toki[1].c_str()), atof(toki[2].c_str()));
  }
}

Color operator+(const Color &lhs, const Color &rhs) {
  return Color(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b);
}
Color operator*(const Color &lhs, float rhs) {
  return Color(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
}
Color operator/(const Color &lhs, float rhs) {
  return Color(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs);
}

const Color &operator+=(Color &lhs, const Color &rhs) {
  lhs = lhs + rhs;
  return lhs;
}

bool operator==(const Color &lhs, const Color &rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}
