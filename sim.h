#ifndef SIM_CACHE_H
#define SIM_CACHE_H

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;

// Put additional data structures here as per your requirement.

class Cache{
	()
public:
	
	bool realCache;
	bool activeBuffer;
	int readMisses;
	int reads;
	int writeMisses;
	int writes;
	int writebacksToNext;
	float missRate;
	int prefetches;
	int readPrefetches;
	int readPrefetchMiss;
	int memTraffic;

	Cache(int size, int assoc, int blocksize,int N, int M, Cache* nextP);
	void printCacheSetup();
	void printVar();
	u_int32_t getTag(u_int32_t addr);
	u_int32_t getSet(u_int32_t addr);
	void LRUFix(u_int32_t set, int way);
	void request(u_int32_t addr, char r_w);
	void missRateCalc(int cacheNum);
	bool streamBufferRequest(u_int32_t addr);
	void LRUBufferFix(int LRUIndex);
	void BufferSync(u_int32_t block, int LRUIndex, int prefetchAdder);
	void BufferNew(u_int32_t block);
	void printBuffer();
private:
	
	int Sets;
	int Assoc;
	int BlockSize;
	
	int nIndexBits;
	int nBlockOffsetBits;

	int NStreamBuffers;
	int MMemoryBlocks;
	
	u_int32_t *TagArray;
	int *LRUArray;
	bool *ValidArray;
	bool *DirtyArray;

	u_int32_t *BufferArray;
	bool *BufferValidArray;
	int *BufferLRUArray;
	
	Cache * nextCache;
	
	

	
};

#endif
