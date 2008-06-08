
#include "perfbar.h"

#include "args.h"
#include "gfx.h"
#include "os_timer.h"

#include <stack>

using namespace std;

DEFINE_bool(perfbar, false, "Enable performance bar");

class PerfChunk {
public:
  float start;
  float end;
  Color col;
  int indent;
};

static vector<PerfChunk> pchunks;

static stack<pair<const PerfStack *, PerfChunk> > pstack;

static Timer timer;

PerfStack::PerfStack(const Color &col) {
  if(FLAGS_perfbar) {
    PerfChunk pch;
    pch.start = timer.ticksElapsed() / (float)timer.getFrameTicks();
    pch.col = col;
    pch.indent = pstack.size();
    pstack.push(make_pair(this, pch));
  }
}
PerfStack::~PerfStack() {
  if(FLAGS_perfbar) {
    CHECK(pstack.top().first == this);
    PerfChunk pch = pstack.top().second;
    pstack.pop();
    pch.end = timer.ticksElapsed() / (float)timer.getFrameTicks();
    pchunks.push_back(pch);
  }
}

void startPerformanceBar() {
  if(FLAGS_perfbar) {
    CHECK(pstack.empty());
    pchunks.clear();
    timer = Timer();
  }
}

void drawPerformanceBar() {
  if(FLAGS_perfbar) {
    setZoomVertical(0, 0, 1.2);
    const float xmarg = 0.02;
    const float xstart = getZoom().ex - xmarg * 2;
    int megindent = 0;
    for(int i = 0; i < pchunks.size(); i++) {
      float xps = xstart - xmarg * pchunks[i].indent;
      setColor(pchunks[i].col);
      drawLine(Float4(xps, pchunks[i].start, xps, pchunks[i].end), xmarg * 0.9);
      megindent = max(megindent, pchunks[i].indent);
    }
    setColor(C::gray(1.0));
    drawLine(Float4(xstart - xmarg * (megindent + 2), 1.0, getZoom().ex, 1.0), xmarg / 2);
  }
}
