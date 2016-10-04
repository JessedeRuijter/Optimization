#pragma once

#define L1CACHESIZE		8192					// total L1$ size, in bytes
#define L2CACHESIZE		16384					// total L2$ size, in bytes
#define L3CACHESIZE		65536					// total L3$ size, in bytes
#define SLOTSIZE		64						// cache slot size, in bytes
#define NWAY1			4						// >>>>>>>N<<<<<-way cache, L1
#define NWAY2			8						// >>>>>>>N<<<<<-way cache, L2
#define NWAY3			16						// >>>>>>>N<<<<<-way cache, L3
#define ADDRESSMASK		(0x1000000 - SLOTSIZE)	// used for masking out lowest log2(SLOTSIZE) bits
#define ADDRESSMASK16	(0x1000000 - SLOTSIZE/2) //addressmask for reading/writing 16-bit values
#define ADDRESSMASK32   (0x1000000 - SLOTSIZE/4) //addressmask for reading/writing 32-bit values
#define OFFSETMASK		(SLOTSIZE - 1)			// used for masking out bits above log2(SLOTSIZE)
#define OFFSETMASK16	(SLOTSIZE / 2 - 1)		// offsetmask for reading/writing 16-bit values
#define OFFSETMASK32	(SLOTSIZE / 4 - 1)		// offsetmask for reading/writing 32-bit values
#define SETMASK12		(0x7C0)					// used for masking out 5 set bits for L1 and L2 addresses
#define SETMASK3		(0xFC0)					// used for masking out 6 set bits for L3 addresses
#define RAMACCESSCOST	110
#define L1ACCESSCOST	8
#define L2ACCESSCOST	16						//( + L1ACCESSCOST)
#define L3ACCESSCOST	48						//( + L2ACCESSCOST + L1ACCESSCOST)

//Eviction policy (PICK ONE):
//Least Recently Used eviction policy.
	#define EV_LRU	
//random replacement eviction policy (NOTE: this affects the displayed pattern because of rand() being called!)
	//#define EV_RANDOM
//Least Frequently Used eviction policy
	//#define EV_LFU	
//Most Recently Used eviction policy
	//#define EV_MRU		
//always overwrite first slot
	//#define EV_CONST

//Real-time data visualization:
#define VISUALIZE			//turn visualization on or off
#define DATAHEIGHT	 100	//the height of the plotted data in pixels

//Number of caches used (PICK ONE):
//#define C_ONE
//#define C_TWO
#define C_THREE

//Use 8-bit, 16-bit or 32-bit data types (PICK ONE):
//#define B8 //(default)
//#define B16
#define B32

typedef unsigned int address;

struct CacheLine
{
	byte age = 0; //LRU, MRU eviction policies
	byte n_uses = 0; //LFU eviction policy
	uint tag;
	byte value[SLOTSIZE];
	bool dirty = false; //valid and dirty bits not included in tag for convenience of implementation
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
	Cache( Memory* mem, int size, int nway, int setMask, int cost, Cache* c = NULL);
	~Cache();
	// methods
	byte READ( address a );
	CacheLine READLINE(address a, int mask);
	void WRITE( address a, byte );
	void WRITELINE(address a, CacheLine& line, int mask);
	int EVICTION(int n);
	// TODO: READ/WRITE functions for (aligned) 16 and 32-bit values
	//read/write 16-bit value
    __int16 READ16(address a);
	void WRITE16(address a, __int16);
	//read/write 32-bit value
	__int32 READ32(address a);
	void WRITE32(address a, __int32);
	// data
	CacheLine **slot;
	Memory* memory;
	Cache* nextCache;
	int hits, misses, totalCost, setMask, nway, cost, cum_hits, cum_misses;
};