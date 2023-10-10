#include "dump_profile.hpp"
#include "hook_mem_record.hpp"
#include <thread>
#include <sstream>
#include <atomic>
#include <mutex>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <intrin.h>
#include <Windows.h>
#else
#include <x86intrin.h>
#include <dlfcn.h>
#endif
#pragma intrinsic(__rdtsc)

#if ENABLE_TRACE_MEM_USAGE
struct dump_mem_items
{
    std::string name;      // The name of the event, as displayed in Trace Viewer
    std::string ph = "C";  // The event type, 'B'/'E' OR 'X'
    std::string pid = "0"; // The process ID for the process
    std::string tid;
    uint64_t ts = 0;
    // Args
    std::vector<std::pair<std::string, std::string>> vecArgs;
};
#endif // ENABLE_TRACE_MEM_USAGE

struct dump_items
{
    std::string name;      // The name of the event, as displayed in Trace Viewer
    std::string cat;       // The event categories
    std::string ph = "X";  // The event type, 'B'/'E' OR 'X'
    std::string pid = "0"; // The process ID for the process
    std::string tid;       // The thread ID for the thread that output this event.
    uint64_t ts1 = 0;      // The tracing clock timestamp of the event, [microsecond]
    uint64_t ts2 = 0;      // Duration = ts2 - ts1.
    std::string tts;       // Optional. The thread clock timestamp of the event
    std::vector<std::pair<std::string, std::string>> vecArgs;
};

static inline std::string get_thread_id()
{
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
}

static uint64_t rdtsc_calibrate(int seconds = 1)
{
    uint64_t start_ticks = __rdtsc();
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return (__rdtsc() - start_ticks) / seconds;
}

inline std::string location(void)
{
#ifdef _WIN32
	TCHAR path[1024];
	::GetModuleFileNameA(NULL, path, 1024);
	std::string dll_fn = std::string(path);
    std::string fn = dll_fn.substr(dll_fn.find_last_of("\\") + 1, dll_fn.length());
#else
    Dl_info info;
    dladdr(reinterpret_cast<void *>(&location), &info);
    std::string dll_fn = std::string(info.dli_fname);
    std::string fn = dll_fn.substr(dll_fn.find_last_of("/") + 1, dll_fn.length());
#endif

    fn = fn.substr(0, fn.find_last_of("."));
    return fn;
}

class ProfilerManager
{
protected:
    std::vector<dump_items> _vecItems;
    #if ENABLE_TRACE_MEM_USAGE
    std::vector<dump_mem_items> _vecMemItems;
    #endif
    std::atomic<uint64_t> tsc_ticks_per_second{0};
    std::atomic<uint64_t> tsc_ticks_base{0};
    std::mutex _mutex;

public:
    ProfilerManager()
    {
        if (tsc_ticks_per_second == 0)
        {
            uint64_t expected = 0;
            auto tps = rdtsc_calibrate();
            tsc_ticks_per_second.compare_exchange_strong(expected, tps);
            std::cout << "=== ProfilerManager: tsc_ticks_per_second = " << tsc_ticks_per_second << std::endl;
            tsc_ticks_base.compare_exchange_strong(expected, __rdtsc());
            std::cout << "=== ProfilerManager: tsc_ticks_base = " << tsc_ticks_base << std::endl;
        }
    }

    ProfilerManager(ProfilerManager &other) = delete;
    void operator=(const ProfilerManager &) = delete;
    ~ProfilerManager()
    {
        // Save tracing log to json file.
        save_to_json();
    }

    void add(const dump_items &val)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _vecItems.emplace_back(val);
    }

#if ENABLE_TRACE_MEM_USAGE
    void add_mem(const dump_mem_items &val)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _vecMemItems.emplace_back(val);
    }
#endif // ENABLE_TRACE_MEM_USAGE

