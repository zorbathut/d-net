
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

	static vector< char > buf(1);
	va_list args;

	va_start( args, bort );
	int len = vsnprintf( &(buf[ 0 ]), 0, bort, args ) + 1;
	va_end( args );

	buf.resize( len + 1 );

	int done = 0;
	do {
		if( done )
			buf.resize( buf.size() * 2 );
		va_start( args, bort );
		done = vsnprintf( &(buf[ 0 ]), buf.size(),  bort, args );
		va_end( args );
	} while( done == buf.size() - 1 );

	assert( done <= len );

	OutputDebugString( &(buf[ 0 ]) );

	return 0;

};
