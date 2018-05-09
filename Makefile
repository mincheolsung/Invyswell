all:
	g++ test_threads.cpp tm/WriteSet.c -std=c++11 -pthread -g -O0 -o test_threads

clean:
	rm -f test_threads
