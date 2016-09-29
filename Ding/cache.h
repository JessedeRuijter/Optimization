#pragma once

#define L1CACHESIZE		8192					// total L1$ size, in bytes
#define L2CACHESIZE		16384					// total L2$ size, in bytes
#define L3CACHESIZE		65536					// total L3$ size, in bytes
#define SLOTSIZE		64						// cache slot size, in bytes
#define NWAY			4						// >>>>>>>N<<<<<-way cache
#define ADDRESSMASK		(0x1000000 - SLOTSIZE)	// used for masking out lowest log2(SLOTSIZE) bits
#define OFFSETMASK		(SLOTSIZE - 1)			// used for masking out bits above log2(SLOTSIZE)
#define SETMASK			(0x7C0)					// used for masking out set bits (to do: make variable)
#define RAMACCESSCOST	110
#define L1ACCESSCOST	8
#define L2ACCESSCOST	16
#define L3ACCESSCOST	48

//random replacement eviction policy, 14M cycles
#//define EV_RANDOM
//Least Recently Used eviction policy, 12m cycles
#define EV_LRU	
//Most Recently Used eviction policy, 33M cycles
//#define EV_MRU		
//Least Frequently Used eviction policy, 22M cycles
//#define EV_LFU								

typedef unsigned int address;

struct CacheLine
{
	byte age = 0;
	byte n_uses = 0; //LFU eviction policy
	uint tag;
	byte value[SLOTSIZE];
	bool dirty = false;
	bool valid = false;
};

class Memory
{
public:
	// ctor/dtor
	Memory( uint size );
	~Memory();
	// methods
	CacheLine READ( address a );
	void WRITE( address a, CacheLine& line );
	// data members
	CacheLine* data;
	bool artificialDelay;
};

class Cache
{
public:
	// ctor/dtor
	Cache( Memory* mem );
	~Cache();
	// methods
	byte READ( address a );
	void WRITE( address a, byte );
	int EVICTION(int n);
	// TODO: READ/WRITE functions for (aligned) 16 and 32-bit values
	// data
	CacheLine **slot;
	Memory* memory;
	int hits, misses, totalCost;
};