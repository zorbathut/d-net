
#include "dumper.h"

#include "args.h"
#include "stream.h"
#include "stream_file.h"
#include "stream_gz.h"
#include "stream_process_primitive.h"
#include "stream_process_utility.h"
#include "stream_process_vector.h"
#include "stream_process_map.h"
#include "stream_process_rng.h"
#include "stream_process_coord.h"
#include "stream_process_string.h"
#include "audit.h"
#include "dumper_registry.h"
#include "os.h"

DEFINE_string(writeTarget, "dumps/dump-", "Prefix for file dump");
DEFINE_string(readTarget, "", "File to replay from");

const int layout_version = 8;

bool layoutsIdentical(const InputState &lhs, const InputState &rhs) {
  if(lhs.controllers.size() != rhs.controllers.size()) return false;
  for(int i = 0; i < lhs.controllers.size(); i++) {
    if(lhs.controllers[i].axes.size() != rhs.controllers[i].axes.size()) return false;
    if(lhs.controllers[i].keys.size() != rhs.controllers[i].keys.size()) return false;
  }
  return true;
}

BOOST_STATIC_ASSERT(sizeof(float) == 4);  // ugh

RngSeed Dumper::prepare(const RngSeed &option) {
  
  RngSeed seed = option;
  
  if(FLAGS_readTarget != "") {
    dprintf("Reading state record from file %s\n", FLAGS_readTarget.c_str());
    
    istr = new IStreamGz(FLAGS_readTarget);
    CHECK(istr->readInt() == layout_version);
    
    readPacket();
    
    CHECK(read_packet.type == Packet::TYPE_INIT);
    seed = read_packet.init_seed;
    
    setRegistryData(read_packet.init_registry);
  }
  
  write_packet.type = Packet::TYPE_INIT;
  write_packet.init_seed = seed;
  write_packet.init_registry = getRegistryData();
  
  if(FLAGS_writeTarget != "" && (FLAGS_readTarget == "" || FLAGS_writeTarget_OVERRIDDEN)) {
    string fname = FLAGS_writeTarget;
    char timestampbuf[128];
    time_t ctmt = time(NULL);
    strftime(timestampbuf, sizeof(timestampbuf), "%Y%m%d-%H%M%S", gmtime(&ctmt));
    fname = StringPrintf("%s-%s.dnd", fname.c_str(), timestampbuf);
    
    dprintf("Opening %s for write\n", fname.c_str());
    
    ostr = new OStreamGz(fname);
    
    ostr->write(layout_version);
    
    writePacket();
  }
  
  if(istr) {
    readPacket();
    
    CHECK(read_packet.type == Packet::TYPE_LAYOUT);
    
    layout = read_packet.inputstate;
    sources = read_packet.layout_sources;
    primaryid = read_packet.layout_primaryid;
    
    readPacket(); // get the next packet ready
  }
  
  // we'll write controller layout soon
  
  return seed;
}

bool Dumper::is_done() {
  return istr && read_packet.type == Packet::TYPE_EOF;
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
    CHECK(layoutsIdentical(is, layout));
  CHECK(primaryid == -1 || primaryid == in_primary_id);
  
  layout = is;
  sources = in_sources;
  primaryid = in_primary_id;
  
  if(ostr) {
    write_packet.type = Packet::TYPE_LAYOUT;
    
    write_packet.inputstate = layout;
    write_packet.layout_sources = sources;
    write_packet.layout_primaryid = primaryid;
    
    writePacket();
  }
  
  CHECK(primaryid >= 0 && primaryid < layout.controllers.size());
}

void Dumper::get_layout(InputState *is, vector<pair<int, int> > *in_sources, int *in_primary_id) {
  if(istr && read_packet.type == Packet::TYPE_LAYOUT) {
    *is = read_packet.inputstate;
    *in_sources = read_packet.layout_sources;
    *in_primary_id = read_packet.layout_primaryid;
    
    readPacket();
  }
  
  if(ostr && (!layoutsIdentical(layout, *is) || sources != *in_sources || primaryid != *in_primary_id)) {
    write_packet.type = Packet::TYPE_LAYOUT;
    
    write_packet.inputstate = *is;
    write_packet.layout_sources = *in_sources;
    write_packet.layout_primaryid = *in_primary_id;
    
    writePacket();
  }
  
  layout = *is;
  sources = *in_sources;
  primaryid = *in_primary_id;
}

