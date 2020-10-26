#include<stdio.h>

#define Roi_Start(label) _RoiStart(label, __LINE__, __FILE__)
#define Roi_End(label) _RoiEnd(label, __LINE__, __FILE__)
void _RoiStart(const char* label, unsigned int __line, const char* __file) __attribute__((noinline, weak));
void _RoiEnd(const char* label, unsigned int __line, const char* __file) __attribute__((noinline, weak));


// Define a compiler barrier to prevent compiler reordering
void _RoiStart(const char* label, unsigned int __line, const char* __file){
}

// Define a compiler barrier to prevent compiler reordering
void _RoiEnd(const char* label, unsigned int __line, const char* __file){
}

