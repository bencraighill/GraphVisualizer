#include "memory_tracker.hpp"
#include <cstdlib>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/truetype/stb_truetype.h"

MemoryTracker g_MemoryTracker;
std::atomic<size_t> MemoryTracker::total_allocated{ 0 };

void* operator new(std::size_t sz) {
	MemoryTracker::total_allocated += sz;
	g_MemoryTracker.cumulative_allocated += sz;
	g_MemoryTracker.record();
	void* ptr = std::malloc(sz);
	if (!ptr) throw std::bad_alloc();
	return ptr;
}

void operator delete(void* ptr, std::size_t sz) noexcept {
	MemoryTracker::total_allocated -= sz;
	g_MemoryTracker.record();
	std::free(ptr);
}

void* operator new[](std::size_t sz) { return operator new(sz); }
void operator delete[](void* ptr, std::size_t sz) noexcept { operator delete(ptr, sz); }
