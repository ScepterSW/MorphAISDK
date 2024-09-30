#include "post_process.h"

class PostProcessCustom: public PostProcess
{
public:
    virtual int Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results);
};