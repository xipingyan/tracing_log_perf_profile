#pragma once
#include <string>
#include <vector>

// Control whether or not to print memory usage to a log file.
#ifndef ENABLE_TRACE_MEM_USAGE
#define ENABLE_TRACE_MEM_USAGE 1
#endif // !ENABLE_TRACE_MEM_USAGE

#if ENABLE_TRACE_MEM_USAGE

#ifndef CAT
#define TOKEN_PASTE(x, y) x##y
#define CAT(x,y) TOKEN_PASTE(x,y)
#endif // CAT

class MyProfileMem
{
public:
    MyProfileMem();
    ~MyProfileMem() = default;
};
#define MY_PROFILE_MEM() auto CAT(var_, __LINE__) = MyProfileMem()
#else
#define MY_PROFILE_MEM()
#endif // !ENABLE_TRACE_MEM_USAGE

class MyProfile
{
public:
    MyProfile() = delete;
    MyProfile(const std::string &name, const std::vector<std::pair<std::string, std::string>> &args = std::vector<std::pair<std::string, std::string>>());
    ~MyProfile();

private:
    std::string _name;
    uint64_t _ts1;
    std::vector<std::pair<std::string, std::string>> _args;
};

#define MY_PROFILE(NAME) MyProfile(NAME + std::string(":") + std::to_string(__LINE__))
#define MY_PROFILE_ARGS(NAME, ...) MyProfile(NAME + std::string(":") + std::to_string(__LINE__), __VA_ARGS__)
// Example 1: MY_PROFILE / MY_PROFILE_ARGS
/******************************************************
auto p = MY_PROFILE("fun_name")
Or
{
    auto p = MY_PROFILE("fun_name")
    func()
}
Or
{
    auto p2 = MY_PROFILE_ARGS("fun_name", {{"arg1", "sleep 30 ms"}});
    func()
}
******************************************************/

#define MY_PROFILE_VAR(VAR, NAME) auto VAR = MY_PROFILE(NAME)
#define MY_PROFILE_VAR_ARGS(VAR, NAME, ...) auto VAR = MY_PROFILE_ARGS(NAME, __VA_ARGS__)
// Example 2: MY_PROFILE_VAR / MY_PROFILE_VAR_ARGS
/******************************************************
MY_PROFILE_VAR(p, "fun_name")
Or
{
    MY_PROFILE_VAR(p1, "fun_name")
    func()
}
Or
{
    MY_PROFILE_VAR_ARGS(p2, "fun_name", {{"arg1", "sleep 30 ms"}});
    func()
}
******************************************************/