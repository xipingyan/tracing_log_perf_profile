#include "dump_profile.hpp"
#include <thread>
#include <sstream>
#include <atomic>
#include <mutex>
#include <vector>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#ifdef __linux__
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <intrin.h>
#include <Windows.h>
#else
#include <x86intrin.h>
#include <dlfcn.h>
#endif
#pragma intrinsic(__rdtsc)

#ifdef __linux__
static bool memory_profiling_enabled()
{
    const char *value = std::getenv("ENABLE_PROFILE_MEM");
    return value != nullptr &&
           (strcasecmp(value, "ON") == 0 || strcasecmp(value, "TRUE") == 0 ||
            std::strcmp(value, "1") == 0);
}

static bool read_rss_bytes(uint64_t &rss_bytes)
{
    FILE *file = fopen("/proc/self/statm", "r");
    if (file == nullptr)
        return false;

    unsigned long long total_pages = 0;
    unsigned long long resident_pages = 0;
    int read_count = fscanf(file, "%llu %llu", &total_pages, &resident_pages);
    fclose(file);
    long page_size = sysconf(_SC_PAGESIZE);
    if (read_count != 2 || page_size <= 0)
        return false;

    rss_bytes = resident_pages * static_cast<uint64_t>(page_size);
    return true;
}

// One sampling thread serves all active scopes; the lock also protects each
// scope's peak from concurrent updates during its destruction.
class RssSampler
{
    const bool _enabled = memory_profiling_enabled();
    std::mutex _mutex;
    std::condition_variable _wake;
    std::vector<MyProfile *> _active;
    bool _stopping = false;
    std::thread _worker;

    void run()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (!_stopping)
        {
            if (_active.empty())
            {
                _wake.wait(lock, [this] { return _stopping || !_active.empty(); });
                continue;
            }
            if (_wake.wait_for(lock, std::chrono::milliseconds(1),
                               [this] { return _stopping || _active.empty(); }))
                continue;

            uint64_t rss_bytes = 0;
            if (read_rss_bytes(rss_bytes))
            {
                uint64_t timestamp = __rdtsc();
                for (MyProfile *profile : _active)
                {
                    if (rss_bytes > profile->_rss_peak_bytes)
                    {
                        profile->_rss_peak_bytes = rss_bytes;
                        profile->_rss_peak_tsc = timestamp;
                    }
                }
            }
        }
    }

public:
    RssSampler()
    {
        if (_enabled)
            _worker = std::thread(&RssSampler::run, this);
    }
    RssSampler(const RssSampler &) = delete;
    RssSampler &operator=(const RssSampler &) = delete;

    ~RssSampler()
    {
        if (!_enabled)
            return;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stopping = true;
        }
        _wake.notify_one();
        _worker.join();
    }

    bool enabled() const { return _enabled; }

    void add(MyProfile *profile)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _active.push_back(profile);
        }
        _wake.notify_one();
    }

    bool finish(MyProfile *profile, uint64_t &rss_end_bytes, uint64_t &end_tsc)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        bool has_end = read_rss_bytes(rss_end_bytes);
        end_tsc = __rdtsc();
        if (has_end && rss_end_bytes > profile->_rss_peak_bytes)
        {
            profile->_rss_peak_bytes = rss_end_bytes;
            profile->_rss_peak_tsc = end_tsc;
        }
        auto it = std::find(_active.begin(), _active.end(), profile);
        if (it != _active.end())
            _active.erase(it);
        if (_active.empty())
            _wake.notify_one();
        return has_end;
    }
};
#endif

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
#ifdef __linux__
    bool has_rss = false;
    double rss_start_mb = 0;
    double rss_end_mb = 0;
    double rss_peak_mb = 0;
    uint64_t rss_peak_tsc = 0;
#endif
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
        if (err != 0) {
            printf("Can't fopen:%s", json_fn.c_str());
            return;
        }
#else
        pf = fopen(json_fn.c_str(), "wb");
        if (nullptr == pf) {
            printf("Can't fopen:%s", json_fn.c_str());
            return;
        }
