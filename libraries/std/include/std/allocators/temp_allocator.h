#pragma once

#include "std/allocator_interface.h"

typedef struct std_TempAllocator {
	IAllocator interface;
	void* buffer;
	u64 size;
} TempAllocator;

// TempAllocator* std_new_temp_allocator();
