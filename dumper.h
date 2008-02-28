#ifndef DNET_DUMPER
#define DNET_DUMPER

#include "input.h"
#include "rng.h"

// Init-related functions
// Set up input and output streams
RngSeed dumper_init(const RngSeed &option);

// See if we're replaying an input stream, and get the layout if we are
bool dumper_is_replaying();
InputState dumper_get_layout();
vector<pair<int, int> > dumper_get_sources();
int dumper_get_primary_id();

// Alternatively, write our layout
void dumper_set_layout(const InputState &is, const vector<pair<int, int> > &sources, int primary_id);

// Handle adler queries properly
void dumper_read_adler(); // Reads adler sets from the file and primes the adler storage system with 'em
void dumper_write_adler();  // Writes anything in the adler system to the file

// Handle the actual movement input or output
InputState dumper_read_input();
void dumper_write_input(const InputState &is);

// close everything, etc
void dumper_shutdown();

#endif
