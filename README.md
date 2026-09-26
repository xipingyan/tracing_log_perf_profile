# tracing_log_perf_profile
Profile performance of computational functions.

Branch ``main``: only print run-time tracing log; <br>
Branch ``enable_mem_statistic`` : Run-time tracing log + mem allocate/free size statistic; <br>
Branch ``python_profile``: [Python3]Profiling python codes.

Memory profiling is disabled by default. On Linux, set `ENABLE_PROFILE_MEM=ON`
(or `TRUE` or `1`) in the environment before starting the program to enable
memory data in the trace. For example: `export ENABLE_PROFILE_MEM=ON`.
The values `ON` and `TRUE` are case-insensitive. Unset, empty, and other
values disable memory profiling. When disabled, the profiler does not read
RSS or start a memory-sampling thread; time profiling remains active. The
setting is read once at process startup.

When enabled, each profiled scope writes numeric `rss_start_mb`, `rss_end_mb`,
`rss_delta_mb`, and `rss_peak_mb` to the event's `args` in
`profile_<executable>.json`. A shared background thread samples process RSS
every 1 ms while scopes are active. The peak includes the entry, exit, and
all samples during the scope, even if memory is freed before the scope ends.
The trace contains `Process RSS` counter events at scope entry, peak, and exit,
plus a separate `Peak RSS` counter at each scope exit (the peak for that scope).
Open the JSON in Chrome tracing or Perfetto to see both memory graphs aligned
with the function timeline.
Values are decimal megabytes (1 MB = 1,000,000 bytes).
These are process-wide resident memory measurements, not per-function allocation
totals. Other threads can affect the values, and peaks shorter than the 1 ms
sampling interval may be missed. `rss_delta_mb` can be negative; `rss_peak_mb`
is the maximum observed process RSS during the scope. If an RSS read at entry
or exit fails, the memory fields are omitted for that event.

The sample executable profiles `new[]`, `malloc`, and vector `push_back` in
separate scopes. Each allocation stays alive until its scope ends, so the
corresponding event retains a measurable `rss_delta_mb`.
The `transient_mmap` example allocates and frees memory inside a single scope;
when memory profiling is enabled, its `rss_peak_mb` should exceed both
`rss_start_mb` and `rss_end_mb`.
