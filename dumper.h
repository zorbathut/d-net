#ifndef DNET_DUMPER
#define DNET_DUMPER

#include "input.h"
#include "rng.h"
#include "stream.h"

/* Okay, packet ordering:

'I' - Init
'L' - Layout
'C' - Controls
'K' - Full checksum
'A' - Standard adler

IL(AK?CL?)*

Dumper handles a lot of stuff behind the scenes - mostly, you just ask it what it has, giving suggestions as to what you think it should want. It will override you if it's in read mode, and conform otherwise. */

class Dumper {
public:
  
  // Set up input and output streams. Pass it a potential RNG seed, it returns the one you should actually use.
  RngSeed prepare(const RngSeed &option);

  // We have all our input complete. Yay.
  bool is_done();

  // Tells you whether or not you're replaying an existing stream.
  bool is_replaying();
  InputState get_layout();
  vector<pair<int, int> > get_sources();
  int get_primary_id();
  
  // Write our layout
  void set_layout(const InputState &is, const vector<pair<int, int> > &sources, int primary_id);
  
  // Handle adler queries properly
  void read_audit(); // Reads adler sets from the file and primes the global audit storage system with 'em
  void write_audit();  // Writes anything in the global audit system to the file

  // Handle checksum queries properly
  bool has_checksum(bool want_checksum);
  void read_checksum_audit();
  void write_checksum_audit();
  
  // Handle the actual movement input or output (if it has a checksum, you must deal with that first!)
  InputState read_input();
  void write_input(const InputState &is);
  
  Dumper();
  ~Dumper();
  
private:
  struct Packet {
    enum Type { TYPE_INIT, TYPE_LAYOUT, TYPE_CONTROLS, TYPE_CHECKSUM, TYPE_AUDIT, TYPE_EOF } type;
    
    RngSeed init_seed;
    
    vector<pair<int, int> > layout_sources;
    int layout_primaryid;
    
    InputState inputstate;
    
    vector<unsigned long> audit_data; // we re-use this for checksum
    
    Packet() : init_seed(0) { };
  };
  
  Packet read_packet;
  Packet write_packet;
  
  void readPacket();
  void writePacket();
  
  IStream *istr;
  OStream *ostr;

  InputState layout;
  vector<pair<int, int> > sources;
  int primaryid;
  
  void read_audit_internal();
  void write_audit_internal();
};

#endif
