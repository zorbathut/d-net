
#include "debug.h"

#include <cstdio>
#include <vector>
#include <stdarg.h>
using namespace std;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void dprintf( const char *bort, ... ) {
	static vector< char > buf(1);
	va_list args;
	va_start( args, bort );
	int len = vsnprintf( &(buf[ 0 ]), 0, bort, args ) + 1;
	va_end( args );
	buf.resize( len );
	va_start( args, bort );
	vsprintf( &(buf[ 0 ]), bort, args );
	va_end( args );
	OutputDebugString( &(buf[ 0 ]) );
};
