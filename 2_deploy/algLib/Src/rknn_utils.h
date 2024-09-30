#ifndef _RKNN_DEMO_UTILS_H
#define _RKNN_DEMO_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <memory.h>
#include <assert.h>

#include "rknn/rknn_api.h"

#define OBJ_CLASS_NUM     80
#define PROP_BOX_SIZE     (5+OBJ_CLASS_NUM)
static const char* LABELS[OBJ_CLASS_NUM]= {
"person",
"bicycle",
"car",
"motorcycle",
"airplane",
"bus",
"train",
"truck",
"boat",
"traffic light",
"fire hydrant",
"stop sign",
"parking meter",
"bench",
"bird",
"cat",
"dog",
"horse",
"sheep",
"cow",
"elephant",
"bear",
"zebra",
"giraffe",
"backpack",
"umbrella",
"handbag",
"tie",
"suitcase",
"frisbee",
"skis",
"snowboard",
"sports ball",
"kite",
"baseball bat",
"baseball glove",
"skateboard",
"surfboard",
"tennis racket",
"bottle",
"wine glass",
"cup",
"fork",
"knife",
"spoon",
"bowl",
"banana",
"apple",
"sandwich",
"orange",
"broccoli",
"carrot",
"hot dog",
"pizza",
"donut",
"cake",
"chair",
"couch",
"potted plant",
"bed",
"dining table",
"toilet",
"tv",
"laptop",
"mouse",
"remote",
"keyboard",
"cell phone",
"microwave",
"oven",
"toaster",
"sink",
"refrigerator",
"book",
"clock",
"vase",
"scissors",
"teddy bear",
"hair drier",
"toothbrush"
};

typedef struct _MODEL_INFO
{
    rknn_context ctx; // rknn context
    bool use_zp;      // whether use zero copy api, default is true

    uint32_t n_input;                     // input number
    rknn_tensor_attr *in_attrs = nullptr; // input tensors` attribute
    rknn_input *inputs = nullptr;         // rknn inputs, used for normal api
    rknn_tensor_mem **in_mems = nullptr;  // inputs` memory, used for zero-copy api

    uint32_t n_output;                     // output number
    rknn_tensor_attr *out_attrs = nullptr; // output tensors` attribute
    rknn_output *outputs = nullptr;        // rknn outputs, used for normal api
    rknn_tensor_mem **out_mems = nullptr;  // outputs` memory, used for zero-copy api

    int img_w;
    int img_h;
} MODEL_INFO;

int rkdemo_model_init(const char *model_path, MODEL_INFO *model_info);
int rkdemo_model_release(MODEL_INFO *model_info);

#endif