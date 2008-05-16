
#include "init.h"

#include <map>

using namespace std;

multimap<int, boost::function<void (int*, char ***)> > &getFuncs() {
  static multimap<int, boost::function<void (int*, char ***)> > it;
  return it;  // return it :D
}

void initProgram(int *argc, char ***argv) {
  const multimap<int, boost::function<void (int*, char ***)> > &mp = getFuncs();
  for(multimap<int, boost::function<void (int*, char ***)> >::const_iterator itr = mp.begin(); itr != mp.end(); itr++) {
    itr->second(argc, argv);
  }
}

InitterRegister::InitterRegister(boost::function<void (int *, char ***)> func, int priority) {
  getFuncs().insert(make_pair(priority, func));
}

