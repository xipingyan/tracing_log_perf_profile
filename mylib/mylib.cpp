#include <iostream>
#include <vector>

#include "mylib.hpp"

void test_lib_fun()
{
    std::cout << "this is lib test fun." << std::endl;
    {
        std::cout << "  std::vector<int> vv(20);" << std::endl;
        std::vector<int> vv(20);
        vv[0] = 9;
        std::cout << "  std::vector<int> vv(20); release" << std::endl;
    }
    std::cout << "test_lib_fun end" << std::endl;
}
