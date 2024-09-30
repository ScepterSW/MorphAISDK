import os
import cv2
import sys

# add path
realpath = os.path.abspath(__file__)
sys.path.append(os.path.join(os.path.dirname(realpath), '..'))

from py_utils.coco_utils import COCO_test_helper
from py_utils.rknn_executor import RKNN_model_container
from py_utils.yolov5_postprocess import *
import numpy as np

# the path of rknn model file
MODEL_FILE_PATH = './out/vzense_box_precompile.rknn'

# the path of the test image
TEST_IMAGE_PATH = './model/Color.jpg'

# the path of saved the detection result
OUT_PATH = './out'

# the ip of the connected device
DEVICE_IP = '192.168.1.101'

ANCHORS = [[[10.0, 13.0], [16.0, 30.0], [33.0, 23.0]], 
           [[30.0, 61.0], [62.0, 45.0], [59.0, 119.0]], 
           [[116.0, 90.0], [156.0, 198.0], [373.0, 326.0]]]

CLASSES = ["vzense box"]

class Config:
    def __init__(self) -> None:
        self.target_platform = 'rv1126'
        self.model_type = 'yolov5'

        self.model_file_path = MODEL_FILE_PATH
        self.test_img_path = TEST_IMAGE_PATH
        self.model_input_img_size = (640, 640)
        self.anchors = ANCHORS
        self.obj_thresh = 0.25
        self.nms_thresh = 0.45
        self.out_path = OUT_PATH
        self.device_id = DEVICE_IP + ':5555'

    def __str__(self):
        print('*'*10, 'config', '*'*10)
        print("model_type = ", self.model_type)
        print("model_file_path = ", self.model_file_path)
        print("test_img_path = ", self.test_img_path)
        print("device_id = ", self.device_id)
        print("out_path = ", self.out_path)
        return '*'*25
    

def draw(image, boxes, scores, classes):
    for box, score, cl in zip(boxes, scores, classes):
        top, left, right, bottom = [int(_b) for _b in box]
        print("%s @ (%d %d %d %d) %.3f" %
              (CLASSES[cl], top, left, right, bottom, score))
        cv2.rectangle(image, (top, left), (right, bottom), (255, 0, 0), 2)
        cv2.putText(image, '{0} {1:.2f}'.format(CLASSES[cl], score),
                    (top, left - 6), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

if __name__ == '__main__':
    config = Config()
    print(config)

    # init model
    model = RKNN_model_container(config.model_file_path, config.target_platform, config.device_id)

    # read image
    img_src = cv2.imread(config.test_img_path)
    if img_src is None:
        print("img_src is None")
        exit(-1)

    # image preprocess
    # Due to rga init with (0,0,0), we using pad_color (0,0,0) instead of (114, 114, 114)
    pad_color = (0, 0, 0)
    print(config.test_img_path, img_src.shape)
    co_helper = COCO_test_helper(enable_letter_box=True)
    img = co_helper.letter_box(im=img_src.copy(), new_shape=(
        config.model_input_img_size[1], config.model_input_img_size[0]), pad_color=pad_color)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # detect
    outputs = model.run(img)
    
    # postprocess
    boxes, classes, scores = post_process(outputs, config.anchors, img.shape, config.obj_thresh, config.nms_thresh)

    # save test result
    img_p = img_src.copy()
    if boxes is not None:
        draw(img_p, co_helper.get_real_box(boxes), scores, classes)

    if not os.path.exists(config.out_path):
        os.mkdir(config.out_path)
    result_path = os.path.join(config.out_path, config.test_img_path.split(os.path.sep)[-1])
    cv2.imwrite(result_path, img_p)
    print('Detection result save to {}'.format(result_path))

    
