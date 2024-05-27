//cache file

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <iostream>
#include <cmath>
#include "sim.h"

using namespace std;

Cache::Cache(int size, int assoc, int blocksize, int N, int M, Cache* nextP){

	if(size > 0){
		realCache = true;
		Sets = size/(assoc * blocksize);
		Assoc = assoc;
		BlockSize = blocksize;

		nIndexBits = log2(Sets);
		nBlockOffsetBits = log2(BlockSize);

		int arrayLength = Sets * Assoc;

		DirtyArray = new bool[arrayLength];
		ValidArray = new bool[arrayLength];
		LRUArray = new int[arrayLength];
		TagArray = new u_int32_t[arrayLength];

		NStreamBuffers = N;
		MMemoryBlocks = M;

		int bufferLength = NStreamBuffers * MMemoryBlocks;

		BufferArray = new u_int32_t[bufferLength];
		BufferLRUArray = new int[NStreamBuffers];
		BufferValidArray = new bool[NStreamBuffers];

		nextCache = nextP;

		//intialize arrays
		for(int i = 0; i < arrayLength; i++){
			ValidArray[i] = false;
			DirtyArray[i] = false;
			LRUArray[i] = (i%Assoc);
			TagArray[i] = 0;
		}
		reads = 0;
		readMisses = 0;
		writes = 0;
		writeMisses = 0;
		writebacksToNext = 0;
		missRate = 0.0; 
		prefetches = 0;
		readPrefetches = 0;
		readPrefetchMiss = 0;
		memTraffic = 0;
		activeBuffer = false;
		
		
		
		if((N > 0) && nextP == NULL){
			activeBuffer = true;
			
			for(int i = 0; i < bufferLength; i++){
				BufferArray[i] = 0;
			}
			for(int i = 0; i < NStreamBuffers;i++){
				BufferLRUArray[i] = i;
				
				BufferValidArray[i] = false;
			}
		}
		
	}
	else{
		realCache = false;
		activeBuffer = false;
		reads = 0;
		readMisses = 0;
		writes = 0;
		writeMisses = 0;
		writebacksToNext = 0;
		missRate = 0.0; 
		prefetches = 0;
		readPrefetches = 0;
		readPrefetchMiss = 0;
		memTraffic = 0.000;
		NStreamBuffers = 0;
		MMemoryBlocks = 0;
	}
}
void Cache::printCacheSetup(){
	cout << "------------------------------------------" << endl;
	cout << "Sets:" << Sets << endl;
	cout << "Assoc:" << Assoc << endl;
	cout << "BlockSize:" << BlockSize << endl;
	cout << "nIndexBits:" << nIndexBits << endl;
	cout << "nBlockOffsetBits:" << nBlockOffsetBits << endl;
	cout << "------------------------------------------" << endl;
	cout << endl;	

}

void Cache::missRateCalc(int num){
	if(num == 1){
	missRate = (float(readMisses) + float(writeMisses)) / (float(reads) + float(writes));
	}
	else{
		missRate = float(readMisses) / float(reads);
	}
	
}
void Cache::printVar(){
	u_int32_t tempTags[Assoc];
	bool tempDirty[Assoc];

	
        for (int i = 0; i < Sets; ++i) {
            printf("set\t%d:", i);
            for (int j = 0; j < Assoc; ++j) {
				/*
					
                if (ValidArray[i+j]) {
                    printf("   %x", TagArray[i+j]);
                    if (DirtyArray[i+j]) {
                        printf(" D ");
                    }
                }
				*/
				tempTags[LRUArray[Assoc*i+j]] = TagArray[Assoc*i+j];
				tempDirty[LRUArray[Assoc*i+j]] = DirtyArray[Assoc*i+j];


            }
			for(int k = 0; k < Assoc; k++)
			{
			std::cout << "  " << std::hex << tempTags[k] << std::dec;
			if (tempDirty[k])
			{
				std::cout << " D";
			}else
			{
				std::cout << "  ";
			}
			}
		
            cout << endl;
        }
    
	cout<< endl;

}
void Cache::printBuffer(){
	cout << "===== Stream Buffer(s) contents ====="<<endl;
	for(int i = 0; i < (NStreamBuffers); i++){
		for(int j = 0; j < NStreamBuffers;j++){
			if(BufferLRUArray[j] == i){
				
				for(int k = 0; k < MMemoryBlocks;k++){
					cout << hex << BufferArray[(j*MMemoryBlocks)+k] << " ";
					
				}
			}
		}
		cout << endl;
	}
	
	
	cout << endl;
}

