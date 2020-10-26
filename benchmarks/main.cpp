#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <unistd.h> //getopt
#include <getopt.h>
#include <string>
#include "roi_api.h"


#define REP2(S)        S ;        S
#define REP4(S)   REP2(S);   REP2(S)
#define REP8(S)   REP4(S);   REP4(S)
#define REP16(S)  REP8(S);   REP8(S)
#define REP32(S)  REP16(S);  REP16(S)
#define REP64(S)  REP32(S);  REP32(S)
#define REP128(S) REP64(S);  REP64(S)
#define REP256(S) REP128(S); REP128(S)
#define REP512(S) REP256(S); REP256(S)



//For computing memory bandwidth, the actual size of the arrays should be 4 times the cache one.

int N;
int IT;

typedef double T;


	

void delim_start(void) __attribute__((noinline));
void delim_stop(void) __attribute__((noinline));

void delim_start(void){
	std::cout << "~~ Start - " << std::endl;
}


void delim_stop(void){
	std::cout << "~~ Stop - " << std::endl;
}



#ifdef VECTORIZED
// STREAM TRIAD - a[j] = b[j] + <scalar> * c[j]
__attribute__((noinline)) void vector_triad(const T* __restrict  a, const T* __restrict b, T* __restrict c, T scale){
	//__asm__("vzeroupper");
#else
__attribute__((noinline)) void vector_triad(const T* a, const T* b, T* c, T scale){
#endif

	for(int i=0; i<N; i++){
		c[i] = a[i] + scale * b[i];
	}
	return;
}

#ifdef VECTORIZED
// STREAM SCALE - b[j] = <scalar> * c[j]
__attribute__((noinline)) void vector_scale(const T* __restrict a, T* __restrict b, T scale){
#else
__attribute__((noinline)) void vector_scale(const T* a, T* b, T scale){
#endif
	for(int i=0; i<N; i++){
		b[i] = scale * a[i];
	}
	return;
}

#ifdef VECTORIZED
// STREAM ADD - c[j] = a[j] + b[j]
__attribute__((noinline)) void vector_sum(const T* __restrict a,const T* __restrict b , T * __restrict c){
#else
__attribute__((noinline)) void vector_sum(const T* a,const T* b , T* c){
#endif

	for(int i=0; i<N; i++){
		c[i] = b[i] + a[i];
	}
	return;
}


// Kernel 1 features 1 floating point operation
#define KERNEL1(a,b,c)   ((a) = (b) + (c))
// Kernel 2 features 2 floating point operation
#define KERNEL2(a,b,c)   ((a) = (a)*(b) + (c))


__attribute__((noinline)) void ert_kernel(T* __restrict A){
    
  T alpha = 2.0;
  T beta = 1.0;
  for(int i=0; i<N; i++){
      // 64 FLOPS
      REP32(KERNEL2(beta,A[i],alpha));
      A[i] = -beta;
  }
  return;

}

#ifdef VECTORIZED
// STREAM COPY - c[j] = a[j]
__attribute__((noinline)) void vector_copy(T* __restrict a, const T* __restrict b){
#else
__attribute__((noinline)) void vector_copy(T* a, const T* b){
#endif

	for(int i=0; i<N; i++){
		a[i] = b[i];
	}

	return;
}


#ifdef VECTORIZED
void function_sum(const T* __restrict a, const T* __restrict b, T* __restrict c) __attribute__((noinline));
void function_sum(const T* __restrict a, const T* __restrict b, T* __restrict c){
#else
void function_sum(const T* a, const T* b, T* c) __attribute__((noinline));
void function_sum(const T* a, const T* b, T* c){
#endif
	for(int j=0; j<IT; j++){
			vector_sum(a,b,c);
	}
	return;
}


void expected_fp(std::string label,int iterations, int vector_size, int operations){
	assert(iterations > 0 );
	assert(vector_size > 0 );
	assert(operations >= 0 );
	unsigned long long fp_ops = (unsigned long long)iterations * (unsigned long long ) vector_size * (unsigned long long) operations;
	std::cout << std::scientific;
	std::cout << "Label -- " <<label <<" -- Expected Floating point operations are: " << (double) fp_ops << std::scientific << std::endl;
	return;
}


void expected_bytes(int iterations, int vector_size, int bytes_per_elem, int mem_accesses_per_operation){
	assert(iterations > 0 );
	assert(vector_size> 0 );
	assert(bytes_per_elem > 0 );
	assert(mem_accesses_per_operation > 0 );

        unsigned long long fp_ops = (unsigned long long) iterations * (unsigned long long) vector_size * (unsigned long long)bytes_per_elem *(unsigned long long) mem_accesses_per_operation;

	std::cout << std::scientific;
	std::cout << "Expected bytes accessed are: " << (double) fp_ops << " bytes." << std::endl;
	return;
}



void execute_command(std::string command){

#ifdef VECTORIZED
	std::cout << "I'm running the benchmark vectorized version" << std::endl;
#endif

	T* a = new T[N];
	T* b = new T[N];
	T* c = new T[N];

	// If the command is not defined, tell the user.
	std::cout << "Running " << command << std::endl;
	if (command.compare("sum") == 0){

		expected_fp("sum",IT, N, 1);
		expected_bytes(IT,N,sizeof(T), 3);


		Roi_Start("sum");
		for(int j=0; j<IT; j++){
			vector_sum(a,b,c);
		}
		Roi_End("sum");

	}
    else if (command.compare("ert") == 0){

		expected_fp(command,IT, N, 65);
		expected_bytes(IT,N,sizeof(T), 2);

		Roi_Start(command.c_str());
		for(int j=0; j<IT; j++){
            ert_kernel(a);
		}
		Roi_End(command.c_str());
	}

	else if (command.compare("triad") == 0){

		expected_fp(command,IT, N, 2);
		expected_bytes(IT,N,sizeof(T), 3);

		Roi_Start(command.c_str());
		for(int j=0; j<IT; j++){
			vector_triad(a,b,c,2.0);
		}
		Roi_End(command.c_str());
	}
	else if (command.compare("scale") == 0){

		expected_fp(command,IT, N, 1);
		expected_bytes(IT,N,sizeof(T), 2);

		Roi_Start(command.c_str());
		for(int j=0; j<IT; j++){
			vector_scale(a,b,2.5);
		}
		Roi_End(command.c_str());

	}
	else if (command.compare("copy") == 0){
		expected_fp(command,IT, N, 0);
		expected_bytes(IT,N,sizeof(T), 2);

		Roi_Start(command.c_str());
		for(int j=0; j<IT; j++){
			vector_copy(a,b);
		}
		Roi_End(command.c_str());

	}
	else if (command.compare("all") == 0){

		Roi_Start("triad");
		for(int j=0; j<IT; j++){
			vector_triad(a,b,c,2.0);
		}
		Roi_End("triad");

		Roi_Start("sum");
		for(int j=0; j<IT; j++){
			vector_sum(a,b,c);
		}
		Roi_End("sum");

		Roi_Start("scale");
		for(int j=0; j<IT; j++){
			vector_scale(a,b,2.5);
		}
		Roi_End("scale");

	}
	else if (command.compare("delim") == 0){

		expected_fp("sum",IT, N, 1);
		expected_bytes(IT,N,sizeof(T), 3);

		delim_start();

		for(int j=0; j<IT; j++){
			vector_sum(a,b,c);
		}

		delim_stop();
	}
	else if (command.compare("func") == 0){

		expected_fp("sum",IT, N, 1);
		expected_bytes(IT,N,sizeof(T), 3);

		function_sum(a,b,c);
		function_sum(a,b,c);

	}
	else
		std::cout << "ERROR: >> Command " << command << " Not Found" << std::endl;
	delete[] a;
	delete[] b;
	delete[] c;
	return;
}




int main(int argc, char* argv[]){

	int opt;
	const char* short_options="i:s:c:";
	std::string command;
	struct option long_options[]=
	{
		{"iter", required_argument, NULL, 'i'},
		{"cmd", required_argument, NULL, 'c'},
		{"size", required_argument, NULL, 'o'}
	};
	while((opt = getopt_long(argc, argv, short_options, long_options, NULL )) != -1){
		switch(opt){
			case 'i':
				IT = atoi(optarg);
				std::cout << "Running for " << IT << " iterations" << std::endl;
				break;
			case 's':
				N = atoi(optarg);
				std::cout << "Array size is " << N << std::endl;
				break;
			case 'c':
				command = optarg;
				break;
		}
	}

	execute_command(command);

	return 0;

}