private:
    std::string tsc_to_nsec(uint64_t tsc_ticks)
    {
        double val = (tsc_ticks - tsc_ticks_base) * 1000000.0 / tsc_ticks_per_second;
        return std::to_string(val);
    }

    std::string tsc_to_nsec(uint64_t start, uint64_t end)
    {
        double val = (end - start) * 1000000.0 / tsc_ticks_per_second;
        return std::to_string(val);
    }

    void save_to_json()
    {
        std::string json_fn = "profile_" + location() + ".json";
        FILE *pf;
#ifdef _WIN32
        errno_t err = fopen_s(&pf, json_fn.c_str(), "wb");
        if (err != 0)
#else
        pf = fopen(json_fn.c_str(), "wb");
        if (nullptr == pf)
#endif
        {
            printf("Can't fopen:%s", json_fn.c_str());
            return;
        }

        // Headers
        fprintf(pf, "{\n\"schemaVersion\": 1,\n\"traceEvents\":[\n");

// Save mem satistic.
#if ENABLE_TRACE_MEM_USAGE
        for (size_t i = 0; i < _vecMemItems.size(); i++)
        {
            auto &itm = _vecMemItems[i];
            // Write 1 event
            fprintf(pf, "{");
            fprintf(pf, "\"name\":\"%s\",", itm.name.c_str());
            fprintf(pf, "\"ph\":\"%s\",", itm.ph.c_str());
            fprintf(pf, "\"pid\":\"%s\",", itm.pid.c_str());
            fprintf(pf, "\"ts\":\"%s\",", tsc_to_nsec(itm.ts).c_str());
            fprintf(pf, "\"args\":{");
            for (size_t j = 0; j < itm.vecArgs.size(); j++)
            {
                fprintf(pf, "\"%s\":\"%s\"%s", itm.vecArgs[j].first.c_str(), itm.vecArgs[j].second.c_str(), j + 1 == itm.vecArgs.size() ? "" : ",");
            }
            fprintf(pf, "}},\n");
        }
        fprintf(pf, "{}%s\n", (_vecItems.size() == 0) ? "" : ",");
#endif

        for (size_t i = 0; i < _vecItems.size(); i++)
        {
            auto &itm = _vecItems[i];
            // Write 1 event
            fprintf(pf, "{");
            fprintf(pf, "\"name\":\"%s\",", itm.name.c_str());
            fprintf(pf, "\"cat\":\"%s\",", itm.cat.c_str());
            fprintf(pf, "\"ph\":\"%s\",", itm.ph.c_str());
            fprintf(pf, "\"pid\":\"%s\",", itm.pid.c_str());
            fprintf(pf, "\"tid\":\"%s\",", itm.tid.c_str());
            fprintf(pf, "\"ts\":\"%s\",", tsc_to_nsec(itm.ts1).c_str());
            fprintf(pf, "\"dur\":\"%s\",", tsc_to_nsec(itm.ts1, itm.ts2).c_str());
            fprintf(pf, "\"args\":{");
            for (size_t j = 0; j < itm.vecArgs.size(); j++)
            {
                fprintf(pf, "\"%s\":\"%s\"%s", itm.vecArgs[j].first.c_str(), itm.vecArgs[j].second.c_str(), j + 1 == itm.vecArgs.size() ? "" : ",");
            }
            fprintf(pf, "}},\n");
        }
        if (_vecItems.size() > 0)
        {
            fprintf(pf, "{}\n");
        }

        fprintf(pf, "]\n}\n");
        fclose(pf);
        printf("Profiler log is saved to: %s\n", json_fn.c_str());
    }
};

static ProfilerManager g_profileManage;

#if ENABLE_TRACE_LOG
MyProfile::MyProfile(const std::string &name, const std::vector<std::pair<std::string, std::string>> &args)
{
    _name = name;
    _args = args;
    _ts1 = __rdtsc();
}

MyProfile::~MyProfile()
{
    dump_items itm;
    itm.ts2 = __rdtsc();
    itm.ts1 = _ts1;
    itm.name = _name;
    itm.tid = get_thread_id();
    itm.cat = "PERF";
    itm.vecArgs = _args;
    g_profileManage.add(itm);
}
#endif // !ENABLE_TRACE_LOG

#if ENABLE_TRACE_MEM_USAGE
MyProfileMem::MyProfileMem()
{
    dump_mem_items itm;
    itm.ts = __rdtsc();
    itm.name = "mem";
    itm.tid = get_thread_id();
    auto mem_size = RecordMem::getInstance()->getMemSize();
    itm.vecArgs.push_back({"cats", std::to_string(mem_size)});
    g_profileManage.add_mem(itm);
}
#endif // !ENABLE_TRACE_MEM_USAGE