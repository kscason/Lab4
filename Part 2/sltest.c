#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>

#include "SortedList.h"

#define THREADS            'A'
#define ITERATIONS         'B'
#define YIELD              'C'
#define SYNC               'D'
#define LISTS			   'E'

#define KEY_SIZE 10
#define BILLION 1000000000L

int opt_yield = 0;
int num_lists;
SortedList_t* list;
SortedList_t* elements;
char** keys;
static volatile int test_lock;
static pthread_mutex_t test_mutex;

/* struct of info for ThreadFunction */
struct threadInfo {
    long ID;
    int n_iterations;
};

/* Simple hash of the key */
int hash(const char* key)
{
	int value = 0;
    int i;
	for(i = 0; i < strlen(key); ++i)
	{
		value += (int)key[i];
	}
	return value % num_lists;
}

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
    int element_start = mydata->ID * mydata->n_iterations;

    /* Insert each of its elements into list */
    int i;
    for(i = 0; i < mydata->n_iterations; ++i) 
    {
		SortedList_insert(&list[hash(elements[element_start+i].key)], &elements[element_start+i]);
	}
	/* Grabs list length */
	int length = SortedList_length(list);
	/* Look up each added key and delete the returned element from list */
    for(i = 0; i < mydata->n_iterations; ++i)
    {
    	SortedListElement_t *toDelete = SortedList_lookup(&list[hash(elements[element_start+i].key)], keys[element_start+i]);
		SortedList_delete(toDelete);
	}
	return NULL;
}

/* Method protected by a pthread_mutex */
void* ThreadFunction_m(void *tInfo)
{
    struct threadInfo *mydata;
    mydata = (struct threadInfo*) tInfo;
    int element_start = mydata->ID * mydata->n_iterations;

    /* Insert each of its elements into list */
    int i;
    for(i = 0; i < mydata->n_iterations; ++i) 
    {
    	pthread_mutex_lock(&test_mutex);
		SortedList_insert(&list[hash(elements[element_start+i].key)], &elements[element_start+i]);
		pthread_mutex_unlock(&test_mutex);
	}

	/* Grabs list length */
	int length = SortedList_length(list);

	/* Look up each added key and delete the returned element from list */
    for(i = 0; i < mydata->n_iterations; ++i)
    {
    	pthread_mutex_lock(&test_mutex);
    	SortedListElement_t *toDelete = SortedList_lookup(&list[hash(elements[element_start+i].key)], keys[element_start+i]);
		SortedList_delete(toDelete);
		pthread_mutex_unlock(&test_mutex);
	}
	return NULL;
}

/* Method protected by a spin-lock */
void* ThreadFunction_s(void *tInfo)
{
    struct threadInfo *mydata;
    mydata = (struct threadInfo*) tInfo;
    int element_start = mydata->ID * mydata->n_iterations;

    /* Insert each of its elements into list */
    int i;
    for(i = 0; i < mydata->n_iterations; ++i) 
    {
    	while(__sync_lock_test_and_set(&test_lock, 1))
        	continue;
		SortedList_insert(&list[hash(elements[element_start+i].key)], &elements[element_start+i]);
		__sync_lock_release(&test_lock);
	}

	/* Grabs list length */
	int length = SortedList_length(list);

	/* Look up each added key and delete the returned element from list */
    for(i = 0; i < mydata->n_iterations; ++i)
    {
    	while(__sync_lock_test_and_set(&test_lock, 1))
        	continue;
    	SortedListElement_t *toDelete = SortedList_lookup(&list[hash(elements[element_start+i].key)], keys[element_start+i]);
		SortedList_delete(toDelete);
		__sync_lock_release(&test_lock);
	}
	return NULL;
}

int main(int argc, char **argv)
{
    int c;
    int num_threads = 1;
    int num_iterations = 1;
    num_lists = 1;
    char opt_sync = '\0';
    int return_value = 0;
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
          {"lists",           optional_argument, 0,          LISTS},
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
                if( strlen(optarg) < 1 || strlen(optarg) > 3 )
                {
                    fprintf( stderr, "%s: usage: %s YIELD. Using default (NONE).\n", argv[0], optarg );
                    opt_yield = 0;
                    return_value = 1;
                }
                /* Check first character */
                if(optarg[0] == 'i')
                	opt_yield |= INSERT_YIELD;
                else if(optarg[0] == 'd')
                	opt_yield |= DELETE_YIELD;
                else if(optarg[0] == 's')
                	opt_yield |= SEARCH_YIELD;

                /* Check second character (if present) */
                if(sizeof(optarg) > 1)
                {
                	if(optarg[1] == 'i')
                		opt_yield |= INSERT_YIELD;
                	else if(optarg[1] == 'd')
                		opt_yield |= DELETE_YIELD;
                	else if(optarg[1] == 's')
                		opt_yield |= SEARCH_YIELD;
 
                	/* Check third character (if present) */
                	if(sizeof(optarg) == 3)
                	{
                		if(optarg[2] == 'i')
                		opt_yield |= INSERT_YIELD;
                	else if(optarg[2] == 'd')
                		opt_yield |= DELETE_YIELD;
                	else if(optarg[2] == 's')
                		opt_yield |= SEARCH_YIELD;
                	}
                }
                break;

            case SYNC:
                /* Set sync type */
                opt_sync = *optarg;
                if( opt_sync != 'm' && opt_sync != 's' && opt_sync != '\0')
                {
                    fprintf( stderr, "%s: usage: %s SYNC. Using default ('\0').\n", argv[0], optarg );
                    opt_sync = '\0';
                    return_value = 1;
                }
                break;

            case LISTS:
            	/* Break the sorted list into sub-lists */
            	num_lists = atoi(optarg);
                if( num_lists < 1 )
                {
                    fprintf( stderr, "%s: usage: %s NLISTS. Using default (1).\n", argv[0], optarg );
                    num_lists = 1;
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
        num_threads, num_iterations, (num_threads*num_iterations*2*num_iterations/num_lists));

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

      	/* Check for synchronization */
      	int rs;
      	if(opt_sync == 'm')
      		rs = pthread_create(&threadID[t], 0, ThreadFunction_m, (void*)&thread_info_array[t]);
      	else if(opt_sync == 's')
      		rs = pthread_create(&threadID[t], 0, ThreadFunction_s, (void*)&thread_info_array[t]);
      	else // opt_sync == '\0'
      		rs = pthread_create(&threadID[t], 0, ThreadFunction, (void*)&thread_info_array[t]);
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
    if(SortedList_length(list))
        fprintf(stderr, "ERROR: final length = %d\n", SortedList_length(list));

    /* Log to STDOUT runtime (ns), average time/op (ns) */
    fprintf(stdout, "elapsed time: %lu ns\nper operation: %lu ns\n", 
        timediff, timediff/(uint64_t)(num_threads*num_iterations*2*num_iterations/num_lists));


    for(i=0; i < tot_elements; ++i)
        free(keys[i]);
    free(keys);
    free(elements);
    free(list);
    return return_value;
}
