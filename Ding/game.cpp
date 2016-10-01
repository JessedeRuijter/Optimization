#include "template.h"

unsigned char m[513 * 513];
Pixel colors[4] = { 0xFFFFFF, 0x0000FF, 0x00FF00, 0xFF0000 };
int columncounter = 0;

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// instantiate simulated memory and cache
	memory = new Memory( 1024 * 1024 ); // allocate 1MB
	cache3 = new Cache(memory, L3CACHESIZE, NWAY3, SETMASK3, L3ACCESSCOST);
	cache2 = new Cache(memory, L2CACHESIZE, NWAY2, SETMASK12, L2ACCESSCOST, cache3);
	cache1 = new Cache(memory, L1CACHESIZE, NWAY1, SETMASK12, L1ACCESSCOST, cache2);
	//instantiate data visualizer on -1 (= don't draw)
	for (int i = 0; i < DATAHEIGHT; i++)
		for (int n = 0; n < SCRWIDTH; n++)
			data[n][i] = -1;
	// intialize fractal algorithm
	srand( 1000 );
	Set( 0, 0, IRand( 255 ) );
	Set( 512, 0, IRand( 255 ) );
	Set( 0, 512, IRand( 255 ) );
	Set( 512, 512, IRand( 255 ) );
	// put first subdivision task on stack
	taskPtr = 0;
	Push( 0, 0, 512, 512, 256 );
}

// -----------------------------------------------------------
// Helper functions for reading and writing data
// -----------------------------------------------------------
void Game::Set( int x, int y, byte value )
{
	address a = x + y * 513;
	cache1->WRITE(a, value);
	m[a] = value;
}
byte Game::Get( int x, int y )
{
	address a = x + y * 513;
	return cache1->READ( a );	
}

// -----------------------------------------------------------
// Recursive subdivision of the height map
// -----------------------------------------------------------
void Game::Subdivide( int x1, int y1, int x2, int y2, int scale )
{
	// termination
	if ((x2 - x1) == 1) return;
	// calculate diamond vertex positions
	int cx = (x1 + x2) / 2, cy = (y1 + y2) / 2;
	// set vertices
	if (Get( cx, y1 ) == 0) Set( cx, y1, (Get( x1, y1 ) + Get( x2, y1 )) / 2 + IRand( scale ) - scale / 2 );
	if (Get( cx, y2 ) == 0) Set( cx, y2, (Get( x1, y2 ) + Get( x2, y2 )) / 2 + IRand( scale ) - scale / 2 );
	if (Get( x1, cy ) == 0) Set( x1, cy, (Get( x1, y1 ) + Get( x1, y2 )) / 2 + IRand( scale ) - scale / 2 );
	if (Get( x2, cy ) == 0) Set( x2, cy, (Get( x2, y1 ) + Get( x2, y2 )) / 2 + IRand( scale ) - scale / 2 );
	if (Get( cx, cy ) == 0) Set( cx, cy, (Get( x1, y1 ) + Get( x2, y2 )) / 2 + IRand( scale ) - scale / 2 );
	// push new tasks
	Push( x1, y1, cx, cy, scale / 2 );
	Push( cx, y1, x2, cy, scale / 2 );
	Push( x1, cy, cx, y2, scale / 2 );
	Push( cx, cy, x2, y2, scale / 2 );
}

// -----------------------------------------------------------
// Main game tick function
// -----------------------------------------------------------
void Game::Tick( float dt )
{
	// execute 128 tasks per frame
	for( int i = 0; i < 128; i++ )
	{
		// execute one subdivision task
		if (taskPtr == 0) break;
		int x1 = task[--taskPtr].x1, x2 = task[taskPtr].x2;
		int y1 = task[taskPtr].y1, y2 = task[taskPtr].y2;
		Subdivide( x1, y1, x2, y2, task[taskPtr].scale );
	}
	// report on memory access cost (134M before your improvements :) )
	printf( "total memory access cost: %iM cycles\n", cache1->totalCost / 1000000);
	//printf("hits: %i \n", cache->hits);
	// visualize current state
	// artificial RAM access delay and cost counting are disabled here
	memory->artificialDelay = false, c = cache1->totalCost;
	for (int y = 0; y < 513; y++) for (int x = 0; x < 513; x++)
		screen->Plot(x + 140, y + 60, GREY(m[x + y * 513]));
	memory->artificialDelay = true, cache1->totalCost = c;
	//real-time data visulization
#ifdef VISUALIZE
	int total = cache1->hits + cache2->hits + cache3->hits + cache3->misses;
	if (total != 0)
	{
		int ram = cache3->misses * DATAHEIGHT / total;
		int h_cache1 = cache1->hits * DATAHEIGHT / total;
		int h_cache2 = cache2->hits * DATAHEIGHT / total;
		int h_cache3 = cache3->hits * DATAHEIGHT / total;
		//fill new column of data array
		for (int i = 0; i < DATAHEIGHT; i++)
		{
			if (i < h_cache1)
				data[columncounter][i] = 1;
			else if (i < h_cache1 + h_cache2)
				data[columncounter][i] = 2;
			else if (i < h_cache1+ h_cache2 + h_cache3)
				data[columncounter][i] = 3;
			else if (i < ram + h_cache3 + h_cache2 + h_cache1)
				data[columncounter][i] = 0;
		}
		//draw data visualization. values -1..3 = nothing, memory, l1, l2, l3
		//Colors: L1 blue, L2 green, L3 red, memory white
		for (int y = 0; y < DATAHEIGHT; y++)
			for (int x = 0; x < SCRWIDTH; x++)
				if (data[(SCRWIDTH + columncounter - x) % SCRWIDTH][y] != -1)
					screen->Plot(x, y, colors[data[(SCRWIDTH + columncounter - x) % SCRWIDTH][DATAHEIGHT - y - 1]]);
		//reset hits and misses for next tick
		cache1->hits = 0;
		cache1->misses = 0;
		cache2->hits = 0;
		cache2->misses = 0;
		cache3->hits = 0;
		cache3->misses = 0;
		//columncounter keeps track of which column to start drawing at, rather than shifting the array by 1 column every tick
		columncounter = (columncounter + 1) % SCRWIDTH;
	}
#endif
}

// -----------------------------------------------------------
// Clean up
// -----------------------------------------------------------
void Game::Shutdown()
{
	delete memory;
	delete cache1;
	delete cache2;
	delete cache3;
}