u_int32_t Cache::getSet(u_int32_t addr){
	return ((addr >> nBlockOffsetBits) & (Sets - 1));
}

u_int32_t Cache::getTag(u_int32_t addr){
	return ((addr)>>(nBlockOffsetBits + nIndexBits));
}

//-----------------------------------------------------------
void Cache::LRUFix(u_int32_t set, int way){
	
	//cout << LRUArray[way] << endl;
	for(u_int32_t i = set*Assoc; i < (set+1)*Assoc; i++){

		if(LRUArray[i] < LRUArray[way]){
			//increment ways that are less than selected
			
			LRUArray[i]++;
		}
	}
	//update current LRU to be most recent
	
	LRUArray[way] = 0;
}

void Cache::LRUBufferFix(int LRUIndex){
	
	for(int i = 0; i< NStreamBuffers; i++){
		//cout << BufferLRUArray[i] << endl;
			//cout << LRUIndex << endl;
		if(BufferLRUArray[i] < BufferLRUArray[LRUIndex]){
			
			BufferLRUArray[i]++;
			
		}
	}
	BufferLRUArray[LRUIndex] = 0;
}


//---------------------------------------------------------

/*Idea for buffer request
If hits in cache, check buffer,
no hit in buffer, do not do anything
hit in buffer, save current spot, clear buffer, refill*/

void Cache::BufferNew(u_int32_t addr){
	int index = 0;

	u_int32_t block = (addr >> nBlockOffsetBits);
	block++;
	for(int i = 0; i < NStreamBuffers; i++){
		if(BufferLRUArray[i] == (NStreamBuffers - 1)){
			index = i;
		}
	}
	
	LRUBufferFix(index);
	BufferValidArray[index] = true;
	index = index * MMemoryBlocks;
	
	for(int i = 0; i < MMemoryBlocks; i++){
		
		BufferArray[index + i] = block;
		block++;

	}
	
	prefetches += MMemoryBlocks;
	memTraffic += MMemoryBlocks;
	
	
}

