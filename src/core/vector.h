#ifndef VEC_H_
#define VEC_H_

#include <stdlib.h>
#include <stdio.h>

typedef struct vector_ {
    size_t capacity;
    size_t element_number;
    int element_size;
    void** items;
} vector;

vector* vector_init(int);
void vector_push(vector* vec, void* item);
#endif
