#ifndef DNET_COLOR
#define DNET_COLOR

#include <string>

using namespace std;
 
struct Color {
public:
  float r, g, b;

  float getHue() const;

  Color();
  Color(float in_r, float in_g, float in_b);
};

Color colorFromString(const string &str);

Color operator+(const Color &lhs, const Color &rhs);
Color operator*(const Color &lhs, float rhs);
Color operator/(const Color &lhs, float rhs);

const Color &operator+=(Color &lhs, const Color &rhs);

#endif