void Cache::BufferSync(u_int32_t block, int LRUIndex,int prefetchAdder){
	prefetches += prefetchAdder;
	memTraffic += prefetchAdder;
	u_int32_t test = block;
	int bufferStart = LRUIndex * MMemoryBlocks;
	int bufferEnd = bufferStart + MMemoryBlocks;
	
	for(int i = bufferStart; i < bufferEnd;i++){
		
		BufferArray[i] = test;
		test++;
	}
}
bool Cache::streamBufferRequest(u_int32_t addr){
	
	u_int32_t block = (addr >> nBlockOffsetBits);
	//int arrayLength = NStreamBuffers * MMemoryBlocks;

	int LRUOrder[NStreamBuffers];
	int temp = 0;
	int LRUNum = 0;
	
	//get LRU indices in order
	while(temp < NStreamBuffers){
		
		for(int i = 0; i < NStreamBuffers; i++){
			
			if(BufferLRUArray[i] == LRUNum){
				
				LRUOrder[temp] = i;
				temp++;
				LRUNum++;
				break;
			}
		}
	}
	
		int LRUSave = 0;
		u_int32_t addrSave = 0;
		for(int i = 0; i < NStreamBuffers; i++){
			int BlockIndex = LRUOrder[i] * MMemoryBlocks;
			for(int j = 0; j < MMemoryBlocks;j++){
					if((BufferArray[BlockIndex + j]== block) && BufferValidArray[LRUOrder[i]]){
						addrSave = block + 1;
						LRUSave = LRUOrder[i];
						BufferSync(addrSave,LRUSave,j+1);
						LRUBufferFix(LRUSave);
						return true;
					}
				}
			}

	return false;
		
	
	
}
void Cache::request(u_int32_t addr, char r_w){
	u_int32_t setNum = Cache::getSet(addr);
	u_int32_t tag = Cache::getTag(addr);

	switch(r_w){
		case 'r':{
			reads++;
			int LRUIndex = -1;
			bool bufferHit = false;
			if(activeBuffer){
				bufferHit = streamBufferRequest(addr);				
			}
			for(int i = 0; i < Assoc; i++){
				
				
				int arrayIndex = setNum*Assoc + i;
				
				if(ValidArray[arrayIndex] && TagArray[arrayIndex] == tag){
					//read hit
					LRUFix(setNum,arrayIndex);
					
					return;
				}
				if(LRUArray[arrayIndex] == (Assoc-1)){
					LRUIndex = arrayIndex;
					
				}
				
				
				
			}
			
			//if reaches this point, read miss
			if(activeBuffer){
				if(!bufferHit){
					
					readMisses++;
					BufferNew(addr);
				}
			}
			else{
				readMisses++;
			}
				
			
			
			//find LRU
			
			if(!DirtyArray[LRUIndex]){
				//block is not dirty, no wb, just read to next level
					if(!activeBuffer){
					if(nextCache != NULL){
						nextCache->request(addr,'r');
					}
					else{
						memTraffic++;
					}
					}else{
						if(!bufferHit){
							memTraffic++;
						}
					}
				LRUFix(setNum,LRUIndex);
				ValidArray[LRUIndex] = true;
				TagArray[LRUIndex] = tag;
				return;
			}
			else{
				//block is dirty, writeback to next level
				writebacksToNext++;
				if(!activeBuffer){
				if(nextCache != NULL){
					u_int32_t newaddr = ((TagArray[LRUIndex]) << (nIndexBits + nBlockOffsetBits)) + (setNum << nBlockOffsetBits);
					nextCache->request(newaddr,'w');
					nextCache->request(addr,'r');
				}else{
					memTraffic++;
					memTraffic++;
				}
				}else{
					memTraffic++;
					if(!bufferHit){
						memTraffic++;
					}
				}
				//clearing dirty, already valid
				LRUFix(setNum,LRUIndex);
				ValidArray[LRUIndex] = true;
				TagArray[LRUIndex] = tag;
				DirtyArray[LRUIndex] = false;
				return;
			}
			break;
		}
		case 'w':{
			writes++;
			int LRUIndex = -1;
			bool bufferHit = false;
			if(activeBuffer){
				bufferHit = streamBufferRequest(addr);				
			}
			for(int i = 0; i < Assoc; i++){
	
				int arrayIndex = setNum*Assoc + i;
				
				if(ValidArray[arrayIndex] && TagArray[arrayIndex] == tag){
					//write hit, need to set it to dirty
					LRUFix(setNum,arrayIndex);
					
					DirtyArray[arrayIndex] = true;
					return;
				}
				if(LRUArray[arrayIndex] == (Assoc-1)){
					LRUIndex = arrayIndex;
				}

			}
			
			//if it reaches this point, it is a write miss
			if(activeBuffer){
				if(!bufferHit){
					writeMisses++;
					BufferNew(addr);
				}
			}
			else{
				writeMisses++;
			}
			
			if(!DirtyArray[LRUIndex]){
				//block is not dirty, no wb, just read to next level, set dirty
				if(!activeBuffer){
				if(nextCache != NULL){
					nextCache->request(addr,'r');
				}else{
					memTraffic++;
				}
				}else{
					if(!bufferHit){
							memTraffic++;
						}
				}
				LRUFix(setNum,LRUIndex);
				ValidArray[LRUIndex] = true;
				TagArray[LRUIndex] = tag;
				DirtyArray[LRUIndex] = true;
				return;
			}else{
				//block is dirty, writeback to next level
				writebacksToNext++;
				if(!activeBuffer){

				
				if(nextCache != NULL){
					u_int32_t newaddr = ((TagArray[LRUIndex]) << (nIndexBits + nBlockOffsetBits)) + (setNum << nBlockOffsetBits);
					nextCache->request(newaddr,'w');
					nextCache->request(addr,'r');
				}
				else{
					memTraffic++;
					memTraffic++;
				}
				}else{
					memTraffic++;
					if(!bufferHit){
							memTraffic++;
						}
				}
				//clearing dirty, already valid
				LRUFix(setNum,LRUIndex);
				ValidArray[LRUIndex] = true;
				TagArray[LRUIndex] = tag;
				DirtyArray[LRUIndex] = true;
				return;
			}


			break;
		}
	}
	
		


	
}
