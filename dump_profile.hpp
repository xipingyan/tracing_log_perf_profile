#pragma once
#include <string>
#include <vector>

// Control whether or not tp save tracing log.
#define ENABLE_TRACE_LOG 1

// Control whether or not to print memory usage to a log file.
#include <features.h>
#if __GLIBC_PREREQ(2, 34)
// If glibc > 2.34, it removes __malloc_hook, so we have to disable memory statistic.
#define ENABLE_TRACE_MEM_USAGE 0
#else
#define ENABLE_TRACE_MEM_USAGE 1
#endif

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

#if ENABLE_TRACE_LOG
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

#define MY_PROFILE_VAR(VAR, NAME) auto VAR = MyProfile(NAME + std::string(":") + std::to_string(__LINE__))
#define MY_PROFILE_VAR_ARGS(VAR, NAME, ...) auto VAR = MyProfile(NAME + std::string(":") + std::to_string(__LINE__), __VA_ARGS__)
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
#else
#define MY_PROFILE_VAR(VAR, NAME) 
#define MY_PROFILE_VAR_ARGS(VAR, NAME, ...) 
#endif