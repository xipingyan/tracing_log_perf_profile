#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Linux 内存统计默认关闭；只统计耗时时无需设置环境变量。
// 启动程序前设置 ENABLE_PROFILE_MEM=ON（也支持 TRUE、1；字母不区分大小写），
// 例如：export ENABLE_PROFILE_MEM=ON
// 启用后，trace 中会包含作用域的 RSS 起始值、结束值、增量及采样峰值，
// 并输出内存曲线；关闭时不读取 RSS，也不启动内存采样线程。
// 环境变量仅在进程启动时读取；目前内存统计仅支持 Linux。
class MyProfile
{
public:
    MyProfile() = delete;
    MyProfile(const std::string &name, const std::vector<std::pair<std::string, std::string>> &args = std::vector<std::pair<std::string, std::string>>());
    ~MyProfile();

private:
    std::string _name;
    uint64_t _ts1;
#ifdef __linux__
    friend class RssSampler;
    uint64_t _rss_start_bytes = 0;
    uint64_t _rss_peak_bytes = 0;
    uint64_t _rss_min_bytes = 0;
    bool _has_rss_start = false;
#endif
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
// Example 2: MY_PROFILE / MY_PROFILE_ARGS
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