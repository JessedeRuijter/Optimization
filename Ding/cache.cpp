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
	if (artificialDelay) delay();
	// return the requested data
	return data[a / SLOTSIZE];
}

// write a cacheline to memory
void Memory::WRITE( address a, CacheLine& line )
{
	// verify that the requested address is the start of a cacheline in memory
	_ASSERT( (a & OFFSETMASK) == 0 );
	// simulate the slowness of RAM
	if (artificialDelay) delay();
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
Cache::Cache( Memory* mem )
{
	slot = new CacheLine[L1CACHESIZE / SLOTSIZE];
	memset(slot, 0, L1CACHESIZE);
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
	for (int i = 0; i < L1CACHESIZE / SLOTSIZE; i++) {
		if (slot[i].value[0] & 0x80) {
			if (((slot[i].value[0] << 24 | slot[i].value[1] << 16 | slot[i].value[2] << 8 | slot[i].value[3]) & 0x3FFFFFFC) == (a & 0x3FFFFFFC)) {
				totalCost += L1ACCESSCOST; hits++;
				return slot[i].value[slot[i].value[3] & 3];
			}
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
void Cache::WRITE( address a, byte value )
{
	//for (int i = 0; i < L1CACHESIZE / SLOTSIZE; i++) {
	//	if (slot[i].value[0] & 0x80) {
	//		if (((slot[i].value[0] << 24 | slot[i].value[1] << 16 | slot[i].value[2] << 8 | slot[i].value[3]) & 0x3FFFFFFC) == (a & 0x3FFFFFFC)) {
	//			slot[i].value[slot[i].value[3] & 3] = value;
	//			slot[i].value[0] = 0x40 & slot[i].value[0];
	//			return;
	//		}
	//	}
	//}

	for (int i = 0; i < L1CACHESIZE / SLOTSIZE; i++) {
		if (!(slot[i].value[0] & 0x80)) { // if niet valid
			slot[i].value[0] = (byte)((a & 0xFF000000 | 0xC0000000) >> 24);
			slot[i].value[1] = (byte)((a & 0x00FF0000) >> 16);
			slot[i].value[2] = (byte)((a & 0x0000FF00) >> 8);
			slot[i].value[3] = (byte)(a & 0x000000FF);

			slot[i].value[slot[i].value[3] & 3] = value;
			return;
		}
	}

	// request a full line from memory
	CacheLine line = memory->READ( a & ADDRESSMASK );
	// change the byte at the correct offset
	line.value[a & OFFSETMASK] = value;
	// write the line back to memory
	memory->WRITE( a & ADDRESSMASK, line );
	// update memory access cost
	totalCost += RAMACCESSCOST;	// TODO: replace by L1ACCESSCOST for a hit
	misses++;					// TODO: replace by hits++ for a hit
}