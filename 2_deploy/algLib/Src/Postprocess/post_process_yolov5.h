#include "post_process.h"

class PostProcessYolov5: public PostProcess
{
private:
    int process_u8(uint8_t *input, int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                   std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId,
                   float threshold, uint32_t zp, float scale);
public:
    virtual int Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results);
};