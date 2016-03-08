addtest: addtest.c
	gcc -lrt -pthread -o addtest addtest.c

clean:
	rm -rf addtest
