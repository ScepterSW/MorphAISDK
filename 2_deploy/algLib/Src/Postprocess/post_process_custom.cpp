#include "post_process_custom.h"
#include "log.h"

int PostProcessCustom::Process(MODEL_INFO *model_info, float conf_threshold, float nms_threshold, detect_result_group_t *od_results)
{
    Log("A custom implementation is required.");

    return 0;
}