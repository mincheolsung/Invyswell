all:
	g++ test_threads.cpp tm/WriteSet.c -std=c++11 -pthread -o test_threads

clean:
	rm -f test_threads
