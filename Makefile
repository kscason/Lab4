addtest: addtest.o
	gcc -lrt -o addtest addtest.o

addtest.o: addtest.c
	gcc -c -std=gnu11 -Wall addtest.c