#endif

        // Headers
        fprintf(pf, "{\n\"schemaVersion\": 1,\n\"traceEvents\":[\n");

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
#ifdef __linux__
            if (itm.has_rss)
            {
                fprintf(pf, "\"rss_start_mb\":%.6f,\"rss_end_mb\":%.6f,\"rss_delta_mb\":%.6f,\"rss_peak_mb\":%.6f%s",
                        itm.rss_start_mb, itm.rss_end_mb, itm.rss_end_mb - itm.rss_start_mb, itm.rss_peak_mb,
                        itm.vecArgs.empty() ? "" : ",");
            }
#endif
            for (size_t j = 0; j < itm.vecArgs.size(); j++)
            {
                fprintf(pf, "\"%s\":\"%s\"%s", itm.vecArgs[j].first.c_str(), itm.vecArgs[j].second.c_str(), j + 1 == itm.vecArgs.size() ? "" : ",");
            }
            fprintf(pf, "}}");
#ifdef __linux__
            if (itm.has_rss)
            {
                fprintf(pf, ",\n{\"name\":\"Process RSS\",\"cat\":\"memory\",\"ph\":\"C\",\"pid\":\"%s\",\"ts\":%s,\"args\":{\"rss_mb\":%.6f}}",
                        itm.pid.c_str(), tsc_to_nsec(itm.ts1).c_str(), itm.rss_start_mb);
                if (itm.rss_peak_tsc != itm.ts1 && itm.rss_peak_tsc != itm.ts2)
                    fprintf(pf, ",\n{\"name\":\"Process RSS\",\"cat\":\"memory\",\"ph\":\"C\",\"pid\":\"%s\",\"ts\":%s,\"args\":{\"rss_mb\":%.6f}}",
                        itm.pid.c_str(), tsc_to_nsec(itm.rss_peak_tsc).c_str(), itm.rss_peak_mb);
                fprintf(pf, ",\n{\"name\":\"Process RSS\",\"cat\":\"memory\",\"ph\":\"C\",\"pid\":\"%s\",\"ts\":%s,\"args\":{\"rss_mb\":%.6f}}",
                        itm.pid.c_str(), tsc_to_nsec(itm.ts2).c_str(), itm.rss_end_mb);
                fprintf(pf, ",\n{\"name\":\"Peak RSS\",\"cat\":\"memory\",\"ph\":\"C\",\"pid\":\"%s\",\"ts\":%s,\"args\":{\"peak_mb\":%.6f}}",
                    itm.pid.c_str(), tsc_to_nsec(itm.ts2).c_str(), itm.rss_peak_mb);
            }
#endif
            fprintf(pf, "%s\n", i == _vecItems.size() - 1 ? "" : ",");
        }

        fprintf(pf, "]\n}\n");
        fclose(pf);
        printf("Profiler log is saved to: %s\n", json_fn.c_str());
    }
};
static ProfilerManager g_profileManage;
#ifdef __linux__
static RssSampler g_rssSampler;
#endif
MyProfile::MyProfile(const std::string &name, const std::vector<std::pair<std::string, std::string>> &args)
{
    _name = name;
    _args = args;
#ifdef __linux__
    if (g_rssSampler.enabled())
        _has_rss_start = read_rss_bytes(_rss_start_bytes);
#endif
    _ts1 = __rdtsc();
#ifdef __linux__
    if (_has_rss_start)
    {
        _rss_peak_bytes = _rss_start_bytes;
        _rss_peak_tsc = _ts1;
        g_rssSampler.add(this);
    }
#endif
}

MyProfile::~MyProfile()
{
    dump_items itm;
#ifdef __linux__
    uint64_t rss_end_bytes = 0;
    bool has_rss_end = _has_rss_start && g_rssSampler.finish(this, rss_end_bytes, itm.ts2);
    if (!_has_rss_start)
        itm.ts2 = __rdtsc();
    if (has_rss_end)
    {
        itm.has_rss = true;
        itm.rss_start_mb = static_cast<double>(_rss_start_bytes) / 1000000.0;
        itm.rss_end_mb = static_cast<double>(rss_end_bytes) / 1000000.0;
        itm.rss_peak_mb = static_cast<double>(_rss_peak_bytes) / 1000000.0;
        itm.rss_peak_tsc = _rss_peak_tsc;
    }
#else
    itm.ts2 = __rdtsc();
#endif
    itm.ts1 = _ts1;
    itm.name = _name;
    itm.tid = get_thread_id();
    itm.cat = "PERF";
    itm.vecArgs.insert(itm.vecArgs.end(), _args.begin(), _args.end());
    g_profileManage.add(itm);
}
