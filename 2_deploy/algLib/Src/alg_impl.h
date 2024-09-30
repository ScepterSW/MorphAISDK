#ifndef _ALG_IMPL_H_
#define _ALG_IMPL_H_

#include <functional>
#include <mutex>
#include <memory>
#include "alg_types.h"
#include "alg_api.h"
#include "image_process.h"
#include "rknn_inference.h"
#include "stoppable_thread.h"

using std::mutex;
using std::unique_ptr;

class ALG_Impl:public StoppableThread
{
private:
    virtual void Process();
    void CallBackFunc(uint64_t timestamp, const detect_result_group_t& detect_result_group);
    void CallBackFunc(uint64_t timestamp, string& info);
public:
	ALG_Impl();
    ~ALG_Impl();
	virtual void Run();
	virtual int Start();
    virtual void Stop();

	int Init(const ALGO_INIT_PARAM_T *p_init_param);
	bool UpdateData(const CAM_JOINT_FRAME_T *p_joint_frame);
    int SetSN(const string& sn);
    const string& GetSN() { return m_SN;}
    int SetConf_Thresh(float threshold);
	float GetConf_Thresh();
    int SetIOU_Thresh(float threshold);
	float GetIOU_Thresh();
    int SetParamTestASCII(const char *p_in_param, uint16_t param_len);
    int GetParamTestASCII(const char **p_out_param, uint16_t *p_param_len);
    int SetParamTestHEX(const char *p_in_param, uint16_t param_len);
    int GetParamTestHEX(const char **p_out_param, uint16_t *p_param_len);
    int SetSaveResultImgEnable(bool enable);
    void SetInfo(int errorNo);
    void SetInfo(const string& info, bool isStopInfo = false);

private:
    string m_SN;
    ImgProcess m_imgprocess;
    RKNNInference m_inference;
    AlgorithmInitParam::ALGO_PROCESS_CB_FUNC_T m_p_callback_func;

    mutex m_mutex;
    unique_ptr<uint8_t[]> m_result_buf;
    string m_Info;
    string m_paramTestASCII;
    unique_ptr<uint8_t[]> m_paramTestHEX;
    int m_paramTestHEXBufCapacity;
    int m_paramTestHEXBufUsedSize;
};

#endif // _ALG_IMPL_H_