
#include "dumper.h"

#include "args.h"
#include "stream.h"
#include "stream_file.h"
#include "stream_gz.h"
#include "stream_process_primitive.h"
#include "stream_process_utility.h"
#include "stream_process_rng.h"
#include "stream_process_coord.h"
#include "adler32.h"
#include "audit.h"

DEFINE_string(writeTarget, "dumps/dump", "Prefix for file dump");
DEFINE_string(readTarget, "", "File to replay from");

void compareLayouts(const InputState &lhs, const InputState &rhs) {
  CHECK(lhs.controllers.size() == rhs.controllers.size());
  for(int i = 0; i < lhs.controllers.size(); i++) {
    CHECK(lhs.controllers[i].axes.size() == rhs.controllers[i].axes.size());
    CHECK(lhs.controllers[i].keys.size() == rhs.controllers[i].keys.size());
  }
}

RngSeed Dumper::prepare(const RngSeed &option) {
  
  RngSeed seed = option;
  
  if(FLAGS_readTarget != "") {
    dprintf("Reading state record from file %s\n", FLAGS_readTarget.c_str());
    
    istr = new IStreamGz(FLAGS_readTarget);
    CHECK(istr->readInt() == 8);
    istr->read(&seed);
    
    layout.controllers.resize(istr->readInt());
    sources.resize(layout.controllers.size());
    CHECK(layout.controllers.size());
    
    for(int i = 0; i < layout.controllers.size(); i++) {
      layout.controllers[i].keys.resize(istr->readInt());
      layout.controllers[i].axes.resize(istr->readInt());
      istr->read(&sources[i]);
    }
    primaryid = istr->readInt();
  }
  
  if(FLAGS_writeTarget != "" && FLAGS_readTarget == "" || FLAGS_writeTarget_OVERRIDDEN) {
    string fname = FLAGS_writeTarget;
    char timestampbuf[128];
    time_t ctmt = time(NULL);
    strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S", gmtime(&ctmt));
    fname = StringPrintf("%s-%s.dnd", fname.c_str(), timestampbuf);
    
    dprintf("Opening %s for write\n", fname.c_str());
    
    ostr = new OStreamGz(fname);
    
    ostr->write(8);
    ostr->write(seed);
    
    // we'll write controller layout soon
  }
  
  return seed;
}

bool Dumper::is_replaying() {
  return istr;
}

InputState Dumper::get_layout() {
  CHECK(istr);
  return layout;
}

vector<pair<int, int> > Dumper::get_sources() {
  CHECK(istr);
  return sources;
}

int Dumper::get_primary_id() {
  CHECK(istr);
  return primaryid;
}

void Dumper::set_layout(const InputState &is, const vector<pair<int, int> > &in_sources, int in_primary_id) {
  CHECK(!sources.size() || sources == in_sources);
  if(layout.controllers.size())
    compareLayouts(is, layout);
  CHECK(primaryid == -1 || primaryid == in_primary_id);
  
  layout = is;
  sources = in_sources;
  primaryid = in_primary_id;
  
  if(ostr) {
    ostr->write(layout.controllers.size());
    for(int i = 0; i < layout.controllers.size(); i++) {
      ostr->write(layout.controllers[i].keys.size());
      ostr->write(layout.controllers[i].axes.size());
      ostr->write(sources[i]);
    }
    ostr->write(primaryid);
    ostr->flush();
  }
}

void Dumper::read_audit() {
  if(istr) {
    audit_start_compare();
    
    int count = istr->readInt();
    for(int i = 0; i < count; i++)
      audit_start_compare_add(istr->readInt());
    
    audit_start_compare_done();
  } else {
    audit_start_create();
  }
}

void Dumper::write_audit() {
  audit_register_finished();
  
  if(ostr) {
    int count = audit_read_count();
    ostr->write(count);
    for(int i = 0; i < count; i++)
      ostr->write(audit_read_ref());
    ostr->flush();
  }
  
  audit_finished();
}

InputState Dumper::read_input() {
  CHECK(istr);
  bool failure = istr->tryRead(&layout.escape.down);
  if(failure) {
    InputState tt;
    tt.valid = false;
    return tt;
  }
  for(int i = 0; i < layout.controllers.size(); i++) {
    istr->read(&layout.controllers[i].menu.x);
    istr->read(&layout.controllers[i].menu.y);
    istr->read(&layout.controllers[i].u.down);
    istr->read(&layout.controllers[i].d.down);
    istr->read(&layout.controllers[i].l.down);
    istr->read(&layout.controllers[i].r.down);
    for(int j = 0; j < layout.controllers[i].keys.size(); j++)
      istr->read(&layout.controllers[i].keys[j].down);
    for(int j = 0; j < layout.controllers[i].axes.size(); j++)
      istr->read(&layout.controllers[i].axes[j]);
  }
  return layout;
}

void Dumper::write_input(const InputState &is) {
  compareLayouts(layout, is);
  if(ostr) {
    ostr->write(is.escape.down);
    for(int i = 0; i < is.controllers.size(); i++) {
      ostr->write(is.controllers[i].menu.x);
      ostr->write(is.controllers[i].menu.y);
      ostr->write(is.controllers[i].u.down);
      ostr->write(is.controllers[i].d.down);
      ostr->write(is.controllers[i].l.down);
      ostr->write(is.controllers[i].r.down);
      for(int j = 0; j < is.controllers[i].keys.size(); j++)
        ostr->write(is.controllers[i].keys[j].down);
      for(int j = 0; j < is.controllers[i].axes.size(); j++)
        ostr->write(is.controllers[i].axes[j]);
    }
  }
}

Dumper::Dumper() {
  istr = NULL;
  ostr = NULL;
  primaryid = -1;
}

Dumper::~Dumper() {
  delete istr;
  delete ostr;
}
