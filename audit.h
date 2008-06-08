#ifndef DNET_AUDIT
#define DNET_AUDIT

#include "adler32.h"

#include <vector>
using namespace std;



// Start initializing.
void audit_start_create();
void audit_start_compare(const vector<unsigned long> &data);

// Ignore audits while this is on the stack
class AuditIgnore {
public:
  AuditIgnore();
  ~AuditIgnore();
};

// Register adlers - that can be either "compare" or "create"
#define audit(x) audit_worker(x, __FILE__, __LINE__)
void audit_worker(const Adler32 &adl, const char *file, int line);
void audit_worker(unsigned long ul, const char *file, int line);

// Pause functions
void audit_pause();
void audit_unpause();

// Done registering
void audit_register_finished();

// Once we're done, we can use this to output what we originally compared to, or what we created.
int audit_read_count();
vector<unsigned long> audit_read_ref();

// Eventually we're done. Init must happen after this.
void audit_finished();

#endif
