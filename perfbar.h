#ifndef DNET_PERFBAR
#define DNET_PERFBAR

#include "color.h"

#include <boost/noncopyable.hpp>
using namespace std;


namespace PBC {
  const Color tick = Color(0.2, 0.2, 0.2);
  
  const Color gametick = Color(0.4, 0.4, 0.4);
  const Color gameticktankmovement = Color(1.0, 0.5, 0.5);
  const Color gametickcollider = Color(0.0, 0.0, 1.0);
  const Color gametickcollidersetup = Color(0.2, 1.0, 1.0);
  const Color gametickcollidersetupcleanup = Color(1.0, 0.2, 0.2);
  const Color gametickcollidersetupgamemap = Color(0.2, 1.0, 0.2);
  const Color gametickcollidersetupentities = Color(0.2, 0.2, 1.0);
  const Color gametickcolliderprocess = Color(1.0, 0.2, 1.0);
  const Color gametickcolliderresults = Color(0.2, 1.0, 0.2);
  const Color gametickpbwe = Color(0.5, 1.0, 0.5);
  
  const Color persistent = Color(0.2, 0.5, 0.2);
  const Color shop = Color(1.0, 0.2, 0.2);
  const Color shopnormalize = Color(1.0, 1.0, 0.2);
  
  const Color render = Color(0.9, 0.9, 0.9);
  const Color rinit = Color(0.1, 0.1, 0.1);
  const Color rendermeta = Color(0.2, 0.9, 0.2);
  const Color rendershop = Color(0.9, 0.9, 0.2);
  const Color rendershopnode = Color(0.6, 0.1, 0.1);
  
  const Color gfxtext = Color(0.1, 0.1, 0.4);
  const Color gfxsolid = Color(0.1, 0.4, 0.1);
  
  const Color checksum = Color(0.6, 0.2, 0.2);
};

class PerfStack : boost::noncopyable {
public:
  PerfStack(const Color &col);
  ~PerfStack();
};

void startPerformanceBar();
void drawPerformanceBar();

#endif
