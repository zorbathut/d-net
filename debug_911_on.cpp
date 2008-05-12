
#include "debug_911.h"

#include "debug.h"
#include "os.h"
#include "util.h"

#include <string>
#include <fstream>

using namespace std;

string writeDebug() {
  string writedest = getTempFilename();
  
  ofstream ofs(writedest.c_str());
  if(!ofs)
    return "";
  
  for(int i = 0; i < dbgrecord().size(); i++) {
    while(dbgrecord()[i].size() && dbgrecord()[i][dbgrecord()[i].size() - 1] == '\n')
      dbgrecord()[i].erase(dbgrecord()[i].begin() + dbgrecord()[i].size() - 1);
    
    ofs << dbgrecord()[i] << '\n';
  }
  
  if(ofs)
    return writedest;
  else
    return "";
};

void Prepare911(const char *crashfname, int crashline) {
  string fname = writeDebug();
  if(!fname.size()) {
    dprintf("Failed to write file, aborting with a vengeance\n");
    return; // welp
  } else {
    dprintf("Wrote debug dump to %s\n", fname.c_str());
  }
  
  SpawnProcess(StringPrintf("build/reporter.exe d-net \"%s\" %s %d %d", fname.c_str(), crashfname, crashline, exesize()));
};
