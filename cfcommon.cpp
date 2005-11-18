
#include "cfcommon.h"

/*************
 * Fast sin/cos
 */

float sin_table[ SIN_TABLE_SIZE + 1 ];

class sinTableMaker {
public:
	sinTableMaker() {
		for( int i = 0; i < SIN_TABLE_SIZE + 1; i++ ) {
			sin_table[ i ] = sin( i * PI / SIN_TABLE_SIZE / 2 );
		}

		//for( float x = 0; x < PI * 2; x += 0.01 )
			//dprintf( "%f-%f, %f-%f\n", fsin( x ), sin( x ), fcos( x ), cos( x ) );
	}
};

sinTableMaker sinInit;
