#pragma once
#include<stdint.h>
#include<string>
#include "alg_api.h"

using std::string;
 
class Alg
{
public:
    int Initialize(const string& configStr);
    int Release();
    int Ctrl_start(void);
    int Ctrl_stop(void);
    int Push_frame(CameraJointFrame& jointFrame);
    int Set_param(uint32_t param_id, const char *in_param_p, uint16_t param_len);
    int Get_param(uint32_t param_id, const char **out_param_p, uint16_t *param_len_p);
    void CallbackFunc(uint64_t timestamp, const char* proc_result_p, uint32_t result_len);
};