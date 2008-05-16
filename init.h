#ifndef DNET_INIT
#define DNET_INIT

#include <boost/function.hpp>

void initProgram(int *argc, char ***argv);

class InitterRegister {
public:
  InitterRegister(boost::function<void (int *, char ***)> func, int priority);
};

#define ADD_INITTER(func, priority)   namespace { InitterRegister inr##__LINE__(func, priority); }

#endif
