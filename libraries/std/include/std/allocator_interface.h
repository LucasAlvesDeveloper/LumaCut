#pragma once

#include "std/ints.h"
typedef void* (*AllocateFn)(void* data, size size);
typedef void (*DeallocateFn)(void* data);

typedef struct IAllocator {
	AllocateFn alloc;
	DeallocateFn free;
} IAllocator;
