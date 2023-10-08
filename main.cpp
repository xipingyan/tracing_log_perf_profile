#include <chrono>
#include <thread>
#include <iostream>

#include "dump_profile.hpp"
#define TIP_TEST() std::cout << "== Test: " << __FUNCTION__ << std::endl
void example_1()
{
    TIP_TEST();
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
    TIP_TEST();
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

void example_3()
{
    TIP_TEST();
    // Example: MY_PROFILE_MEM, MY_PROFILE_VAR
#define EXECUTE_FUN(FUN) std::cout << "Execute: " << #FUN << std::endl; FUN; MY_PROFILE_MEM()
    MY_PROFILE_MEM();
    MY_PROFILE_VAR(p, __FUNCTION__);
    EXECUTE_FUN(float *arr = new float[1024]);
    {
        auto p1 = MY_PROFILE("sleep_20");
        EXECUTE_FUN(float *arr2 = new float[1024]);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXECUTE_FUN(delete[] arr2);
    }
    {
        auto p2 = MY_PROFILE_ARGS("sleep_30", {{"arg1", "sleep 30 ms"}});
        
        EXECUTE_FUN(void *pbuf2 = malloc(1024));

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        
        EXECUTE_FUN(free(pbuf2));
    }

    EXECUTE_FUN(delete[] arr);
}

int main(int argc, char **argv)
{
    example_1();
    example_2();
#if ENABLE_TRACE_MEM_USAGE
    example_3();
#endif
    return 0;
}