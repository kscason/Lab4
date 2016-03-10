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


#define THREADS            'A'
#define ITERATIONS         'B'
#define YIELD              'C'
#define SYNC               'D'

/* New versions for add function */
#define PMUTEX             'm'
#define SPLOCK             's'
#define CMPSWAP            'c'           


#define BILLION 1000000000L

static long long counter;
static int opt_yield;
static volatile int test_lock;
static pthread_mutex_t test_mutex;

/* struct of info for ThreadFunction */
struct threadInfo {
    long ID;
    int n_iterations;
    char my_sync;
};

/* Basic Add Routine */
void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (opt_yield)
      pthread_yield();
    *pointer = sum;
}

/* Add Routine protected by a pthread_mutex */
void add_m(long long *pointer, long long value)
{
    pthread_mutex_lock(&test_mutex);
    long long sum = *pointer + value;
    if (opt_yield)
      pthread_yield();
    *pointer = sum;
    pthread_mutex_unlock(&test_mutex);
}

/* Add Routine protected by a spin-lock */
void add_s(long long *pointer, long long value)
{
    while(__sync_lock_test_and_set(&test_lock, 1))
        continue;
    long long sum = *pointer + value;
    if (opt_yield)
      pthread_yield();
    *pointer = sum;
    __sync_lock_release(&test_lock);
}

/* Add Routine protected by GCC atomic __sync_ functions */
void add_c(long long *pointer, long long value)
{
    long long sum = *pointer + value;

    if (opt_yield)
      pthread_yield();

    long long old = *pointer;
    long long retval;

    do{ 
        old = *pointer;
        sum = old + value;
        retval = __sync_val_compare_and_swap(pointer, old, sum);
    } while(retval != old);
    
}

void* ThreadFunction(void *tInfo)
{
    struct threadInfo *mydata;
    mydata = (struct threadInfo*) tInfo;
    char opt_sync = mydata->my_sync;

    /* Add 1 to the counter */
    int i;
    for(i = 0; i < mydata->n_iterations; ++i)
    {
        if(opt_sync == PMUTEX)
        {
            /* Protect with a pthread_mutex */
            add_m(&counter, 1);
        }
        else if(opt_sync == SPLOCK)
        {
            /* Protect with a spin-lock */
            add_s(&counter, 1);
        }
        else if(opt_sync == CMPSWAP)
        {
            add_c(&counter, 1);
        }
        else
            add(&counter, 1);
    }

    /* Add -1 to the counter */
    for (i = 0; i < mydata->n_iterations; ++i)
    {
        if(opt_sync == PMUTEX)
        {
            /* Protect with a pthread_mutex */
            add_m(&counter, -1);
        }
        else if(opt_sync == SPLOCK)
        {
            /* Protect with a spin-lock */
            add_s(&counter, -1);
        }
        else if(opt_sync == CMPSWAP)
        {
            add_c(&counter, -1);
        }
        else
        {
            add(&counter, -1);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int c;
    int num_threads = 1;
    int num_iterations = 1;
    char opt_sync = '\0';
    int return_value = 0;
    counter = 0;
    opt_yield = 0;
    test_lock = 0;
    pthread_mutex_init(&test_mutex, NULL);
    struct timespec start, end;
    uint64_t timediff;

    while (1)
    {
        static struct option long_options[] =
        {
          /* These options don’t set a flag. */
          {"threads",         optional_argument, 0,          THREADS},
          {"iterations",      optional_argument, 0,          ITERATIONS},
          {"yield",           optional_argument, 0,          YIELD},
          {"sync",            optional_argument, 0,          SYNC},
          {0, 0, 0, 0}
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
                    fprintf( stderr, "%s: usage: %s NTHREADS. Using default (1).\n", argv[0], optarg );
                    num_threads = 1;
                    return_value = 1;
                }
                break;

            case ITERATIONS:
                /* Set number of iterations */
                num_iterations = atoi(optarg);
                if( num_iterations < 1 )
                {
                    fprintf( stderr, "%s: usage: %s NITERATIONS. Using default (1).\n", argv[0], optarg );
                    num_iterations = 1;
                    return_value = 1;
                }
                break;

            case YIELD:
              /* Set opt_yield */
                opt_yield = atoi(optarg);
                if( opt_yield != 1 )
                {
                    fprintf( stderr, "%s: usage: %s YIELD. Using default (0).\n", argv[0], optarg );
                    opt_yield = 1;
                    return_value = 1;
                }
                break;

            case SYNC:
                /* Set sync type */
                opt_sync = *optarg;
                if( opt_sync != PMUTEX && opt_sync != SPLOCK && opt_sync != CMPSWAP && opt_sync != '\0')
                {
                    fprintf( stderr, "%s: usage: %s SYNC. Using default ('\0').\n", argv[0], optarg );
                    opt_sync = '\0';
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
      thread_info_array[t].my_sync = opt_sync;

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
