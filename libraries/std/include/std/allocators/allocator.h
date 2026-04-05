#pragma once

#include "std/allocator_interface.h"

typedef struct std_Allocator {
	IAllocator allocator;
} Allocator;

// Allocator* std_new_default_allocator();
