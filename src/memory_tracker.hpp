#pragma once
#include <cstddef>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

class MemoryTracker {
public:
	// Start tracking memory
	// sample_interval_ms = interval in milliseconds (can be fractional)
	void begin(float sample_interval_ms = 10.0f) { // default 10ms
		running = false;
		samples.clear();
		samples.push_back(0);
		interval_ms = sample_interval_ms;
		start_time = std::chrono::steady_clock::now();
		last_sample_time = start_time;
		cumulative_allocated = 0;
		running = true;
	}

	// Stop tracking and return the memory usage samples
	std::vector<size_t> end() {
		running = false;

		{
			std::lock_guard<std::mutex> guard(mutex);
			samples.push_back(cumulative_allocated);
		}

		return samples;
	}

	// Called internally by overloaded new/delete
	void record() {
		if (!running || internalPush) return;
		auto now = std::chrono::steady_clock::now();
		float elapsed_ms = std::chrono::duration<float, std::milli>(now - last_sample_time).count();
		if (elapsed_ms >= interval_ms) {
			last_sample_time = now;
			std::lock_guard<std::mutex> guard(mutex);
			internalPush = true;
			samples.push_back(cumulative_allocated);
			internalPush = false;
		}
	}

	static std::atomic<size_t> total_allocated;
	size_t cumulative_allocated{ 0 };

private:
	std::vector<size_t> samples;
	std::mutex mutex;
	std::chrono::steady_clock::time_point start_time;
	std::chrono::steady_clock::time_point last_sample_time;
	float interval_ms{ 10.0f }; // default 10ms
	bool running{ false };
	bool internalPush{ false };
};

// Global instance
extern MemoryTracker g_MemoryTracker;

// new/delete overloads
void* operator new(std::size_t sz);
void operator delete(void* ptr, std::size_t sz) noexcept;
void* operator new[](std::size_t sz);
void operator delete[](void* ptr, std::size_t sz) noexcept;

#define START_MEMORY_TRACKING(interval) g_MemoryTracker.begin(0.01)
#define END_MEMORY_TRACKING() g_MemoryTracker.end()