void Dumper::read_audit() {
  if(istr) {
    CHECK(read_packet.type == Packet::TYPE_AUDIT);
  }
  
  read_audit_internal();
}

void Dumper::write_audit() {
  if(ostr) {
    write_packet.type = Packet::TYPE_AUDIT;
  }
  
  write_audit_internal();
}

bool Dumper::has_checksum(bool want_checksum) {
  if(istr) {
    CHECK(read_packet.type == Packet::TYPE_CHECKSUM || read_packet.type == Packet::TYPE_CONTROLS);
    return read_packet.type == Packet::TYPE_CHECKSUM;
  } else {
    return want_checksum;
  }
}

void Dumper::read_checksum_audit() {
  if(istr) {
    CHECK(read_packet.type == Packet::TYPE_CHECKSUM);
  }
  
  read_audit_internal();
}
void Dumper::write_checksum_audit() {
  if(ostr) {
    write_packet.type = Packet::TYPE_CHECKSUM;
  }
  
  write_audit_internal();
}

InputState Dumper::read_input() {
  CHECK(istr);
  
  CHECK(read_packet.type == Packet::TYPE_CONTROLS);
  
  layout = read_packet.inputstate;
  
  for(int i = 0; i < layout.controllers.size(); i++)
    for(int j = 0; j < layout.controllers[i].axes.size(); j++)
      CHECK(abs(layout.controllers[i].axes[j]) <= 1);
  
  readPacket();
  
  return layout;
}

