#include "dump_profile.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>
#include <vector>

void example_1()
{
    // Example: MY_PROFILE, MY_PROFILE_ARGS
    auto p = MY_PROFILE(__FUNCTION__);
    {
        auto p1 = MY_PROFILE("sleep_20");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    {
        auto p2 = MY_PROFILE_ARGS("sleep_30", {{"arg1", "sleep 30 ms"}});
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}
void example_2()
{
    // Example: MY_PROFILE_VAR, MY_PROFILE_VAR_ARGS
    MY_PROFILE_VAR(p, __FUNCTION__);
    {
        MY_PROFILE_VAR(p1, "sleep_20");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    {
        MY_PROFILE_VAR_ARGS(p2, "sleep_30", {{"arg1", "sleep 30 ms"}});
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}
void example_memory()
{
    constexpr size_t allocation_size = 8 * 1024 * 1024;
    unsigned char *new_buffer = nullptr;
    void *malloc_buffer = nullptr;
    std::vector<int> values;

    {
        auto profile = MY_PROFILE("new_array");
        new_buffer = new unsigned char[allocation_size];
        std::memset(new_buffer, 1, allocation_size);
    }
    {
        auto profile = MY_PROFILE("malloc");
        malloc_buffer = std::malloc(allocation_size);
        if (malloc_buffer == nullptr)
            throw std::bad_alloc();
        std::memset(malloc_buffer, 1, allocation_size);
    }
    // release new.
    {
        auto profile = MY_PROFILE("release_new");
        delete[] new_buffer;
        new_buffer = nullptr;
    }

    {
        auto profile = MY_PROFILE("vector_append");
        for (int index = 0; index < 2 * 1024 * 1024; ++index)
            values.push_back(index);
    }

    delete[] new_buffer;
    std::free(malloc_buffer);
}
int main(int argc, char **argv)
{
    example_1();
    example_2();
    example_memory();
    return 0;
}