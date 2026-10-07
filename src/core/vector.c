#include "vector.h"
#include "log.h"
#include <stdio.h>

vector* vector_init(int element_size){
    vector* vec = (vector*) malloc(sizeof(vector));
    vec->element_size = element_size;
    vec->element_number = 0;
    vec->capacity = 1;
    vec->items = (void**) malloc(vec->capacity * sizeof(void*));
    return vec;
}

void vector_push(vector* vec, void* item){
    if(vec->capacity == vec->element_number){
        size_t new_cap = vec->capacity * 2;
        void** new_element = realloc(vec->items, new_cap * sizeof(void*));
        if(!new_element){
            log_message(LOG_LEVEL_ERROR, "error in allocating space for realloc");
            return; 
        }
        vec->items = new_element;
        vec->capacity = new_cap;
    }
    vec->items[vec->element_number++] = item;
}
