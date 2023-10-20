#include <iostream>
#include <vector>
#include <thread>

#include "mylib.hpp"

void test_lib_fun()
{
    std::cout << "this is lib test fun." << std::endl;
    {
        std::cout << "  std::vector<int> vv(20);" << std::endl;
        std::vector<int> vv(20);
        vv[0] = 9;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "  std::vector<int> vv(20); release" << std::endl;
    }
    std::cout << "test_lib_fun end" << std::endl;
}
