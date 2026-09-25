# tracing_log_perf_profile
Profile performance of computational functions.

Branch ``main``: only print run-time tracing log; <br>
Branch ``enable_mem_statistic`` : Run-time tracing log + mem allocate/free size statistic; <br>
Branch ``python_profile``: [Python3]Profiling python codes.

On Linux, each profiled scope writes numeric `rss_start_mb`, `rss_end_mb`, and
`rss_delta_mb` to the event's `args` in `profile_<executable>.json`.
The same trace also contains `Process RSS` counter events at each scope's entry
and exit, with `rss_mb` values. Open the JSON in Chrome tracing or Perfetto to
see a memory graph aligned with the function timeline. The graph is sampled at
scope boundaries, so it does not show peaks between those points.
Values are decimal megabytes (1 MB = 1,000,000 bytes).
These are process-wide resident memory snapshots at scope entry and exit;
the delta may be negative and is not the function's allocation total or peak
memory usage. Other threads can affect these values. If an RSS read fails,
the memory fields are omitted for that event.

The sample executable profiles `new[]`, `malloc`, and vector `push_back` in
separate scopes. Each allocation stays alive until its scope ends, so the
corresponding event retains a measurable `rss_delta_mb`.
