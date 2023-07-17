#include "dump_profile.hpp"
#include <chrono>
#include <thread>

int main(int argc, char **argv)
{
    auto p = MyProfile("main");
    {
        auto p1 = MyProfile("sleep_20");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    {
        auto p2 = MyProfile("sleep_30", {{"arg1", "sleep 30 ms"}});
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    return 0;
}