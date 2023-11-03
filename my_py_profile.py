import time
import json
import threading
from io import open
import psutil
import sys

# Control if profiling memory(USS)
g_profiling_uss=False

class ITMS:
    _name=""
    _ts1=0
    _dur=0
    _kwargs={}
    def __str__(self) -> str:
        return f"name={self._name}, ts1={self._ts1}, dur={self._dur}, kwargs={self._kwargs}"

class ManageProfile:
    _itms_tracing=[]
    _itms_mems=[]
    def add_tracing(self, itm):
        self._itms_tracing.append(itm)
    def add_mem(self, itm):
        self._itms_mems.append(itm)
    def __del__(self):
        # Save tracing log to json
        dict_itms=[]
        for itm in self._itms_tracing:
            dict_itm=self.create_tracing_itm(itm)
            dict_itms.append(dict_itm)
            # print(f'dict_itm={dict_itm}')
        # Save mem to json
        for itm in self._itms_mems:
            dict_itm=self.create_mem_itm(itm)
            dict_itms.append(dict_itm)
            # print(f'mem dict_itm={dict_itm}')

        dictionary={"schemaVersion": 1,"traceEvents":dict_itms}
        self.save(dictionary)
    
    def create_tracing_itm(self, itm):
        dict_itm={}
        dict_itm["name"]=itm._name
        dict_itm["cat"]="PERF"
        dict_itm["ph"]="X"
        dict_itm["pid"]="0"
        dict_itm["tid"]=threading.current_thread().ident
        dict_itm["ts"]=itm._ts1*1000000 # Unit 1000ns
        dict_itm["dur"]=itm._dur*1000000
        dict_itm["args"]=itm._kwargs
        return dict_itm

    def create_mem_itm(self, itm):
        dict_itm={}
        dict_itm["name"]="mem"
        dict_itm["ph"]="C"
        dict_itm["pid"]="0"
        dict_itm["ts"]=itm._ts1*1000000
        itm._kwargs["mem_size"]=itm._kwargs["mem_size"]-g_min_uss
        dict_itm["args"]=itm._kwargs
        return dict_itm

    # Save to json file.
    def save(self, dictionary):
        # Serializing json
        json_object = json.dumps(dictionary, indent=4)
        # Writing to myprofile.json
        json_fn = "myprofile.json"
        with open(json_fn, "w") as outfile:
            outfile.write(json_object)
        print(f'Saved profiling result to {json_fn}')

g_manage_profile=ManageProfile()

# Release 'g_manage_profile' manually in order to trigger save result json file.
def profile_finish():
    global g_manage_profile
    del g_manage_profile

# It's easy to compare and view when we minus min value.
g_min_uss=sys.maxsize

class MyProfile:
    def __init__(self, name, **kwargs):
        self._ts1 = time.perf_counter()
        self._name = name
        self._kwargs=kwargs

        global g_profiling_uss
        if g_profiling_uss:
            self._mem_info=psutil.Process().memory_full_info()
            global g_min_uss
            g_min_uss=min(g_min_uss, self._mem_info.uss)

        # print(f'Profiling start: {self._name}')
    def __del__(self):
        global g_manage_profile
        global g_profiling_uss

        ts2=time.perf_counter()

        if g_profiling_uss:
            mem_info=psutil.Process().memory_full_info()
            global g_min_uss
            g_min_uss=min(g_min_uss, self._mem_info.uss)

        # Tracing log
        itm_tracing=ITMS()
        itm_tracing._dur=ts2-self._ts1
        itm_tracing._name=self._name
        itm_tracing._ts1=self._ts1        
        itm_tracing._kwargs=self._kwargs
        g_manage_profile.add_tracing(itm_tracing)
        del itm_tracing

        # Memory log, record function [begin, end]'s USS.
        if g_profiling_uss:
            itm_mem1=ITMS()
            itm_mem1._ts1=self._ts1
            itm_mem1._kwargs=dict(mem_size=self._mem_info.uss)
            g_manage_profile.add_mem(itm_mem1)

            itm_mem2=ITMS()
            itm_mem2._ts1=ts2
            itm_mem2._kwargs=dict(mem_size=mem_info.uss)
            g_manage_profile.add_mem(itm_mem2)
            del itm_mem2

        # print(f'Profiling end: {self._name}')
        # print("==============1")
        # for iii in g_manage_profile._itms:
        #     print(f" ->{iii._name}")
        # print("==============2")