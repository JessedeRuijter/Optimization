#include "template.h"

unsigned char m[513 * 513];
Pixel colors[4] = { 0xFFFFFF, 0xFF5722, 0xCDDC39, 0x009688 };
int columncounter = 0;
int drawcounter = 0;
Cache* lastCache;

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::Init()
{
	// instantiate simulated memory and cache
	memory = new Memory( 1024 * 1024 * 2); // allocate 2MB (1M is not enough for 32-bit read/writes)
	//cache initialization (all 3 caches must always be initialized)
#ifdef C_ONE
	cache3 = new Cache(memory, L3CACHESIZE, NWAY3, SETMASK3, L3ACCESSCOST);
	cache2 = new Cache(memory, L2CACHESIZE, NWAY2, SETMASK12, L2ACCESSCOST);
	cache1 = new Cache(memory, L1CACHESIZE, NWAY1, SETMASK12, L1ACCESSCOST);
	lastCache = cache1;
#endif
#ifdef C_TWO
	cache3 = new Cache(memory, L3CACHESIZE, NWAY3, SETMASK3, L3ACCESSCOST);
	cache2 = new Cache(memory, L2CACHESIZE, NWAY2, SETMASK12, L2ACCESSCOST);
	cache1 = new Cache(memory, L1CACHESIZE, NWAY1, SETMASK12, L1ACCESSCOST, cache2);
	lastCache = cache2;
#endif
#ifdef C_THREE
	cache3 = new Cache(memory, L3CACHESIZE, NWAY3, SETMASK3, L3ACCESSCOST);
	cache2 = new Cache(memory, L2CACHESIZE, NWAY2, SETMASK12, L2ACCESSCOST, cache3);
	cache1 = new Cache(memory, L1CACHESIZE, NWAY1, SETMASK12, L1ACCESSCOST, cache2);
	lastCache = cache3;
#endif
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
#ifdef B8
	cache1->WRITE(a, value);
#endif
#ifdef B16
	cache1->WRITE16(a, value);
#endif
#ifdef B32
	cache1->WRITE32(a, value);
#endif
	m[a] = value;
}
byte Game::Get( int x, int y )
{
	address a = x + y * 513;
#ifdef B8
	return  cache1->READ(a);
#endif
#ifdef B16
	return  cache1->READ16(a);
#endif
#ifdef B32
	return  cache1->READ32(a);
#endif
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
	// artificial RAM access delay and cost counting are disabled here
	memory->artificialDelay = false, c = cache1->totalCost;
	for (int y = 0; y < 513; y++) for (int x = 0; x < 513; x++)
	{
		Pixel ding = GREY(m[x + y * 513]);
		if (ding == 0)
		{
			int kdjhfgakjdshf = 2;
		}
		screen->Plot(x + 140, y + 60, GREY(m[x + y * 513]));
	}
	memory->artificialDelay = true, cache1->totalCost = c;
	//real-time data visulization
	//cumulative hits and misses update
	cache1->cum_hits += cache1->hits;
	cache1->cum_misses += cache1->misses;
	cache2->cum_hits += cache2->hits;
	cache2->cum_misses += cache2->misses;
	cache3->cum_hits += cache3->hits;
	cache3->cum_misses += cache3->misses;
	// report on memory access cost (134M before your improvements :) )
	printf("total cost: %iM cycles\t", (cache1->totalCost + cache2->totalCost + cache3->totalCost )/ 1000000);
	// report on cache hits and misses
	if (cache1->cum_hits != 0) printf("L1 hit: %f%% \t", (cache1->cum_hits * 100.0 / (cache1->cum_hits + cache1->cum_misses)));
	if (cache2->cum_hits != 0) printf("L2 hit: %f%% \t", (cache2->cum_hits * 100.0 / (cache2->cum_hits + cache2->cum_misses)));
	if (cache3->cum_hits != 0) printf("L3 hit: %f%% \n", (cache3->cum_hits * 100.0 / (cache3->cum_hits + cache3->cum_misses)));
	printf("\n");
#ifdef VISUALIZE
	int total = cache1->hits + cache2->hits + cache3->hits + lastCache->misses;
	if (total != 0)
	{
		int ram = lastCache->misses * DATAHEIGHT / total;
		int h_cache1 = cache1->hits * DATAHEIGHT / total;
		int h_cache2 = cache2->hits * DATAHEIGHT / total;
		int h_cache3 = cache3->hits * DATAHEIGHT / total;
		//skip some ticks maybe
		if (drawcounter % DELAY == 0)
		{
			//fill new column of data array
			for (int i = 0; i < DATAHEIGHT; i++)
			{
				if (i < h_cache1)
					data[columncounter][i] = 1;
				else if (i < h_cache1 + h_cache2)
					data[columncounter][i] = 2;
				else if (i < h_cache1 + h_cache2 + h_cache3)
					data[columncounter][i] = 3;
				else if (i < ram + h_cache3 + h_cache2 + h_cache1)
					data[columncounter][i] = 0;
			}
		}
		//draw data visualization. values -1..3 = nothing, memory, l1, l2, l3
		//Colors: L1 orange, L2 yellow, L3 green, memory white
		for (int y = 0; y < DATAHEIGHT; y++)
			for (int x = 0; x < SCRWIDTH; x++)
				if (data[(SCRWIDTH + columncounter - x) % SCRWIDTH][y] != -1)
					screen->Plot(SCRWIDTH-x, SCRHEIGHT - DATAHEIGHT + y, colors[data[(SCRWIDTH + columncounter - x) % SCRWIDTH][DATAHEIGHT - y - 1]]);
		//columncounter keeps track of which column to start drawing at, rather than shifting the array by 1 column every tick
		if (drawcounter%DELAY==0) columncounter = (columncounter + 1) % SCRWIDTH;
	}
	drawcounter++;
#endif
	//reset hits and misses for next tick (because of reasons)
	cache1->hits = 0;
	cache1->misses = 0;
	cache2->hits = 0;
	cache2->misses = 0;
	cache3->hits = 0;
	cache3->misses = 0;
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