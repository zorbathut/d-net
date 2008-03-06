#ifndef DNET_DUMPER
#define DNET_DUMPER

#include "input.h"
#include "rng.h"
#include "stream.h"

class Dumper {
public:
  
  // Set up input and output streams. Pass it a potential RNG seed, it returns the one you should actually use.
  RngSeed prepare(const RngSeed &option);

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
  
  // Handle the actual movement input or output
  InputState read_input();
  void write_input(const InputState &is);
  
  Dumper();
  ~Dumper();
  
private:
  IStream *istr;
  OStream *ostr;

  InputState layout;
  vector<pair<int, int> > sources;
  int primaryid;
};

#endif
