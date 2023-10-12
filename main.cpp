#include <chrono>
#include <thread>
#include <iostream>

#include "dump_profile.hpp"
#include "mylib/mylib.hpp"

#define TIP_TEST() std::cout << "== Test: " << __FUNCTION__ << std::endl
void example_1()
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

void example_2()
{
    TIP_TEST();
    // Example: MY_PROFILE_MEM, MY_PROFILE_VAR
#define EXECUTE_FUN(FUN) std::cout << "Execute: " << #FUN << std::endl; FUN; MY_PROFILE_MEM()
    MY_PROFILE_MEM();
    MY_PROFILE_VAR(p, __FUNCTION__);
    EXECUTE_FUN(float *arr = new float[1024]);
    {
        MY_PROFILE_VAR(p1, "sleep_20");
        EXECUTE_FUN(float *arr2 = new float[1024]);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXECUTE_FUN(delete[] arr2);
    }
    {
        MY_PROFILE_VAR_ARGS(p2, "sleep_30", {{"arg1", "sleep 30 ms"}});
        
        EXECUTE_FUN(void *pbuf2 = malloc(1024));

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        
        EXECUTE_FUN(free(pbuf2));
    }

    EXECUTE_FUN(delete[] arr);
}

void example_2_1()
{
    TIP_TEST();
#define EXECUTE_FUN_2_1(FUN) std::cout << "Execute: " << #FUN << std::endl; FUN
    MY_PROFILE_VAR_MEM(p, __FUNCTION__);
    {
        MY_PROFILE_VAR_MEM(p1, "sleep_20");
        EXECUTE_FUN_2_1(float *arr2 = new float[1024]);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXECUTE_FUN_2_1(delete[] arr2);
    }
    {
        MY_PROFILE_VAR_MEM(p1, "sleep_30");
        EXECUTE_FUN_2_1(std::vector<float> vv(30););
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void example_3() {
    TIP_TEST();
    MY_PROFILE_VAR(p, __FUNCTION__);
    MY_PROFILE_MEM();
    test_lib_fun();
    MY_PROFILE_MEM();
}

int main(int argc, char **argv)
{
    example_1();
#if ENABLE_TRACE_MEM_USAGE
    example_2();
    example_2_1();
#endif
    example_3();
    return 0;
}