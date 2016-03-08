#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>

#include <time.h>

#define THREADS   	   'a'
#define ITERATIONS     'b'

//Basic Add Routine
void add(long long *pointer, long long value) {
        long long sum = *pointer + value;
        *pointer = sum;
    }

int main (int argc, char **argv)
{
	int c;
  	int num_threads = 1;
  	int num_iterations = 1;
    long long counter = 0;

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
        		break;

        	case ITERATIONS:
        		/* Set number of iterations */
        		num_iterations = atoi(optarg);
        		break;
        		
        	case '?': {
          	/* In case of missing operands */
          	printf ("Error: Missing operands!\n");
          	return_value = 1;
          	break;
        	}

        	default:
          	printf ("Error: Unrecognized command!\n");
          	return_value = 1;
      	}
    }
}