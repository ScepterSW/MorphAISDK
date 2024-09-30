#include <set>
#include "post_process_yolov5.h"
#include "log.h"

const static int anchor[][6] = {{10, 13, 16, 30, 33, 23},
                                {30, 61, 62, 45, 59, 119},
                                {116, 90, 156, 198, 373, 326}};


int PostProcessYolov5::Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results)
{
    std::vector<float> filterBoxes;
    std::vector<float> objProbs;
    std::vector<int> classId;
    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;
    int model_in_w = model_info->img_w;
    int model_in_h = model_info->img_h;

    memset(od_results, 0, sizeof(detect_result_group_t));
    if (true == model_info->use_zp)
    {
        for (uint32_t i = 0; i < model_info->n_output; i++)
        {
            grid_h = model_info->out_attrs[i].dims[0];
            grid_w = model_info->out_attrs[i].dims[1];
            stride = model_in_h / grid_h;
            validCount += process_u8((uint8_t *)model_info->out_mems[i]->logical_addr, (int *)anchor[i], grid_h, grid_w, model_in_h, model_in_w, stride, filterBoxes, objProbs,
                                        classId, conf_threshold, model_info->out_attrs[i].zp, model_info->out_attrs[i].scale);
        }
    }
    else
    {
        rknn_outputs_get(model_info->ctx, model_info->n_output, model_info->outputs, NULL);
        for (uint32_t i = 0; i < model_info->n_output; i++)
        {
            grid_h = model_info->out_attrs[i].dims[1];
            grid_w = model_info->out_attrs[i].dims[0];
            stride = model_in_h / grid_h;
            validCount += process_u8((uint8_t *)model_info->outputs[i].buf, (int *)anchor[i], grid_h, grid_w, model_in_h, model_in_w, stride, filterBoxes, objProbs,
                                        classId, conf_threshold, model_info->out_attrs[i].zp, model_info->out_attrs[i].scale);
        }

    }
    
    
    // no object detect
    if (validCount <= 0)
    {
        return 0;
    }
    std::vector<int> indexArray;
    for (int i = 0; i < validCount; ++i)
    {
        indexArray.push_back(i);
    }
    Quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));

    for (auto c : class_set)
    {
        Nms(validCount, filterBoxes, classId, indexArray, c, nms_threshold);
    }

    int last_count = 0;
    od_results->count = 0;

    /* box valid detect target */
    for (int i = 0; i < validCount; ++i)
    {
        int n = indexArray[i];
        if (indexArray[i] == -1 
        || last_count >= OBJ_NUMB_MAX_SIZE) 
        {
            continue;
        }

        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        float obj_conf = objProbs[i];

        od_results->results[last_count].box.left = (int)(Clamp(x1, 0, model_in_w));
        od_results->results[last_count].box.top = (int)(Clamp(y1, 0, model_in_h));
        od_results->results[last_count].box.right = (int)(Clamp(x2, 0, model_in_w));
        od_results->results[last_count].box.bottom = (int)(Clamp(y2, 0, model_in_h));
        od_results->results[last_count].prop = obj_conf;
        int id = classId[n];
        const char *label = LABELS[id];
        strncpy(od_results->results[last_count].name, label, OBJ_NAME_MAX_SIZE);

        // Log("result %2d: %.6f (%4d, %4d, %4d, %4d), %s.", i, od_results->results[last_count].prop, od_results->results[last_count].box.left, od_results->results[last_count].box.top,
        //         od_results->results[last_count].box.right, od_results->results[last_count].box.bottom, label);
        last_count++;
    }
    od_results->count = last_count;

    return 0;
}

int PostProcessYolov5::process_u8(uint8_t *input, int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                   std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId,
                   float threshold, uint32_t zp, float scale)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    uint8_t thres_u8 = Qnt_f32_to_affine_u8(threshold, zp, scale);
    for (int a = 0; a < 3; a++)
    {
        for (int i = 0; i < grid_h; i++)
        {
            for (int j = 0; j < grid_w; j++)
            {
                uint8_t box_confidence = input[(PROP_BOX_SIZE * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= thres_u8)
                {
                    int offset = (PROP_BOX_SIZE * a) * grid_len + i * grid_w + j;
                    uint8_t *in_ptr = input + offset;
                    float box_x = (Deqnt_affine_u8_to_f32(*in_ptr, zp, scale)) * 2.0 - 0.5;
                    float box_y = (Deqnt_affine_u8_to_f32(in_ptr[grid_len], zp, scale)) * 2.0 - 0.5;
                    float box_w = (Deqnt_affine_u8_to_f32(in_ptr[2 * grid_len], zp, scale)) * 2.0;
                    float box_h = (Deqnt_affine_u8_to_f32(in_ptr[3 * grid_len], zp, scale)) * 2.0;
                    box_x = (box_x + j) * (float)stride;
                    box_y = (box_y + i) * (float)stride;
                    box_w = box_w * box_w * (float)anchor[a * 2];
                    box_h = box_h * box_h * (float)anchor[a * 2 + 1];
                    box_x -= (box_w / 2.0);
                    box_y -= (box_h / 2.0);

                    uint8_t maxClassProbs = in_ptr[5 * grid_len];
                    int maxClassId = 0;
                    for (int k = 1; k < OBJ_CLASS_NUM; ++k)
                    {
                        uint8_t prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs)
                        {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    if (maxClassProbs > thres_u8)
                    {
                        objProbs.push_back((Deqnt_affine_u8_to_f32(maxClassProbs, zp, scale)) * (Deqnt_affine_u8_to_f32(box_confidence, zp, scale)));
                        classId.push_back(maxClassId);
                        validCount++;
                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                    }
                }
            }
        }
    }
    return validCount;
}