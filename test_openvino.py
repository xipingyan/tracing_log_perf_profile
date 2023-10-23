import time
import sys
from my_py_profile import MyProfile

import numpy as np
import openvino.runtime as ov
import cv2

def main():
    model_fn="../openvino/src/bindings/python/tests/utils/utils/test_model_fp32.xml"
    core = ov.Core()
    model = core.read_model(model_fn)
    compiled_model = core.compile_model(model, "CPU") # plugin_config
    infer_request = compiled_model.create_infer_request()

    # image = cv2.imread("")
    input_data_np=np.zeros([1, 3, 32, 32], dtype=np.float32)
    # Convert image to openvino input tensor
    input_data = ov.Tensor(input_data_np, ov.Shape([1, 3, 32, 32]))
    for _ in range(200):
        frame_results = infer_request.infer(input_data)
    print(f"frame_results={type(frame_results)}")
if __name__ == "__main__":
    main()
