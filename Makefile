all:
	g++ test_threads.cpp -pthread -o test_threads
clean:
	rm test_threads
