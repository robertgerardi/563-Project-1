#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <iostream>
#include <cmath>
#include "sim.h"

using namespace std;

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char *argv[]) {
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
   }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];

   // Open the trace file for reading.
   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }
    
   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE);
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
   printf("L2_SIZE:    %u\n", params.L2_SIZE);
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
   printf("PREF_N:     %u\n", params.PREF_N);
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   //printf("===================================\n");
   Cache *L2P = NULL;
   Cache L2(params.L2_SIZE,params.L2_ASSOC,params.BLOCKSIZE,params.PREF_N,params.PREF_M,NULL);
   if(L2.realCache){
      L2P = &L2;
   }
   cout << endl;
   Cache L1(params.L1_SIZE,params.L1_ASSOC,params.BLOCKSIZE,params.PREF_N,params.PREF_M,L2P);
   
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
     /*
      if (rw == 'r')
         printf("r %x\n", addr);
      else if (rw == 'w')
         printf("w %x\n", addr);
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }
      */
      
      
      
      L1.request(addr,rw);
      
	  
    }
    L1.missRateCalc(1);
    if(L2.realCache){
    L2.missRateCalc(2);
    }
     printf("===== L1 contents =====\n");
    L1.printVar();
   
    
    if(L2.realCache)
    {
      printf("===== L2 contents =====\n");
      L2.printVar();
    }
    if(L1.activeBuffer){
      L1.printBuffer();
    }
    if(L2.activeBuffer){
      L2.printBuffer();
    }

    printf("===== Measurements =====\n");
    printf("a. L1 reads:                   %d\n", L1.reads);
    printf("b. L1 read misses:             %d\n", L1.readMisses);
    printf("c. L1 writes:                  %d\n", L1.writes);
    printf("d. L1 write misses:            %d\n", L1.writeMisses);
    printf("e. L1 miss rate:               %.4f\n", L1.missRate);
    printf("f. L1 writebacks:              %d\n", L1.writebacksToNext);
    printf("g. L1 prefetches:              %d\n", L1.prefetches);
    printf("h. L2 reads (demand):          %d\n", L2.reads);
    printf("i. L2 read misses (demand):    %d\n", L2.readMisses);
    printf("j. L2 reads (prefetch):        %d\n", L2.readPrefetches);
    printf("k. L2 read misses (prefetch):  %d\n", L2.readPrefetchMiss);
    printf("l. L2 writes:                  %d\n", L2.writes);
    printf("m. L2 write misses:            %d\n", L2.writeMisses);
    printf("n. L2 miss rate:               %.4f\n", L2.missRate);
    printf("o. L2 writebacks:              %d\n", L2.writebacksToNext);
    printf("p. L2 prefetches:              %d\n", L2.prefetches);
    printf("q. memory traffic:             %d\n", L1.memTraffic + L2.memTraffic);
     
   
   
   
    return(0);
}
