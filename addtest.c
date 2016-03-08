#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <getopt.h>
#include <pthread.h>
#include <time.h>


#define THREADS            'a'
#define ITERATIONS         'b'

#define BILLION 1000000000L

static long long counter;

/* struct of info for ThreadFunction */
struct threadInfo{
  long ID;
  int n_iterations;
};

//Basic Add Routine
void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    *pointer = sum;
}

void* ThreadFunction( void *tInfo )
{
  struct threadInfo *mydata;
  mydata = (struct threadInfo*)tInfo;

  /* Add 1 to the counter */
  int i;
  for(i = 0; i < mydata->n_iterations; ++i)
  {
    add( &counter, 1 );
  }

  /* Add -1 to the counter */
  for (i = 0; i < mydata->n_iterations; ++i)
  {
      add( &counter, -1 );
  }

  return NULL;
}

int main (int argc, char **argv)
{
	int c;
  	int num_threads = 1;
  	int num_iterations = 1;
    int return_value = 0;
    counter = 0;
    struct timespec start, end;
    uint64_t timediff;

	while (1)
    {
    	static struct option long_options[] =
        {
          /* These options don’t set a flag. */
          {"threads",     	 optional_argument, 0,              THREADS},
          {"iterations",     optional_argument, 0,              ITERATIONS},

          {0,            0,                 0,               0}
        };

    	/* getopt_long stores the option index here. */
    	int option_index = 0;

      	/* Suppress getopt_long error messages */
      	opterr = 0;
      
      	/* Empty string "" since no short options allowed. */
      	c = getopt_long (argc, argv, "",
                       long_options, &option_index);

      	/* Detect the end of the options. */
      	if (c == -1)
        	break;

      	switch (c) {
      		case THREADS:
        		/* Set number of threads */
        		num_threads = atoi(optarg);
                if( num_threads < 1 )
                {
                    fprintf( stderr, "%s: usage: %s NTHREADS. Using default.\n", argv[0], optarg );
                    num_threads = 1;
                    return_value = 1;
                }
        		break;

        	case ITERATIONS:
        		/* Set number of iterations */
        		num_iterations = atoi(optarg);
                if( num_iterations < 1 )
                {
                    fprintf( stderr, "%s: usage: %s NITERATIONS. Using default.\n", argv[0], optarg );
                    num_iterations = 1;
                    return_value = 1;
                }
        		break;

        	default:
          	    printf ("Error: Unrecognized command!\n");
          	    return_value = 1;
      	}
    }

    /* Log to STDOUT total number of ops */
    fprintf(stdout, "%d threads x %d iterations x (add + subtract) = %d operations\n", 
        num_threads, num_iterations, (num_threads*num_iterations*2));

    /* Note the (high resolution) starting time for the run */
    clock_gettime(CLOCK_MONOTONIC, &start); /* mark start time */

    /* Create threads */
    pthread_t threadID[num_threads];
    struct threadInfo thread_info_array[num_threads];
    long t;
    for( t = 0; t < num_threads; ++t )
    {
      //currently passes in thread ID, and width to go                          
      thread_info_array[t].ID = t;
      thread_info_array[t].n_iterations = num_iterations;

      int rs = pthread_create(&threadID[t], 0, ThreadFunction, (void*)&thread_info_array[t]);
      if(rs)
      {
        fprintf(stderr, "Error creating thread. Aborting\n");
        exit(-1);
      }
    }

    /* Mama thread waits for its children */
    for( t = 0; t < num_threads; ++t )
    {
      void *retVal;
      int rs = pthread_join(threadID[t], &retVal);
      if(rs)
      {
        fprintf(stderr, "Error joining thread. Aborting\n");
        exit(-1);
      }
    }

    /* Note the (high resolution) end time for the run */
    clock_gettime(CLOCK_MONOTONIC, &end); /* mark the end time */

    timediff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

    /* Log to STDERR if counter value isn't 0 */
    if(counter)
        fprintf(stderr, "ERROR: final count = %lld\n", counter);

    /* Log to STDOUT runtime (ns), average time/op (ns) */
    fprintf(stdout, "elapsed time: %lu ns\nper operation: %lu ns\n", 
        timediff, timediff/(uint64_t)(num_threads*num_iterations*2));

    return return_value;
}
