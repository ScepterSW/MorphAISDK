import sys
import os
from ruamel import yaml
from rknn.api import RKNN

curdir_ = os.path.dirname(os.path.abspath(__file__)) + '/'

# the path of ONNX model file
MODEL_FILE_PATH = curdir_ + './model/yolov8n.onnx'

# a input data set for rectifying quantization parameters.
DATASET_CONFIG_PATH = curdir_ + '../../datasets/yolo/yolo.txt'

# the path of generated RKNN model file
OUT_PATH = curdir_ + './out'

# the ip of the connected device
DEVICE_IP = '192.168.1.101'


class Config:
    def __init__(self) -> None:
        self.target_platform = 'rv1126'
        self.model_type = 'yolov8'

        # Mean and std deviation are used for the normalization of input data, with the formula: normalize = (value - mean) / std.
        # The values of mean and std must be consistent with the settings used during the model training.
        self.mean_values = [[0, 0, 0]]
        self.std_values = [[255, 255, 255]]

        self.model_file_path = MODEL_FILE_PATH
        self.dataset = DATASET_CONFIG_PATH
        self.quantization_type = 'u8'
        self.precompile = True
        self.out_path = OUT_PATH
        self.device_id = DEVICE_IP + ':5555'

    def __str__(self):
        print('*'*10, 'config', '*'*10)
        print("model_type = ", self.model_type)
        print("model_file_path = ", self.model_file_path)
        print("dataset = ", self.dataset)
        print("device_id = ", self.device_id)
        print("out_path = ", self.out_path)
        return '*'*25

if __name__ == '__main__':
    config = Config()
    print(config)

    # Create RKNN object
    rknn = RKNN(verbose=False)

    # Pre-process config
    print('--> Config model')
    rknn.config(mean_values=config.mean_values, std_values=config.std_values,
                target_platform=config.target_platform, output_optimize = 1)
    print('done')

    # Load model
    print('--> Loading model')
    ret = rknn.load_onnx(model=config.model_file_path)

    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=config.quantization_type,
                     dataset=config.dataset)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')

    # Export pre-complie rknn model
    if (config.precompile):
        output_path = os.path.join(config.out_path, os.path.basename(
            config.model_file_path).split('.')[0]+'_precompile.rknn')
        print('--> Exporting pre-complie model:', output_path)
        rknn.init_runtime(target=config.target_platform,
                          device_id=config.device_id, rknn2precompile=config.precompile)
        rknn.export_rknn_precompile_model(output_path)
        print('done')
    else:
        # Export rknn model
        output_path = os.path.join(config.out_path, os.path.basename(
            config.model_file_path).split('.')[0]+'.rknn')
        print('--> Exporting rknn model:', output_path)
        ret = rknn.export_rknn(output_path)
        if ret != 0:
            print('Export rknn model failed!')
            exit(ret)
        print('done')

    # Release
    rknn.release()
