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
Each active scope retains its own sampled extrema over its **entire** lifetime,
including its callees; the scope event's `args` keep its actual start/end RSS
and peak. For the `Process RSS` chart, the current **deepest active scope** is
split into separate, nonoverlapping display blocks at each change of visible
scope: parent-before-child, child, parent-after-child, etc. Each block records
its own start/end readings and sampled extrema. A block's horizontal rectangle
uses its **maximum** if its end RSS exceeds its start, its **minimum** if it
falls, or its start value if unchanged. Its exit counter uses its actual end
reading. In particular, a parent's blocks before/after a child no longer use
a maximum reached inside the child. With concurrent threads, the deepest
active scope wins; ties go to the most recently entered scope. This priority
does **not** attribute memory to a function: RSS is still process-wide.
The sampler stores one summary per completed visible block, **not** one point
per 1 ms sample; retained chart data grows with scope switches, not with the
duration of a single scope.
Open the JSON in Chrome tracing or Perfetto to see RSS alongside the function
timeline. The JSON counter format cannot request a sloped line: the viewer
decides whether to join these points with sloped segments or steps. A sloped
segment is only a visual connection, **not** evidence that RSS changed linearly
between measurements.
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

## Build guide (OpenVINO 2026.4, Ubuntu 24.04)

Use the extracted stable OpenVINO package
`openvino_toolkit_ubuntu24_2026.4.0.22959.99c81491cc3_x86_64` in the
repository root. Run all commands in the **same shell**, starting from the
repository root; `setupvars.sh` sets `OpenVINO_DIR` for CMake and the runtime
library paths for the executable:

```sh
source openvino_toolkit_ubuntu24_2026.4.0.22959.99c81491cc3_x86_64/setupvars.sh
mkdir -p build
cd build
cmake ..
make -j20
ENABLE_PROFILE_MEM=ON ./myprofile
```

If `build` was configured against another OpenVINO installation, clear its
CMake cache or use an empty build directory before running `cmake ..`; otherwise
CMake may continue using the previously cached `OpenVINO_DIR`. With the commands
above, the trace is saved as `build/profile_myprofile.json` (relative to the
repository root).

## OpenVINO MatMul RSS example

The executable also builds a CPU OpenVINO model with one FP32 MatMul:
`[1, 4096] x [4096, 3072] -> [1, 3072]`. Its constant weight has
12,582,912 values (~50.3 MB). It compiles the model, runs one inference, then
explicitly releases the source model, infer request, compiled model, and Core.
Each step has its own trace scope (`openvino_create_model`,
`openvino_compile_model`, `openvino_release_model`,
`openvino_create_infer_request`, `openvino_infer`,
`openvino_release_infer_request`, `openvino_release_compiled_model`, and
`openvino_release_core`). The surrounding `openvino_lifecycle` scope records
the peak and overall RSS change.
The infer request is released **before** the compiled model so it cannot keep
the latter alive. RSS may not return to its original value after release:
OpenVINO, its CPU plugin, and the process allocator may retain memory or caches.
Look at both `rss_delta_mb` and `rss_peak_mb` rather than treating RSS as an
allocation/free counter.
