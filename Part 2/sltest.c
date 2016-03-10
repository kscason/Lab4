#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h> // MIGHT NOT NEED 
#include <getopt.h>
#include <pthread.h>
#include <time.h>

#include "SortedList.h"

#define THREADS            'A'
#define ITERATIONS         'B'
#define YIELD              'C'
#define SYNC               'D'

/* Lock primitives */
#define PMUTEX             'm'
#define SPLOCK             's'

#define KEY_SIZE 10
#define BILLION 1000000000L

int opt_yield = 0;
SortedList_t* list;
SortedList_t* elements;
char** keys;

/* struct of info for ThreadFunction */
struct threadInfo {
    long ID;
    int n_iterations;
    char my_sync;
};

/* Generate random keys */
char* create_key(int index)
{
	keys[index] = malloc(sizeof(char)*KEY_SIZE);
    if( keys == NULL )
    {
        fprintf(stderr, "Error allocating memory. Aborting\n");
        exit(-1);
    }
    int j;
	for(j = 0; j < KEY_SIZE; ++j)
	{
		if(j == KEY_SIZE - 1)
		{
			keys[index][j] = '\0';
			continue;
		}
		keys[index][j] = '!' + (rand() % 93); //from '!' to '}'
	}
	return keys[index];
}

void* ThreadFunction(void *tInfo)
{
    struct threadInfo *mydata;
    mydata = (struct threadInfo*) tInfo;
    //char opt_sync = mydata->my_sync;
    int element_start = mydata->ID * mydata->n_iterations;

    /* Insert each of its elements into list */
    int i;
    for(i = 0; i < mydata->n_iterations; ++i) 
    {
		SortedList_insert(list, &elements[element_start+i]);
	}

	/* Grabs list length */
	int length = SortedList_length(list);

	/* Look up each added key and delete the returned element from list */
    for(i = 0; i < mydata->n_iterations; ++i)
    {
    	SortedListElement_t *toDelete = SortedList_lookup(list, keys[element_start+i]);
		SortedList_delete(toDelete);
	}


}

int main(int argc, char **argv)
{
    int c;
    int length;
    int num_threads = 1;
    int num_iterations = 1;
    int num_lists = 1;
    char opt_sync = '\0';
    int return_value = 0;
    //test_lock = 0;
    //pthread_mutex_init(&test_mutex, NULL);
    struct timespec start, end;
    uint64_t timediff;

    while (1)
    {
        static struct option long_options[] =
        {
          /* These options don’t set a flag. */
          {"threads",         optional_argument, 0,          THREADS},
          {"iterations",      optional_argument, 0,          ITERATIONS},
          //{"yield",           optional_argument, 0,          YIELD},
          //{"sync",            optional_argument, 0,          SYNC},
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
                //opt_yield = atoi(optarg);
                if( strlen(optarg) < 1 || strlen(optarg) > 3 )
                {
                    fprintf( stderr, "%s: usage: %s YIELD. Using default (NONE).\n", argv[0], optarg );
                    opt_yield = 0;
                    return_value = 1;
                }
                /* Check first character */
                if(optarg[0] == 'i')
                	opt_yield &= INSERT_YIELD;
                else if(optarg[0] == 'd')
                	opt_yield &= DELETE_YIELD;
                else if(optarg[0] == 's')
                	opt_yield &= SEARCH_YIELD;

                /* Check second character (if present) */
                if(sizeof(optarg) > 1)
                {
                	if(optarg[1] == 'i')
                		opt_yield &= INSERT_YIELD;
                	else if(optarg[1] == 'd')
                		opt_yield &= DELETE_YIELD;
                	else if(optarg[1] == 's')
                		opt_yield &= SEARCH_YIELD;
 
                	/* Check third character (if present) */
                	if(sizeof(optarg) == 3)
                	{
                		if(optarg[2] == 'i')
                		opt_yield &= INSERT_YIELD;
                	else if(optarg[2] == 'd')
                		opt_yield &= DELETE_YIELD;
                	else if(optarg[2] == 's')
                		opt_yield &= SEARCH_YIELD;
                	}
                }
                break;

            case SYNC:
                /* Set sync type */
                opt_sync = *optarg;
                if( opt_sync != PMUTEX && opt_sync != SPLOCK && opt_sync != '\0')
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
    fprintf(stdout, "%d threads x %d iterations x (ins + lookup/del) x (100/2 avg len) = %d operations\n", 
        num_threads, num_iterations, (num_threads*num_iterations*2*10));

    /* Initialize the empty list */
    list = malloc(sizeof(SortedList_t)*num_lists);
    if( list == NULL )
  	{
        fprintf(stderr, "Error allocating memory. Aborting\n");
        exit(-1);
    }

    /* Set head for all the lists */
    int i;
    for( i = 0; i < num_lists; ++i )
    {
    	/* Heads have null key */
    	list[i].prev = NULL;
    	list[i].next = NULL;
    	list[i].key = NULL;
    }

    /* Create required number (threads × iterations) of list elements */
    int tot_elements = num_threads * num_iterations;
   	elements = malloc(sizeof(SortedListElement_t)*tot_elements);
   	if( elements == NULL )
  	{
        fprintf(stderr, "Error allocating memory. Aborting\n");
        exit(-1);
    }

    /* Create total number of keys */
    keys = malloc(sizeof(char*)*tot_elements);
	if( keys == NULL )
  	{
        fprintf(stderr, "Error allocating memory. Aborting\n");
        exit(-1);
    }

    /* Initialize list elements */
    for( i = 0; i < tot_elements; ++i )
    {
    	elements[i].prev = NULL;
    	elements[i].next = NULL;
    	elements[i].key = create_key(i);
    }

    /* Note the (high resolution) starting time for the run */
    clock_gettime(CLOCK_MONOTONIC, &start); /* mark start time */

    /* Create threads */
    pthread_t threadID[num_threads];
    struct threadInfo thread_info_array[num_threads];
    long t;
    for( t = 0; t < num_threads; ++t )
    {                        
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

    /* Log to STDERR if length of list isn't 0 */
    length = SortedList_length(list);
    if(length != 0)
        fprintf(stderr, "ERROR: final length = %d\n", length);

    /* Log to STDOUT runtime (ns), average time/op (ns) */
    fprintf(stdout, "elapsed time: %lu ns\nper operation: %lu ns\n", 
        timediff, timediff/(uint64_t)(num_threads*num_iterations*2));

    return return_value;
}