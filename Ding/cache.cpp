#include "template.h"

// ------------------------------------------------------------------
// SLOW RAM SIMULATOR
// Reads and writes full cachelines (64 bytes), like real RAM
// Has horrible performance, just like real RAM
// Should not be modified for the assignment.
// ------------------------------------------------------------------

// constructor
Memory::Memory( uint size )
{
	data = new CacheLine[size / SLOTSIZE];
	memset( data, 0, size );
	artificialDelay = true;
}

// destructor
Memory::~Memory()
{
	delete data;
}

// read a cacheline from memory
CacheLine Memory::READ( address a )
{
	// verify that the requested address is the start of a cacheline in memory
	_ASSERT( (a & OFFSETMASK) == 0 );
	// simulate the slowness of RAM
//	if (artificialDelay) delay();
	// return the requested data
	return data[a / SLOTSIZE];
}

// write a cacheline to memory
void Memory::WRITE( address a, CacheLine& line )
{
	// verify that the requested address is the start of a cacheline in memory
//	_ASSERT( (a & OFFSETMASK) == 0 );
	int i = a & OFFSETMASK;
	// simulate the slowness of RAM
//	if (artificialDelay) delay();
	// write the supplied data to memory
	data[a / SLOTSIZE] = line;
}

// ------------------------------------------------------------------
// CACHE SIMULATOR
// Currently passes all requests directly to simulated RAM.
// TODO (for a passing grade (6)):
// 1. Build a fully associative cache to speed up requests
// 2. Build a direct mapped cache to improve on the fully associative
//    cache
// 3. Build a N-way set associative cache to improve on the direct
//    mapped cache
// Optional (1 extra point per item, up to a 10):
// 4. Experiment with various eviction policies
// 5. Build a cache hierarchy
// 6. Implement functions for reading/writing 16 and 32-bit values
// 7. Provide detailed statistics on cache performance
// ------------------------------------------------------------------

// constructor
//128 slots in the cache
Cache::Cache(Memory* mem)
{
	slot = new CacheLine*[L1CACHESIZE / SLOTSIZE / NWAY];
	for (int i = 0; i < L1CACHESIZE / SLOTSIZE / NWAY; i++) // 32
	{
		slot[i] = new CacheLine[NWAY];
		for (int j = 0; j < NWAY; j++)
		{
			for (int t = 0; t < SLOTSIZE; t++)
				slot[i][j].value[t] = 0;
		}
	   // memset(slot[i], 0, NWAY);
	}
	memory = mem;
	hits = misses = totalCost = 0;
}

// destructor
Cache::~Cache()
{
	delete slot;
}

// read a single byte from memory
// TODO: minimize calls to memory->READ using caching
byte Cache::READ( address a )
{
	int n = (a & SETMASK) >> 6;
	for (int i = 0; i < NWAY; i++)
	{
		if ((slot[n][i].tag & ADDRESSMASK) == (a & ADDRESSMASK))
		{
			totalCost += L1ACCESSCOST; hits++;
			byte ding = slot[n][i].value[a & OFFSETMASK];
			return ding;
		}
	}

	// request a full line from memory
	CacheLine line = memory->READ( a & ADDRESSMASK );
	// return the requested byte
	byte returnValue = line.value[a & OFFSETMASK];
	WRITE(a, returnValue);

	// update memory access cost
	totalCost += RAMACCESSCOST;	// TODO: replace by L1ACCESSCOST for a hit
	misses++;					// TODO: replace by hits++ for a hit
	return returnValue;
}

// write a single byte to memory
// TODO: minimize calls to memory->WRITE using caching
void Cache::WRITE(address a, byte value)
{
	// request a full line from memory
	CacheLine line = memory->READ(a & ADDRESSMASK);

	line.value[a & OFFSETMASK] = value;
	// write the line back to memory
	memory->WRITE(a & ADDRESSMASK, line);

	int n = (a & SETMASK) >> 6;
	for (int i = 0; i < NWAY; i++)
	{
		if (slot[n][i].valid){
			if ((slot[n][i].tag & ADDRESSMASK) == (a & ADDRESSMASK))
			{
				slot[n][i].value[a & OFFSETMASK] = value;
				slot[n][i].dirty = true;
				return;
			}
		}
	}

	for (int i = 0; i < NWAY; i++)
	{
		if (!slot[n][i].valid){
			slot[n][i].tag = a;
			slot[n][i].value[a & OFFSETMASK] = value;
			slot[n][i].valid = true;
			slot[n][i].dirty = true;
			return;
		}
	}

	int z = EVICTION(n);

	
	// change the byte at the correct offset
	if (slot[n][z].dirty)
	{
		line.value[a & OFFSETMASK] = value;
		// write the line back to memory
		memory->WRITE(a & ADDRESSMASK, line);
		// update memory access cost
		totalCost += RAMACCESSCOST;	// TODO: replace by L1ACCESSCOST for a hit
		misses++;					// TODO: replace by hits++ for a hit
	}

	//	slot[n][z].value[a & OFFSETMASK] = value;
	for (int i = 0; i < 64; i++)
	{
		slot[n][z].value[i] = line.value[i];
	}
	slot[n][z].tag = a;
	slot[n][z].valid = true;
	slot[n][z].dirty = true;
	return;
}

int Cache::EVICTION(int n)
{
	//int ding = rand() % 4;
	return 3;
}