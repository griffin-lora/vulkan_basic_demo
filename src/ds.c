#include "ds.h"
#include <stdalign.h>

#define NUM_DATA_STACK_BYTES 33554432 // 32 MiB

alignas(64) uint8_t ds_bytes[NUM_DATA_STACK_BYTES];
alignas(64) void* ds_current_point = ds_bytes;

void* ds_promise(size_t num_bytes) {
    size_t aligned_num_bytes = num_bytes % 64ul == 0 ? num_bytes : num_bytes + (64ul - (num_bytes % 64ul));
    if ((ds_current_point - (const void*)ds_bytes) + aligned_num_bytes >= NUM_DATA_STACK_BYTES) {
        return NULL;
    }

    return ds_current_point;
}

void* ds_push(size_t num_bytes) {
    void* old_point = ds_current_point;
    size_t aligned_num_bytes = num_bytes % 64ul == 0 ? num_bytes : num_bytes + (64ul - (num_bytes % 64ul));
    if ((ds_current_point - (const void*)ds_bytes) + aligned_num_bytes >= NUM_DATA_STACK_BYTES) {
        return NULL;
    }
    ds_current_point += aligned_num_bytes;

    return old_point;
}