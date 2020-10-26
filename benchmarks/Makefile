.PHONY: clean

all: vector_benchmark scalar_benchmark clang_benchmark time_benchmark gcc_benchmark
	./vector -i 1 -s 10 -c sum
	./scalar -i 1 -s 10 -c sum

vector_benchmark: main.cpp roi_api.h
	g++ main.cpp -Wall -g -DVECTORIZED -ftree-vectorize -O3 -o vector

scalar_benchmark: main.cpp roi_api.h
	g++ main.cpp -Wall -g -O3 -fno-tree-vectorize -o scalar

clang_benchmark: main.cpp roi_api.h
	clang++-5.0 main.cpp -Wall -g -O2 -o normal-clang

gcc_benchmark: main.cpp roi_api.h
	g++ main.cpp -Wall -g -O2 -o normal-gcc

time_benchmark: main.cpp roi_api.h
	g++ main.cpp -DTIME_RUN -Wall -g -O2 -o time_bench

clean:
	rm normal-clang normal-gcc vector scalar time_bench
