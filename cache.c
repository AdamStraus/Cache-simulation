#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <math.h>

int associativity = 2;          // Associativity of cache
int blocksize_bytes = 32;       // Cache Block size in bytes
int cachesize_kb  =  64;          // Cache size in KB
int miss_penalty = 30;



void
print_usage ()
{
  printf ("Usage: gunzip2 -c <tracefile> | ./cache -a assoc -l blksz -s size -mp mispen\n");
  printf ("  tracefile : The memory trace file\n");
  printf ("  -a assoc : The associativity of the cache\n");
  printf ("  -l blksz : The blocksize (in bytes) of the cache\n");
  printf ("  -s size : The size (in KB) of the cache\n");
  printf ("  -mp mispen: The miss penalty (in cycles) of a miss\n");
  exit (0);
}

int main(int argc, char * argv []) {

  long address;
  int loadstore, icount;
  char marker;
  
  int i = 0;
  int j = 1;
  // Process the command line arguments
 // Process the command line arguments
  while (j < argc) {
    if (strcmp ("-a", argv [j]) == 0) {
      j++;
      if (j >= argc)
        print_usage ();
      associativity = atoi (argv [j]);
      j++;
    } else if (strcmp ("-l", argv [j]) == 0) {
      j++;
      if (j >= argc)
        print_usage ();
      blocksize_bytes = atoi (argv [j]);
      j++;
    } else if (strcmp ("-s", argv [j]) == 0) {
      j++;
      if (j >= argc)
        print_usage ();
      cachesize_kb = atoi (argv [j]);
      j++;
    } else if (strcmp ("-mp", argv [j]) == 0) {
      j++;
      if (j >= argc)
        print_usage ();
      miss_penalty = atoi (argv [j]);
      j++;
    } else {
      print_usage ();
    }
  }

  // print out cache configuration
  printf("Cache parameters:\n");
  printf ("Cache Size (KB)\t\t\t%d\n", cachesize_kb);
  printf ("Cache Associativity\t\t%d\n", associativity);
  printf ("Cache Block Size (bytes)\t%d\n", blocksize_bytes);
  printf ("Miss penalty (cyc)\t\t%d\n",miss_penalty);
  printf ("\n");

  int blocksize, numblocks, numindex, cachesize;
  cachesize = (cachesize_kb * 1024);
  blocksize = (blocksize_bytes * 32);
  numblocks = (cachesize / blocksize);
  numindex = cachesize / (blocksize * associativity);

  //these are the number of bits
  int bO, indexbits, tagbits;
  long  mask;
  int tag, index;
  int validboolean;
  int min, minass;

  //this is the array that will store the indices
  int cache[262144][associativity];

  //this is the array that will hold the valid bit
  int validbit[262144][associativity];

  //this is the array that will hold the dirty bit
  int dirtybit[262144][associativity];

  //this is the array that will hold the lru info
  int lru[262144][associativity];

  //these are used to display data in the end
  int miss = 0;
  int hit = 0;
  int memaccess = 0;
  int load = 0;
  int store = 0;
  int cycle = 0;
  int instruction = 0;
  int loadmiss = 0;
  int storemiss = 0;
  int loadhit =0;
  int storehit=0;
  int dirtyeviction = 0;
  int stall = 0;
  int memcycle = 0;
  

  //these are the ints I use to traverse arrays
  int k, l, m, n;
  

  //initialize the data in my arrays
  for (k = 0; k<numindex;k++)
    for (l = 0; l<associativity; l++)
      {
      lru[k][l]=0; //lru is 0 initially
      validbit[k][l]= 0; //valid bit is 0 initially
      dirtybit[k][l]= 0; //dirtybit is 0 initially
      cache[k][l] = -1; //the tag will initially be -1
      //printf("dirtybit =%d\n", dirtybit[75][0]);
      }
  

  

  int found;
  int count = 0;
  while (scanf("%c %d %lx %d\n",&marker,&loadstore,&address,&icount) != EOF) {
    
    //figure out the amount of bits for each part of the address
    bO = log2(blocksize_bytes);
    indexbits = log2(cachesize/(associativity*blocksize_bytes));
    tagbits = (32 - log2(cachesize/associativity));

    //to calculate the tag
    tag = address>>(bO+indexbits);
    
    //to calculate the index
    mask = pow(2,(indexbits+bO)-1) - (pow(2,bO)-1);
    index = (address&mask)>>bO;
   
    //printf("tag is %d\n", tag);
    //printf("index is %d\n", index);
    //printf("valid bit is: %d\n", validbit[index][0]);
    //printf("dirty bit is %d\n", dirtybit[index][0]);
    int breaka;
       
	if (loadstore == 0) //this is for a load
	  {
	    load++;
	    memcycle++;
	    memaccess++;
	    breaka=0;
	    cycle++;
	    for (l=0;l<associativity;l++) //we know the index, need to look through the second dimension though
	      {		    
		if (validbit[index][l]==1 && cache[index][l] == tag) //we found a hit
		  {
		    hit++;
		    loadhit++;
		    //printf("(loadhit) %d:\n", count);
		    breaka = 1;
		    break;  //break out of the loop to not go to other associativities
		  }
	      }

	    if (!breaka) //if there was not a hit
	      {
		//printf("loadmiss: \n", 1);
	    miss++;
	    loadmiss++;
	    stall++;
	    cycle = cycle+miss_penalty;
	    memcycle = memcycle + miss_penalty;
	    validboolean = 0;
	    for (l=0; l<associativity;l++)  //go through the lines of the index
	      {
		//printf("valid bit is: %d\n", validbit[index][l]);
		//printf("dirty bit is %d\n", dirtybit[index][l]);
		if (validbit[index][l]==1 && cache[index][l] != tag) //valid bit is 1 but tags don't match
		  {
		    //printf("getting here? \n", 1);
		    validboolean++;  //this is for seeing if all indices have a valid bit of 1 and tags don't match. should = associativity if so
		    min = 1000000000;
		   }
		else if (validbit[index][l]==0) //the information has never been changed
		  {
		    //printf("written to cache%D:\n", 1);
		    cache[index][l] = tag;
		    validbit[index][l] = 1;
		    break;
		  }
		//printf("this damn thing\n", 1);
	      }
	    
	    if (validboolean==associativity) //all have valid bits but no tags match, one of them will have to change
	      {
		//printf("we getting here?\n", 1);
		for (m=0;m<associativity;m++)
		  {
		    if (lru[index][m] < min)
		      {
			min = lru[index][m];
			n=m; //I need this second variable because m will be changing.  n here will only change if lru is the minimum
		      } 
		  }
	    cache[index][n] = tag;
	    //printf("written to cache%D:\n", 1);
	    validbit[index][n] = 1;
	    if (dirtybit[index][n] ==1)
	      {
		dirtybit[index][n] = 0;
		dirtyeviction+=1;
		cycle+=2;	    
	      }
	      }
	      
	      }
	  } //end of loads

    else if (loadstore ==1)  //begin stores
      {
	store++;
	breaka = 0;
	cycle++;
	memcycle++;
	memaccess++;
	for (l=0; l<associativity;l++)
	  {
	    if (cache[index][l] == tag) //we have a hit
	      {
		hit++;
	        storehit++;
		//printf("count is(storehit) %d:\n", count);
		dirtybit[index][l] = 1;
		breaka = 1;
		break;
	      }
	    
	  }
	if (!breaka)
	  {
	storemiss++;
	memcycle = memcycle + miss_penalty;
	cycle = cycle + miss_penalty;
	//printf("count is(storemiss) %d:\n", count);
	min = 1000000000;
	for (l=0;l<associativity;l++)
	  {
	    if (lru[index][l] < min)
	      {
		min = lru[index][l];
		m = min;
	      }
	  }
	cache[index][min] = tag;
	dirtybit[index][min] = 1;
	validbit[index][min] = 1;	
	  }
      }
      
  
    //i++;
  
    /* if(i<11){ */
    /* 	printf("\t%c %d %lx %d\n",marker, loadstore, address, icount); */
    /* 	i++; */
    /*    } */
    /*    else{ */
    /* 	 printf("misses and hits %d %d \n:", miss, hit); */
    /* 	return 1; */
    /*    } */
	for (k=0;k<icount;k++)
	  instruction++;
  }
 
  //printf("Lines found = %i \n",instruction);
  //printf("Simulation results:\n");
  //  Use your simulator to output the following statistics.  The 
  //  print statements are provided, just replace the question marks with
  //  your calcuations.
  int totalcpi = cycle/instruction;
  //printf("loadmiss and storemiss %d %d\n:", loadmiss, storemiss);
  printf("\texecution time %ld cycles\n", cycle);
  printf("\tinstructions %ld\n", instruction);
  printf("\tmemory accesses %ld\n", (loadhit+loadmiss+storehit+storemiss));
  printf("\toverall miss rate %.2f\n", (float)(loadmiss+storemiss)/memaccess);
  printf("\tread miss rate %.2f\n", (float)loadmiss/loadhit);
  printf("\tmemory CPI %.2f\n", (float)(totalcpi+1));
  printf("\ttotal CPI %.2f\n", (float)(totalcpi));
  printf("\taverage memory access time %.2f cycles\n", (float)(memcycle/memaccess));
  printf("dirty evictions %d\n", dirtyeviction);
  printf("load_misses %d\n", loadmiss);
  printf("store_misses %d\n", storemiss);
  printf("load_hits %d\n", loadhit);
  printf("store_hits %d\n", storehit);
  


	 }
  
