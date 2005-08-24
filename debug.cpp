
#include "debug.h"

#ifdef dprintf
#undef dprintf
#endif

#include <cstdio>
#include <vector>
#include <stdarg.h>
#include <assert.h>
using namespace std;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

	OutputDebugString( &(buf[ 0 ]) );

	return 0;

};
