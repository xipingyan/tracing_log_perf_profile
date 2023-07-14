#pragma once
#include <string>

class MyProfile
{
public:
    MyProfile() = delete;
    // MyProfile(MyProfile&)=delete;
    MyProfile(const std::string &name);
    ~MyProfile();
private:
    std::string _name;
    uint64_t _ts1;
};

// Example:
// ==========================================
// auto p = MyProfile("fun_name")
// Or
// {
//     auto p = MyProfile("fun_name")
//     func()
// }