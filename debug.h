#ifndef DNET_DEBUG
#define DNET_DEBUG

int dprintf( const char *bort, ... ) __attribute__((format(printf,1,2)));
//#define dprintf if( 1 ) { } else dprintf

#endif
