#pragma once

#define L1CACHESIZE		8192					// total L1$ size, in bytes
#define L2CACHESIZE		16384					// total L2$ size, in bytes
#define L3CACHESIZE		65536					// total L3$ size, in bytes
#define SLOTSIZE		64						// cache slot size, in bytes
#define NWAY1			4						// >>>>>>>N<<<<<-way cache, L1
#define NWAY2			8						// >>>>>>>N<<<<<-way cache, L2
#define NWAY3			16						// >>>>>>>N<<<<<-way cache, L3
#define ADDRESSMASK		(0x1000000 - SLOTSIZE)	// used for masking out lowest log2(SLOTSIZE) bits
#define OFFSETMASK		(SLOTSIZE - 1)			// used for masking out bits above log2(SLOTSIZE)
#define SETMASK12		(0x7C0)					// used for masking out 5 set bits for L1 and L2 addresses
#define SETMASK3		(0xFC0)					// used for masking out 6 set bits for L3 addresses
#define RAMACCESSCOST	110
#define L1ACCESSCOST	8
#define L2ACCESSCOST	16
#define L3ACCESSCOST	48

//Least Recently Used eviction policy, 12m cycles
	#define EV_LRU	
//random replacement eviction policy, 14M cycles
	//#define EV_RANDOM
//Least Frequently Used eviction policy, 22M cycles
	//#define EV_LFU	
//Most Recently Used eviction policy, 33M cycles
	//#define EV_MRU		
//always overwrite first slot, 43M cycles
	//#define EV_CONST

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
	Cache( Memory* mem, int size, int nway, int setMask, Cache* c = NULL);
	~Cache();
	// methods
	byte READ( address a );
	CacheLine READLINE(address a);
	void WRITE( address a, byte );
	void WRITELINE(address a, CacheLine& line);
	int EVICTION(int n);
	// TODO: READ/WRITE functions for (aligned) 16 and 32-bit values
	// data
	CacheLine **slot;
	Memory* memory;
	Cache* nextCache;
	int hits, misses, totalCost, setMask, nway;
};