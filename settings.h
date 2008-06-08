#ifndef DNET_SETTINGS
#define DNET_SETTINGS


#include <boost/noncopyable.hpp>
using namespace std;

class Settings : boost::noncopyable {
private:
  Settings();
  ~Settings();

public:
  int res_x;
  int res_y;
  bool res_fullscreen;
  float res_aspect;

  static Settings &get_instance();

  void load();  // goes to defaults if it can't load
  void save() const;
};

#endif
