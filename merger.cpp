
#include "os.h"
#include "args.h"
#include "debug.h"
#include "merger_weapon.h"
#include "merger_bombardment.h"
#include "merger_tanks.h"
#include "merger_glory.h"
#include "merger_util.h"

#include <fstream>
#include <string>

using namespace std;

int main(int argc, char *argv[]) {
  set_exename("merger.exe");
  initFlags(argc, argv, 3);
  
  CHECK(argc == 4);
  
  string type;
  {
    ifstream ifs(argv[1]);
    string line;
    CHECK(getline(ifs, line));
    type = parseCsv(line)[0];
  }
  dprintf("Got type %s\n", type.c_str());
  
  if(type == "WEAPON") {
    mergeWeapon(argv[1], argv[2], argv[3]);
  } else if(type == "BOMBARDMENT") {
    mergeBombardment(argv[1], argv[2], argv[3]);
  } else if(type == "TANKS") {
    mergeTanks(argv[1], argv[2], argv[3]);
  } else if(type == "GLORY") {
    mergeGlory(argv[1], argv[2], argv[3]);
  } else {
    CHECK(0);
  }
}
