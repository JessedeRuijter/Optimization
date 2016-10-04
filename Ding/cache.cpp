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
	//_ASSERT( (a & OFFSETMASK) == 0 ); (broken by 16 and 32 bit reads due to smaller offset)
	// simulate the slowness of4 RAM
	if (artificialDelay) delay();
	// return the requested data
	return data[a / (SLOTSIZE/4)]; //use 4 times less addresses to scale for up to 32-bit read and writes
}

// write a cacheline to memory
void Memory::WRITE( address a, CacheLine& line )
{
	// verify that the requested address is the start of a cacheline in memory
	//_ASSERT( (a & OFFSETMASK) == 0 ); (broken by 16 and bit writes due to smaller offset)
	// simulate the slowness of RAM
	if (artificialDelay) delay();
	// write the supplied data to memory
	data[a / (SLOTSIZE/4)] = line; //use 4 times less addresses to scale for up to 32-bit read and writes
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
Cache::Cache(Memory* mem, int size, int Nway, int SetMask, int Cost, Cache* c)
{
	setMask = SetMask;
	nway = Nway;
	cost = Cost;
	slot = new CacheLine*[size / SLOTSIZE / nway];
	for (int i = 0; i < size / SLOTSIZE / nway; i++)
	{
		slot[i] = new CacheLine[nway];
	}
	memory = mem;
	nextCache = c;
	hits = misses = totalCost = cum_hits = cum_misses = 0;
}

// destructor
Cache::~Cache()
{
	delete slot;
}

// read a single byte from cache
byte Cache::READ( address a )
{
	int n = (a & setMask) >> 6; //bitshift offset bits (always 64 with slotsize 64)
	for (int i = 0; i < nway; i++)
	{
			slot[n][i].age++; //LRU
		if ((slot[n][i].tag & ADDRESSMASK) == (a & ADDRESSMASK))
		{
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return slot[n][i].value[a & OFFSETMASK];
		}
	}

	// Read from next cache if exists, otherwise from memory
	CacheLine line;
	byte returnValue;
	if (nextCache)
	{
		returnValue = nextCache->READ(a);
	}
	else //read from memory
	{
		line = memory->READ(a & ADDRESSMASK);
		returnValue = line.value[a & OFFSETMASK];
		totalCost += RAMACCESSCOST;
	}

	WRITE(a, returnValue);
	misses++;
	return returnValue;
}

//read an entire cacheline from cache
CacheLine Cache::READLINE(address a, int mask)
{
	int n = (a & setMask) >> 6;
	for (int i = 0; i < nway; i++)
	{
		slot[n][i].age++; //LRU
		if ((slot[n][i].tag & mask) == (a & mask))
		{
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return slot[n][i];
		}
	}

	// Read from next cache if exists, otherwise from memory
	CacheLine line;
	if (nextCache)
	{
		line = nextCache->READLINE(a, mask);
	}
	else
	{
		line = memory->READ(a & mask);
		totalCost += RAMACCESSCOST;
	}

	WRITELINE(a, line, mask);
	misses++;
	return line;
}

// write a single byte to cache
void Cache::WRITE(address a, byte value)
{
	int n = (a & setMask) >> 6;

	for (int i = 0; i < nway; i++)
		slot[n][i].age++; //LRU

	for (int i = 0; i < nway; i++)
	{
		if (slot[n][i].valid){
			if ((slot[n][i].tag & ADDRESSMASK) == (a & ADDRESSMASK))
			{
				slot[n][i].value[a & OFFSETMASK] = value;
				slot[n][i].dirty = true;
				totalCost += cost; hits++;
				slot[n][i].age = 0; //LRU
				slot[n][i].n_uses++; //LFU
				return;
			}
		}
	}

	// request a full line from memory/cache
	CacheLine line;
	if (nextCache)
		line = nextCache->READLINE(a & ADDRESSMASK, ADDRESSMASK);
	else
		line = memory->READ(a & ADDRESSMASK);

	//if invalid, write entire line
	for (int i = 0; i < nway; i++)
	{
		if (!slot[n][i].valid){
			slot[n][i].tag = a;

			for (int t = 0; t < SLOTSIZE; t++)
				slot[n][i].value[t] = line.value[t];
			slot[n][i].value[a & OFFSETMASK] = value;

			slot[n][i].valid = true;
			slot[n][i].dirty = true;
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return;
		}
	}

	int z = EVICTION(n);

	if (slot[n][z].dirty)
	{
		// write the line back to memory or next cache if dirty
		if (nextCache)
		{
			nextCache->WRITELINE(slot[n][z].tag & ADDRESSMASK, slot[n][z], ADDRESSMASK);
		}
		else
		{
			memory->WRITE(slot[n][z].tag & ADDRESSMASK, slot[n][z]);
			totalCost += RAMACCESSCOST;
		}
	}

	//copy entire cacheline
	for (int t = 0; t < SLOTSIZE; t++)
		slot[n][z].value[t] = line.value[t];
	slot[n][z].value[a & OFFSETMASK] = value;
	slot[n][z].tag = a;
	slot[n][z].valid = true;
	slot[n][z].dirty = true;
	misses++;
	slot[n][z].age = 0; //LRU
	slot[n][z].n_uses++; //LFU
	return;
}

//write an entire line to cache
void Cache::WRITELINE(address a, CacheLine& value, int mask)
{
	int n = (a & setMask) >> 6;
	for (int i = 0; i < nway; i++)
		slot[n][i].age++; //LRU

	for (int i = 0; i < nway; i++)
	{
		if (slot[n][i].valid){
			if ((slot[n][i].tag & mask) == (a & mask))
			{
				for (int t = 0; t < SLOTSIZE; t++)
					slot[n][i].value[t] = value.value[t];
				slot[n][i].dirty = true;
				totalCost += cost; hits++;
				slot[n][i].age = 0; //LRU
				slot[n][i].n_uses++; //LFU
				return;
			}
		}
	}

	for (int i = 0; i < nway; i++)
	{
		if (!slot[n][i].valid){
			for (int t = 0; t < SLOTSIZE; t++)
				slot[n][i].value[t] = value.value[t];
			slot[n][i].tag = a;
			slot[n][i].valid = true;
			slot[n][i].dirty = true;
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return;
		}
	}

	int z = EVICTION(n);

	// request a full line from memory/cache
	CacheLine line;
	if (nextCache)
		line = nextCache->READLINE(a & mask, mask);
	else
		line = memory->READ(a & mask);

	if (slot[n][z].dirty)
	{
		// write the line back to memory or next cache
		if (nextCache)
		{
			nextCache->WRITELINE(slot[n][z].tag & mask, slot[n][z], mask);
		}
		else
		{
			memory->WRITE(slot[n][z].tag & mask, slot[n][z]);
			totalCost += RAMACCESSCOST;
		}
	}

	//copy entire cacheline
	for (int t = 0; t < SLOTSIZE; t++)
		slot[n][z].value[t] = value.value[t];
	slot[n][z].tag = a;
	slot[n][z].valid = true;
	slot[n][z].dirty = true;
	misses++;
	slot[n][z].age = 0; //LRU
	slot[n][z].n_uses++; //LFU
	return;
}

int Cache::EVICTION(int n)
{
	int i;
	#ifdef EV_RANDOM
		i = rand() % 4;
	#endif

	#ifdef EV_LRU
		int max = 0;
		for (int t = 0; t < nway; t++)
		{
			if (slot[n][t].age > max)
			{
				max = slot[n][t].age;
				i = t;
			}
		}
	#endif

	#ifdef EV_MRU
		int min = 999999;
		for (int t = 0; t < nway; t++)
		{
			if (slot[n][t].age < min)
			{
				min = slot[n][t].age;
				i = t;
			}
		}
	#endif

	#ifdef EV_LFU
		int min = 999999;
		for (int t = 0; t < nway; t++)
		{
			if (slot[n][t].n_uses < min)
			{
				min = slot[n][t].n_uses;
				i = t;
			}
		}
	#endif
#ifdef EV_CONST
		{
			i = 0;
		}
#endif

	return i;

}

//read 16-bit data type from cache
__int16 Cache::READ16(address a){
	int n = (a & setMask) >> 6; //bitshift offset bits
	int offset = (a & OFFSETMASK16) * 2;
	for (int i = 0; i < nway; i++)
	{
		slot[n][i].age++; //LRU
		if ((slot[n][i].tag & ADDRESSMASK16) == (a & ADDRESSMASK16))
		{
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return (__int16)(((__int16)slot[n][i].value[offset] << 8) | (__int16)slot[n][i].value[offset + 1]);
		}
	}

	// Read from next cache if exists, otherwise from memory
	CacheLine line;
	__int16 returnValue;
	if (nextCache)
	{
		returnValue = nextCache->READ16(a);
	}
	else //read from memory
	{
		line = memory->READ(a & ADDRESSMASK16);
		returnValue = (__int16)((__int16)(line.value[offset] << 8) | (__int16)line.value[offset + 1]);
		totalCost += RAMACCESSCOST;
	}
	WRITE16(a, returnValue);
	misses++;
	return returnValue;
}

// write 16-bit data type to cache
void Cache::WRITE16(address a, __int16 value)
{
	int n = (a & setMask) >> 6;
	int offset = (a & OFFSETMASK16) * 2;
	for (int i = 0; i < nway; i++)
		slot[n][i].age++; //LRU
	
	for (int i = 0; i < nway; i++)
	{
		if (slot[n][i].valid){
			if ((slot[n][i].tag & ADDRESSMASK16) == (a & ADDRESSMASK16))
			{
				//write two bytes
				slot[n][i].value[offset] = value >> 8;
				slot[n][i].value[offset + 1] = (value & 0xFF);
				slot[n][i].dirty = true;
				totalCost += cost; hits++;
				slot[n][i].age = 0; //LRU
				slot[n][i].n_uses++; //LFU
				return;
			}
		}
	}

	// request a full line from memory/cache
	CacheLine line;
	if (nextCache)
		line = nextCache->READLINE(a & ADDRESSMASK16, ADDRESSMASK16);
	else
		line = memory->READ(a & ADDRESSMASK16);

	//if invalid, write entire line
	for (int i = 0; i < nway; i++)
	{
		if (!slot[n][i].valid){
			slot[n][i].tag = a;

			for (int t = 0; t < SLOTSIZE; t++)
				slot[n][i].value[t] = line.value[t];
			//write two bytes
			slot[n][i].value[offset] = value >> 8;
			slot[n][i].value[offset + 1] = (value & 0xFF);
			slot[n][i].valid = true;
			slot[n][i].dirty = true;
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return;
		}
	}

	int z = EVICTION(n);

	if (slot[n][z].dirty)
	{
		// write the line back to memory or next cache if dirty (no write16 needed, whole line is written)
		if (nextCache)
		{
			nextCache->WRITELINE(slot[n][z].tag & ADDRESSMASK16, slot[n][z], ADDRESSMASK16);
		}
		else
		{
			memory->WRITE(slot[n][z].tag & ADDRESSMASK16, slot[n][z]);
			totalCost += RAMACCESSCOST;
		}
	}

	//copy entire cacheline
	for (int t = 0; t < SLOTSIZE; t++)
		slot[n][z].value[t] = line.value[t];
	//write two bytes
	slot[n][z].value[offset] = value >> 8;
	slot[n][z].value[offset + 1] = (value & 0xFF);
	slot[n][z].tag = a;
	slot[n][z].valid = true;
	slot[n][z].dirty = true;
	misses++;
	slot[n][z].age = 0; //LRU
	slot[n][z].n_uses++; //LFU
	return;
}

//read 32-bit data type from cache
__int32 Cache::READ32(address a){
	int n = (a & setMask) >> 6; //bitshift offset bits (always 64 with slotsize 64)
	int offset = (a & OFFSETMASK32) * 4;
	for (int i = 0; i < nway; i++)
	{
		slot[n][i].age++; //LRU
		if ((slot[n][i].tag & ADDRESSMASK32) == (a & ADDRESSMASK32))
		{
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return (__int32)(slot[n][i].value[offset] << 24) |
							(slot[n][i].value[offset + 1] << 16) |
							(slot[n][i].value[offset + 2] << 8) |
							slot[n][i].value[offset + 3];
		}
	}

	// Read from next cache if exists, otherwise from memory
	CacheLine line;
	__int32 returnValue;
	if (nextCache)
	{
		returnValue = nextCache->READ32(a);
	}
	else //read from memory
	{
		line = memory->READ(a & ADDRESSMASK32);
		returnValue = (__int32)(line.value[offset] << 24) |
								(line.value[offset + 1] << 16) |
								(line.value[offset + 2] << 8) |
								line.value[offset + 3];
		totalCost += RAMACCESSCOST;
	}

	WRITE32(a, returnValue);
	misses++;
	return returnValue;
}

// write 32-bit data type to cache
void Cache::WRITE32(address a, __int32 value)
{
	int n = (a & setMask) >> 6;
	int offset = (a & OFFSETMASK32) * 4;

	for (int i = 0; i < nway; i++)
		slot[n][i].age++; //LRU

	for (int i = 0; i < nway; i++)
	{
		if (slot[n][i].valid){
			if ((slot[n][i].tag & ADDRESSMASK32) == (a & ADDRESSMASK32))
			{
				//write four bytes
				slot[n][i].value[offset]     =  value >> 24;
				slot[n][i].value[offset+ 1] = (value >> 16) & 0xFF;
				slot[n][i].value[offset + 2] = (value >> 8) & 0xFF;
				slot[n][i].value[offset + 3] =  value & 0xFF;
				slot[n][i].dirty = true;
				totalCost += cost; hits++;
				slot[n][i].age = 0; //LRU
				slot[n][i].n_uses++; //LFU
				return;
			}
		}
	}

	// request a full line from memory/cache
	CacheLine line;
	if (nextCache)
		line = nextCache->READLINE(a & ADDRESSMASK32, ADDRESSMASK32);
	else
		line = memory->READ(a & ADDRESSMASK32);

	//if invalid, write entire line
	for (int i = 0; i < nway; i++)
	{
		if (!slot[n][i].valid){
			slot[n][i].tag = a;

			for (int t = 0; t < SLOTSIZE; t++)
				slot[n][i].value[t] = line.value[t];
			//write four bytes
			slot[n][i].value[offset] = value >> 24;
			slot[n][i].value[offset + 1] = (value >> 16) & 0xFF;
			slot[n][i].value[offset + 2] = (value >> 8) & 0xFF;
			slot[n][i].value[offset + 3] = value & 0xFF;
			slot[n][i].valid = true;
			slot[n][i].dirty = true;
			totalCost += cost; hits++;
			slot[n][i].age = 0; //LRU
			slot[n][i].n_uses++; //LFU
			return;
		}
	}

	int z = EVICTION(n);

	if (slot[n][z].dirty)
	{
		// write the line back to memory or next cache if dirty (no write32 needed, whole line is written)
		if (nextCache)
		{
			nextCache->WRITELINE(slot[n][z].tag & ADDRESSMASK32, slot[n][z], ADDRESSMASK32);
		}
		else
		{
			memory->WRITE(slot[n][z].tag & ADDRESSMASK32, slot[n][z]);
			totalCost += RAMACCESSCOST;
		}
	}

	//copy entire cacheline
	for (int t = 0; t < SLOTSIZE; t++)
		slot[n][z].value[t] = line.value[t];
	//write four bytes
	slot[n][z].value[offset] = value >> 24;
	slot[n][z].value[offset + 1] = (value >> 16) & 0xFF;
	slot[n][z].value[offset + 2] = (value >> 8) & 0xFF;
	slot[n][z].value[offset + 3] = value & 0xFF;
	slot[n][z].tag = a;
	slot[n][z].valid = true;
	slot[n][z].dirty = true;
	misses++;
	slot[n][z].age = 0; //LRU
	slot[n][z].n_uses++; //LFU
	return;
}