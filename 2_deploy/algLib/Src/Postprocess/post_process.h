#ifndef _POSTPROCESS_H_
#define _POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include "alg_types.h"
#include "rknn_utils.h"

class PostProcess
{
public:
    virtual ~PostProcess() = default;
    virtual int Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *p_od_results) = 0;

protected:
    int Clamp(float val, int min, int max);
    float Sigmoid(float x);
    float Unsigmoid(float y);
    int32_t __clip(float val, float min, float max);
    uint8_t Qnt_f32_to_affine_u8(float f32, int32_t zp, float scale);
    uint8_t Qnt_f32_to_affine_u8(float f32, uint32_t zp, float scale);
    uint8_t Qnt_f32_to_affine_u8(float f32, uint8_t zp, float scale);
    float Deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale);
    float Deqnt_affine_to_f32(uint8_t qnt, uint8_t zp, float scale);
    float Deqnt_affine_u8_to_f32(uint8_t qnt, int32_t zp, float scale);
    float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1);
    int Nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order, int filterId, float threshold);
    int Quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices);
    void Compute_dfl(float* tensor, int dfl_len, float* box);
};
#endif //_POSTPROCESS_H_
