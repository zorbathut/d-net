
#include "settings.h"
#include "args.h"
#include "os.h"
#include "os_ui.h"
#include "parse.h"
#include "util.h"

#include <fstream>
using namespace std;



DEFINE_int(resolutionx, -1, "X resolution");
DEFINE_int(resolutiony, -1, "Y resolution");
DEFINE_bool(fullscreen, false, "Fullscreen");
DEFINE_float(aspect, -1, "Aspect");

Settings::Settings() {
  res_x = getScreenRes().first;
  res_y = getScreenRes().second;
  res_fullscreen = true;
  res_aspect = (float)res_x / (float)res_y;
};
Settings::~Settings() { };

Settings &Settings::get_instance() {
  static Settings set;
  return set;
}

void Settings::load() {
  ifstream ifs((getConfigDirectory() + "settings").c_str());
  if(ifs) {
    kvData kvd;
    getkvData(ifs, &kvd);
    if(ifs) {
      CHECK(kvd.category == "graphics");
      res_x = atoi(kvd.consume("x").c_str());
      res_y = atoi(kvd.consume("y").c_str());
      res_fullscreen = atoi(kvd.consume("fullscreen").c_str());
      res_aspect = atof(kvd.consume("aspect").c_str());
    }
  }
  
  if(FLAGS_resolutionx_OVERRIDDEN != FS_DEFAULT)
    res_x = FLAGS_resolutionx;
  if(FLAGS_resolutiony_OVERRIDDEN != FS_DEFAULT)
    res_y = FLAGS_resolutiony;
  if(FLAGS_fullscreen_OVERRIDDEN != FS_DEFAULT)
    res_fullscreen = FLAGS_fullscreen;
  if(FLAGS_aspect_OVERRIDDEN != FS_DEFAULT)
    res_aspect = FLAGS_aspect;
}

void Settings::save() const {
  makeConfigDirectory();
  
  ofstream ofs((getConfigDirectory() + "settings").c_str());
  CHECK(ofs);
  
  kvData kvd;
  kvd.category = "graphics";
  kvd.kv["x"] = StringPrintf("%d", res_x);
  kvd.kv["y"] = StringPrintf("%d", res_y);
  kvd.kv["fullscreen"] = StringPrintf("%d", res_fullscreen);
  kvd.kv["aspect"] = StringPrintf("%f", res_aspect);
  
  ofs << stringFromKvData(kvd);
}
