
#include "debug.h"

#ifdef printf
#undef printf
#endif

#ifdef dprintf
#undef dprintf
#endif

#include "os.h"

#include <cstdio>
#include <vector>
#include <stdarg.h>
#include <assert.h>
using namespace std;

int frameNumber = -1;

void CrashHandler() { };

void crash() {
    *(int*)0 = 0;
    while(1);
}

int dprintf( const char *bort, ... ) {

    // this is duplicated code with StringPrintf - I should figure out a way of combining these
	static vector< char > buf(2);
	va_list args;

	int done = 0;
	do {
		if( done )
			buf.resize( buf.size() * 2 );
		va_start( args, bort );
		done = vsnprintf( &(buf[ 0 ]), buf.size() - 1,  bort, args );
		assert( done < (int)buf.size() );
		va_end( args );
	} while( done == buf.size() - 1 || done == -1);

	assert( done < (int)buf.size() );

	outputDebugString( &(buf[ 0 ]) );

	return 0;

};
