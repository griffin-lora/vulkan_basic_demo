#pragma once
#include "result.h"
#include <stddef.h>
#include <stdint.h>

extern uint8_t ds_bytes[];
extern void* ds_current_point;

inline void* ds_save(void) {
    return ds_current_point;
}

inline void ds_restore(void* point) {
    ds_current_point = point;
}

void* ds_promise(size_t num_bytes);
void* ds_push(size_t num_bytes);
result_t ds_push_chunks(size_t num_chunks, const size_t num_chunks_bytes[], void* chunks[]);