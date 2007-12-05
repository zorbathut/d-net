
#include "settings.h"
#include "parse.h"
#include "os.h"
#include "util.h"

#include <fstream>
#include <cstdio>

using namespace std;

Settings::Settings() { };
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
      return;
    }
  }
  
  res_x = 1024;
  res_y = 768;
  res_fullscreen = true;
  res_aspect = 4.0 / 3.0;
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
