sltest: sltest.c SortedList.c
	gcc -lrt -pthread -o sltest sltest.c SortedList.c 

SortedList: SortedList.c SortedList.h
	gcc -lrt -pthread SortedList.c -c

clean:
	rm -rf SortedList.o sltest
