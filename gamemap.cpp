
#include "gamemap.h"

#include "gfx.h"

void Gamemap::render() const {
	setColor( 0.5f, 0.5f, 0.5f );
	drawLinePath( vertices, 0.5 );
}
void Gamemap::addCollide( Collider *collider ) const {
	for( int i = 0; i < vertices.size(); i += 2 )
		collider->add( vertices[ i ], vertices[ i + 1 ], vertices[ ( i + 2 ) % vertices.size() ], vertices[ ( i + 3 ) % vertices.size() ] );
}

Gamemap::Gamemap() {

	// bl corner
	vertices.push_back( 5 );
	vertices.push_back( 5 );

	// bottom bump
	{
		vertices.push_back( 40 );
		vertices.push_back( 5 );

		vertices.push_back( 50 );
		vertices.push_back( 24 );

		vertices.push_back( 70 );
		vertices.push_back( 19 );

		vertices.push_back( 80 );
		vertices.push_back( 5 );
	}

	// br corner
	vertices.push_back( 120 );
	vertices.push_back( 5 );

	// right wedge
	{
		vertices.push_back( 120 );
		vertices.push_back( 15 );

		vertices.push_back( 110 );
		vertices.push_back( 15 );

		vertices.push_back( 120 );
		vertices.push_back( 40 );
	}

	// tr corner
	{
		vertices.push_back( 120 );
		vertices.push_back( 80 );

		vertices.push_back( 115 );
		vertices.push_back( 95 );
	}

	// top depression
	{
		vertices.push_back( 90 );
		vertices.push_back( 95 );

		vertices.push_back( 70 );
		vertices.push_back( 85 );

		vertices.push_back( 55 );
		vertices.push_back( 88 );

		vertices.push_back( 45 );
		vertices.push_back( 97 );

		vertices.push_back( 30 );
		vertices.push_back( 95 );
	}

	// tl corner
	{
		vertices.push_back( 5 );
		vertices.push_back( 95 );
	}

	// left bumpiness
	{
		vertices.push_back( 5 );
		vertices.push_back( 80 );

		vertices.push_back( 20 );
		vertices.push_back( 74 );

		vertices.push_back( 35 );
		vertices.push_back( 60 );

		vertices.push_back( 38 );
		vertices.push_back( 40 );

		vertices.push_back( 33 );
		vertices.push_back( 58 );

		vertices.push_back( 17 );
		vertices.push_back( 70 );

		vertices.push_back( 10 );
		vertices.push_back( 70 );

		vertices.push_back( 5 );
		vertices.push_back( 62 );
	}
}
