#pragma once

#include <gperftools/malloc_hook.h>
#include <vector>
#include <thread>
#include <atomic>

static size_t memory_current = 0;
static auto new_hook = [](const void*, size_t size) {  memory_current += size; };
static std::vector<size_t> result;
static std::atomic<bool> timer(false);
static std::thread t;

inline void run_mem(std::atomic<bool>& flag) {
    MallocHook::AddNewHook(new_hook);
    using namespace std::chrono_literals;
    while (true) {
        std::this_thread::sleep_for(100ns);
        if (flag.load()) { break; }
        result.push_back(memory_current);
    }
    MallocHook::RemoveNewHook(new_hook);
}

inline void start_mem() {
    memory_current = 0;
    result.clear();
    timer.store(false);
    t = std::thread(run_mem, std::ref(timer));
}

inline std::vector<size_t> end_mem() {
    timer.store(true);
    t.join();
    return result;
}
