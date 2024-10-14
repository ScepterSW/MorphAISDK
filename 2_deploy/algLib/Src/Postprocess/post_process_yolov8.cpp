#include <cmath>
#include <set>
#include "post_process_yolov8.h"

int PostProcessYolov8::Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results)
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
    if (false == model_info->use_zp)
    {
        int result = rknn_outputs_get(model_info->ctx, model_info->n_output, model_info->outputs, NULL);
    }

    // default 3 branch
    int dfl_len = model_info->out_attrs[0].dims[2] / 4;
    int output_per_branch = model_info->n_output / 3;
    for (int i = 0; i < 3; i++)
    {

        void *score_sum = nullptr;
        int32_t score_sum_zp = 0;
        float score_sum_scale = 1.0;
        int box_idx = i*output_per_branch;
        
        if (output_per_branch == 3){
            if (model_info->use_zp)
            {
                score_sum = model_info->out_mems[i*output_per_branch + 2]->logical_addr;
                grid_h = model_info->out_attrs[box_idx].dims[0];
                grid_w = model_info->out_attrs[box_idx].dims[1];
            }
            else
            {
                score_sum = model_info->outputs[i * output_per_branch + 2].buf;
                grid_h = model_info->out_attrs[box_idx].dims[1];
                grid_w = model_info->out_attrs[box_idx].dims[0];
            }
            
            score_sum_zp = model_info->out_attrs[i*output_per_branch + 2].zp;
            score_sum_scale = model_info->out_attrs[i*output_per_branch + 2].scale;
        }

        int score_idx = i*output_per_branch + 1;

        stride = model_in_h / grid_h;

        if (model_info->use_zp)
        {
            validCount += process_u8((uint8_t *)model_info->out_mems[box_idx]->logical_addr, model_info->out_attrs[box_idx].zp, model_info->out_attrs[box_idx].scale,
                                     (uint8_t *)model_info->out_mems[score_idx]->logical_addr, model_info->out_attrs[score_idx].zp, model_info->out_attrs[score_idx].scale,
                                     (uint8_t *)score_sum, score_sum_zp, score_sum_scale,
                                     grid_h, grid_w, stride, dfl_len,
                                     filterBoxes, objProbs, classId, conf_threshold);
        }
        else
        {
            validCount += process_u8((uint8_t *)model_info->outputs[box_idx].buf, model_info->out_attrs[box_idx].zp, model_info->out_attrs[box_idx].scale,
                                     (uint8_t *)model_info->outputs[score_idx].buf, model_info->out_attrs[score_idx].zp, model_info->out_attrs[score_idx].scale,
                                     (uint8_t *)score_sum, score_sum_zp, score_sum_scale,
                                     grid_h, grid_w, stride, dfl_len,
                                     filterBoxes, objProbs, classId, conf_threshold);
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
        if (indexArray[i] == -1 || last_count >= OBJ_NUMB_MAX_SIZE)
        {
            continue;
        }
        int n = indexArray[i];

        // float x1 = filterBoxes[n * 4 + 0] - letter_box->x_pad;
        // float y1 = filterBoxes[n * 4 + 1] - letter_box->y_pad;
        
        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[i];

        od_results->results[last_count].box.left = (int)(Clamp(x1, 0, model_in_w));
        od_results->results[last_count].box.top = (int)(Clamp(y1, 0, model_in_h));
        od_results->results[last_count].box.right = (int)(Clamp(x2, 0, model_in_w));
        od_results->results[last_count].box.bottom = (int)(Clamp(y2, 0, model_in_h));
        od_results->results[last_count].prop = obj_conf;
        const char *label = LABELS[id];
        strncpy(od_results->results[last_count].name, label, OBJ_NAME_MAX_SIZE);
        last_count++;
    }
    od_results->count = last_count;
    return 0;
}

int PostProcessYolov8::process_u8(uint8_t *box_tensor, int32_t box_zp, float box_scale,
                      uint8_t *score_tensor, int32_t score_zp, float score_scale,
                      uint8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                      int grid_h, int grid_w, int stride, int dfl_len,
                      std::vector<float> &boxes,
                      std::vector<float> &objProbs,
                      std::vector<int> &classId,
                      float threshold)
{
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    uint8_t score_thres_u8 = Qnt_f32_to_affine_u8(threshold, score_zp, score_scale);
    uint8_t score_sum_thres_u8 = Qnt_f32_to_affine_u8(threshold, score_sum_zp, score_sum_scale);

    for (int i = 0; i < grid_h; i++)
    {
        for (int j = 0; j < grid_w; j++)
        {
            int offset = i * grid_w + j;
            int max_class_id = -1;

            // Use score sum to quickly filter
            if (score_sum_tensor != nullptr)
            {
                if (score_sum_tensor[offset] < score_sum_thres_u8)
                {
                    continue;
                }
            }

            uint8_t max_score = -score_zp;
            for (int c = 0; c < OBJ_CLASS_NUM; c++)
            {
                if ((score_tensor[offset] > score_thres_u8) && (score_tensor[offset] > max_score))
                {
                    max_score = score_tensor[offset];
                    max_class_id = c;
                }
                offset += grid_len;
            }

            // compute box
            if (max_score > score_thres_u8)
            {
                offset = i * grid_w + j;
                float box[4];
                if (dfl_len > 1)
                {
                    /// dfl
                    float before_dfl[dfl_len * 4];
                    for (int k = 0; k < dfl_len * 4; k++)
                    {
                        before_dfl[k] = Deqnt_affine_u8_to_f32(box_tensor[offset], box_zp, box_scale);
                        offset += grid_len;
                    }
                    Compute_dfl(before_dfl, dfl_len, box);
                }
                else
                {
                    for (int k = 0; k < 4; k++)
                    {
                        box[k] = Deqnt_affine_u8_to_f32(box_tensor[offset], box_zp, box_scale);
                        offset += grid_len;
                    }
                }

                float x1, y1, x2, y2, w, h;
                x1 = (-box[0] + j + 0.5) * stride;
                y1 = (-box[1] + i + 0.5) * stride;
                x2 = (box[2] + j + 0.5) * stride;
                y2 = (box[3] + i + 0.5) * stride;
                w = x2 - x1;
                h = y2 - y1;
                boxes.push_back(x1);
                boxes.push_back(y1);
                boxes.push_back(w);
                boxes.push_back(h);

                objProbs.push_back(Deqnt_affine_u8_to_f32(max_score, score_zp, score_scale));
                classId.push_back(max_class_id);
                validCount++;
            }
        }
    }
    return validCount;
}