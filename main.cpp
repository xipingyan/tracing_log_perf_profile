#include "dump_profile.hpp"
#include <chrono>
#include <thread>

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
int main(int argc, char **argv)
{
    example_1();
    example_2();
    return 0;
}