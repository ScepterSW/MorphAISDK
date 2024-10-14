#include "post_process.h"

class PostProcessYolov8: public PostProcess
{
private:
    int process_u8(uint8_t *box_tensor, int32_t box_zp, float box_scale,
                      uint8_t *score_tensor, int32_t score_zp, float score_scale,
                      uint8_t *score_sum_tensor, int32_t score_sum_zp, float score_sum_scale,
                      int grid_h, int grid_w, int stride, int dfl_len,
                      std::vector<float> &boxes,
                      std::vector<float> &objProbs,
                      std::vector<int> &classId,
                      float threshold);
public:
    virtual int Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results);
};