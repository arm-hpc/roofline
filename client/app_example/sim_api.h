#include<time.h>
#include<stdio.h>

void SimRoiStart(void) __attribute__((noinline));
void SimRoiEnd (void) __attribute__((noinline));

clock_t start,end;
double elapsed;

// Define a compiler barrier to prevent
// compiler reordering
void SimRoiStart(void){
#ifdef TIME_RUN
	start = clock();
#endif
	__asm__ volatile("" ::: "memory");
}

// Define a compiler barrier to prevent
// compiler reordering
void SimRoiEnd(void){
	__asm__ volatile("" ::: "memory");
#ifdef TIME_RUN
	end = clock();
	elapsed = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("Raw CPU Time elapsed is: %f\n", elapsed);
#endif
}
