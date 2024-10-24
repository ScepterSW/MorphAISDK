#include <inttypes.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "alg.h"
#include "alg_types.h"
#include "json.h"

int Alg::Initialize(const string& configStr)
{
    char pConfig[256]="./";
    if (false == configStr.empty())
    {
        sprintf(pConfig, "%s", configStr.c_str());
    }
 
    ALGO_INIT_PARAM_T initParam = { pConfig, 
        std::bind(&Alg::CallbackFunc, *this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) };
 
    return algo_initialize(&initParam);
}

int Alg::Release()
{
    return algo_release();
}

int Alg::Ctrl_start(void)
{
    return algo_ctrl_start();
}
 
int Alg::Ctrl_stop(void)
{
    return algo_ctrl_stop();
}
 
int Alg::Push_frame(CameraJointFrame& jointFrame)
{
    return algo_push_frame(&jointFrame);
}
 
int Alg::Set_param(uint32_t param_id, const char *in_param_p, uint16_t param_len)
{
    return algo_set_param(param_id, in_param_p, param_len);
}
 
int Alg::Get_param(uint32_t param_id, const char **out_param_p, uint16_t *param_len_p)
{
    return algo_get_param(param_id, out_param_p, param_len_p);
}

void Alg::CallbackFunc(uint64_t timestamp, const char* proc_result_p, uint32_t result_len)
{
    printf("timestamp:%" PRIu64 ", result_len:%d\n %s \n", timestamp, result_len, proc_result_p);
}