void Dumper::write_input(const InputState &is) {
  CHECK(layoutsIdentical(layout, is));
  
  for(int i = 0; i < is.controllers.size(); i++)
    for(int j = 0; j < is.controllers[i].axes.size(); j++)
      CHECK(abs(is.controllers[i].axes[j]) <= 1);

  if(ostr) {
    write_packet.type = Packet::TYPE_CONTROLS;
    write_packet.inputstate = is;
    for(int i = 0; i < write_packet.inputstate.controllers.size(); i++)
      for(int j = 0; j < write_packet.inputstate.controllers[i].axes.size(); j++)
        CHECK(abs(write_packet.inputstate.controllers[i].axes[j]) <= 1);
    writePacket();
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

void Dumper::readPacket() {
  CHECK(istr); // lolz
  char type;
  if(istr->tryRead(&type)) {
    read_packet.type = Packet::TYPE_EOF;
    return;
  }
  
  if(type == 'I') {
    read_packet.type = Packet::TYPE_INIT;
    CHECK(!istr->tryRead(&read_packet.init_seed));
    CHECK(!istr->tryRead(&read_packet.init_registry));
  } else if(type == 'L') {
    read_packet.type = Packet::TYPE_LAYOUT;
    
    read_packet.inputstate.controllers.resize(istr->readInt());
    read_packet.layout_sources.resize(read_packet.inputstate.controllers.size());
    CHECK(read_packet.inputstate.controllers.size());
    
    for(int i = 0; i < read_packet.inputstate.controllers.size(); i++) {
      read_packet.inputstate.controllers[i].keys.resize(istr->readInt());
      read_packet.inputstate.controllers[i].axes.resize(istr->readInt());
      istr->read(&read_packet.layout_sources[i]);
    }
    read_packet.layout_primaryid = istr->readInt();
  } else if(type == 'C') {
    read_packet.type = Packet::TYPE_CONTROLS;
    
    CHECK(read_packet.inputstate.controllers.size());
    
    istr->read(&read_packet.inputstate.escape.down);
    for(int i = 0; i < read_packet.inputstate.controllers.size(); i++) {
      istr->read(&read_packet.inputstate.controllers[i].menu.x);
      istr->read(&read_packet.inputstate.controllers[i].menu.y);
      istr->read(&read_packet.inputstate.controllers[i].u.down);
      istr->read(&read_packet.inputstate.controllers[i].d.down);
      istr->read(&read_packet.inputstate.controllers[i].l.down);
      istr->read(&read_packet.inputstate.controllers[i].r.down);
      for(int j = 0; j < read_packet.inputstate.controllers[i].keys.size(); j++)
        istr->read(&read_packet.inputstate.controllers[i].keys[j].down);
      for(int j = 0; j < read_packet.inputstate.controllers[i].axes.size(); j++) {
        istr->read(&read_packet.inputstate.controllers[i].axes[j]);
      }
    }
  } else if(type == 'K' || type == 'A') {
    if(type == 'K') {
      read_packet.type = Packet::TYPE_CHECKSUM;
    } else if(type == 'A') {
      read_packet.type = Packet::TYPE_AUDIT;
    } else {
      CHECK(0);
    }
    
    istr->read(&read_packet.audit_data);
  } else {
    CHECK(0);
  }
}
void Dumper::writePacket() {
  CHECK(ostr);
  
  if(write_packet.type == Packet::TYPE_INIT) {
    //dprintf("writing packet I\n");
    ostr->write('I');
    
    ostr->write(write_packet.init_seed);
    ostr->write(write_packet.init_registry);
  } else if(write_packet.type == Packet::TYPE_LAYOUT) {
    //dprintf("writing packet L\n");
    ostr->write('L');
    
    ostr->write(write_packet.inputstate.controllers.size());
    
    for(int i = 0; i < write_packet.inputstate.controllers.size(); i++) {
      ostr->write(write_packet.inputstate.controllers[i].keys.size());
      ostr->write(write_packet.inputstate.controllers[i].axes.size());
      ostr->write(write_packet.layout_sources[i]);
    }
    ostr->write(write_packet.layout_primaryid);
  } else if(write_packet.type == Packet::TYPE_CONTROLS) {
    //dprintf("writing packet C\n");
    ostr->write('C');
      
    ostr->write(write_packet.inputstate.escape.down);
    for(int i = 0; i < write_packet.inputstate.controllers.size(); i++) {
      ostr->write(write_packet.inputstate.controllers[i].menu.x);
      ostr->write(write_packet.inputstate.controllers[i].menu.y);
      ostr->write(write_packet.inputstate.controllers[i].u.down);
      ostr->write(write_packet.inputstate.controllers[i].d.down);
      ostr->write(write_packet.inputstate.controllers[i].l.down);
      ostr->write(write_packet.inputstate.controllers[i].r.down);
      for(int j = 0; j < write_packet.inputstate.controllers[i].keys.size(); j++)
        ostr->write(write_packet.inputstate.controllers[i].keys[j].down);
      for(int j = 0; j < write_packet.inputstate.controllers[i].axes.size(); j++)
        ostr->write(write_packet.inputstate.controllers[i].axes[j]);
    }
  } else if(write_packet.type == Packet::TYPE_CHECKSUM || write_packet.type == Packet::TYPE_AUDIT) {
    if(write_packet.type == Packet::TYPE_CHECKSUM) {
      //dprintf("writing packet K\n");
      ostr->write('K');
    } else if(write_packet.type == Packet::TYPE_AUDIT) {
      //dprintf("writing packet A\n");
      ostr->write('A');
    } else {
      CHECK(0);
    }
    
    ostr->write(write_packet.audit_data);
  } else {
    CHECK(0);
  }
}

void Dumper::read_audit_internal() {
  if(istr) {
    CHECK(read_packet.type == Packet::TYPE_AUDIT || read_packet.type == Packet::TYPE_CHECKSUM);
    
    audit_start_compare(read_packet.audit_data);
    
    readPacket();
  } else {
    audit_start_create();
  }
}

void Dumper::write_audit_internal() {
  audit_register_finished();
  
  if(ostr) {
    CHECK(write_packet.type == Packet::TYPE_AUDIT || write_packet.type == Packet::TYPE_CHECKSUM);
    
    write_packet.audit_data = audit_read_ref();

    writePacket();
  }
  
  audit_finished();
}

/*

    
    layout.controllers.resize(istr->readInt());
    sources.resize(layout.controllers.size());
    CHECK(layout.controllers.size());
    
    for(int i = 0; i < layout.controllers.size(); i++) {
      layout.controllers[i].keys.resize(istr->readInt());
      layout.controllers[i].axes.resize(istr->readInt());
      istr->read(&sources[i]);
    }
    primaryid = istr->readInt();
    
        
*/

/*
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
    */
