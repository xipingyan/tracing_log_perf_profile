# tracing_log_perf_profile
Profile performance of computational functions.

Branch ``'main'``: [CPP]only print run-time tracing log; <br>
Branch ``'enable_mem_statistic'`` : [CPP]Run-time tracing log + mem allocate/free size statistic; <br>
Branch ``'python_profile'``: [Python3]Profiling python codes.

# How to use for python

`Note`: Global variable `g_profiling_uss` controls whether or not memory USS is profiled.

Here is example:
```
import time
import sys
from my_py_profile import MyProfile

def example_1():
    print(f"Execute: {sys._getframe().f_code.co_name}")
    p = MyProfile("example_1")
    time.sleep(0.5)

def main():
    example_1()

if __name__ == "__main__":
    main()

```

More details, please refer: test.py
