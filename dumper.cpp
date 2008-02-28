
#include "dumper.h"

#include "args.h"
#include "stream.h"
#include "stream_file.h"
#include "stream_process_primitive.h"
#include "stream_process_utility.h"
#include "stream_process_rng.h"
#include "stream_process_coord.h"
#include "adler32.h"

DEFINE_string(writeTarget, "dumps/dump", "Prefix for file dump");
DEFINE_string(readTarget, "", "File to replay from");

static IStream *istr = NULL;
static OStream *ostr = NULL;

static InputState inputstate;
static vector<pair<int, int> > inputsources;
int inputprimaryid = -1;

void confirmLayout(const InputState &is) {
  CHECK(inputstate.controllers.size() == is.controllers.size());
  for(int i = 0; i < inputstate.controllers.size(); i++) {
    CHECK(inputstate.controllers[i].axes.size() == is.controllers[i].axes.size());
    CHECK(inputstate.controllers[i].keys.size() == is.controllers[i].keys.size());
  }
}

RngSeed dumper_init(const RngSeed &option) {
  
  RngSeed seed = option;
  
  if(FLAGS_readTarget != "") {
    dprintf("Reading state record from file %s\n", FLAGS_readTarget.c_str());
    
    istr = new IStreamFile(FLAGS_readTarget);
    CHECK(istr->readInt() == 8);
    istr->read(&seed);
    
    inputstate.controllers.resize(istr->readInt());
    inputsources.resize(inputstate.controllers.size());
    CHECK(inputstate.controllers.size());
    
    for(int i = 0; i < inputstate.controllers.size(); i++) {
      inputstate.controllers[i].keys.resize(istr->readInt());
      inputstate.controllers[i].axes.resize(istr->readInt());
      istr->read(&inputsources[i]);
    }
    inputprimaryid = istr->readInt();
  }
  
  if(FLAGS_writeTarget != "" && FLAGS_readTarget == "" || FLAGS_writeTarget_OVERRIDDEN) {
    string fname = FLAGS_writeTarget;
    char timestampbuf[128];
    time_t ctmt = time(NULL);
    strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S", gmtime(&ctmt));
    fname = StringPrintf("%s-%s.dnd", fname.c_str(), timestampbuf);
    
    dprintf("Opening %s for write\n", fname.c_str());
    
    ostr = new OStreamFile(fname);
    
    ostr->write(8);
    ostr->write(seed);
    
    // we'll write controller layout soon
  }
  
  return seed;
}

bool dumper_is_replaying() {
  return istr;
}

InputState dumper_get_layout() {
  CHECK(istr);
  return inputstate;
}

vector<pair<int, int> > dumper_get_sources() {
  CHECK(istr);
  return inputsources;
}

int dumper_get_primary_id() {
  CHECK(istr);
  return inputprimaryid;
}

void dumper_set_layout(const InputState &is, const vector<pair<int, int> > &in_sources, int in_primary_id) {
  CHECK(!inputsources.size() || inputsources == in_sources);
  if(inputstate.controllers.size())
    confirmLayout(is);
  CHECK(inputprimaryid == -1 || inputprimaryid == in_primary_id);
  
  inputstate = is;
  inputsources = in_sources;
  inputprimaryid = in_primary_id;
  
  if(ostr) {
    ostr->write(inputstate.controllers.size());
    for(int i = 0; i < inputstate.controllers.size(); i++) {
      ostr->write(inputstate.controllers[i].keys.size());
      ostr->write(inputstate.controllers[i].axes.size());
      ostr->write(inputsources[i]);
    }
    ostr->write(inputprimaryid);
    ostr->flush();
  }
}

void dumper_read_adler() {
  if(istr) {
    reg_adler_start_compare();
    
    int count = istr->readInt();
    for(int i = 0; i < count; i++)
      reg_adler_start_compare_add(istr->readInt());
    
    reg_adler_start_compare_done();
  } else {
    reg_adler_start_create();
  }
}

void dumper_write_adler() {
  reg_adler_register_finished();
  
  if(ostr) {
    int count = reg_adler_read_count();
    ostr->write(count);
    for(int i = 0; i < count; i++)
      ostr->write(reg_adler_read_ref());
    ostr->flush();
  }
  
  reg_adler_finished();
}

InputState dumper_read_input() {
  CHECK(istr);
  bool failure = istr->tryRead(&inputstate.escape.down);
  if(failure) {
    InputState tt;
    tt.valid = false;
    return tt;
  }
  for(int i = 0; i < inputstate.controllers.size(); i++) {
    istr->read(&inputstate.controllers[i].menu.x);
    istr->read(&inputstate.controllers[i].menu.y);
    istr->read(&inputstate.controllers[i].u.down);
    istr->read(&inputstate.controllers[i].d.down);
    istr->read(&inputstate.controllers[i].l.down);
    istr->read(&inputstate.controllers[i].r.down);
    for(int j = 0; j < inputstate.controllers[i].keys.size(); j++)
      istr->read(&inputstate.controllers[i].keys[j].down);
    for(int j = 0; j < inputstate.controllers[i].axes.size(); j++)
      istr->read(&inputstate.controllers[i].axes[j]);
  }
  return inputstate;
}

void dumper_write_input(const InputState &is) {
  confirmLayout(is);
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

void dumper_shutdown() {
  delete istr;
  delete ostr;
  istr = NULL;
  ostr = NULL;
}

