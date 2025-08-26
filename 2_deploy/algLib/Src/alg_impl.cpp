#include <fstream>
#include <queue>
#include <cmath>
#include <inttypes.h>
#include "log.h"
#include "alg_impl.h"
#include "serialization.h"


using namespace std;

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

ALG_Impl::ALG_Impl() :
    StoppableThread("ALG"),
    m_imgprocess(),
    m_inference(m_imgprocess),
    m_p_callback_func(nullptr),
    m_result_buf(new uint8_t[TOF_IMG_W * TOF_IMG_H]{0}),
    m_Info(),
    m_paramTestASCII("ASCII"),
    m_paramTestHEX(new uint8_t[5]{0x01, 0x02, 0x03, 0x04, 0x05}),
    m_paramTestHEXBufCapacity(5),
    m_paramTestHEXBufUsedSize(5)
{}

ALG_Impl::~ALG_Impl()
{
    Stop();
}

int ALG_Impl::Init(const ALGO_INIT_PARAM_T *p_init_param)
{
    const string alg_root_str = string(p_init_param->p_alg_root_dir);
    Log("the root dir of alg:%s pCallFunc:%p.", alg_root_str.c_str(), p_init_param->process_cb_func);
    m_p_callback_func = p_init_param->process_cb_func;

    const string expanded_name = ".rknn";
    string model_file_path = alg_root_str;
    if ((alg_root_str.length() < expanded_name.length())
        || (alg_root_str.substr(alg_root_str.length() - expanded_name.length()) != expanded_name))
    {
        const string DEFAULT_MODEL_RELATIVE_PATH = "./model/test.rknn";
        if(false == model_file_path.empty() && model_file_path[model_file_path.size()-1] != '/')
        {
            model_file_path += '/';
        }
        model_file_path += DEFAULT_MODEL_RELATIVE_PATH;
    }
    
    int ret = m_inference.Init(model_file_path.c_str());
    if(ret < 0)
    {
        Log("Read config dir :%s pCallFunc:%p.", alg_root_str.c_str(), p_init_param->process_cb_func);
        ret = ALGO_RET_INIT_MODEL_NG;
    }
    const MODEL_INFO modelInfo = m_inference.GetModelInfo();
    ret = m_imgprocess.SetModelInputImageSize(modelInfo.img_w, modelInfo.img_h);

    return ret;
}

bool ALG_Impl::UpdateData(const CAM_JOINT_FRAME_T *p_joint_frame)
{
    bool ret = true;
    int result = m_imgprocess.UpdateData(p_joint_frame);

    if(0 != result)
    {
        SetInfo(result);
        ret = false;
    }
    
	return ret;
}

int ALG_Impl::SetSN(const string& sn)
{
    Log("sn:%s.\n", sn.c_str());
    m_SN = sn;

    return 0;
}

int ALG_Impl::SetConf_Thresh(float threshold)
{
    return m_inference.SetConf_Thresh(threshold);
}

int ALG_Impl::SetIOU_Thresh(float threshold)
{
    return m_inference.SetIOU_Thresh(threshold);
}

int ALG_Impl::SetSaveResultImgEnable(bool enable)
{
    return m_inference.SetSaveResultImgEnable(enable);
}

float ALG_Impl::GetConf_Thresh()
{
    return m_inference.GetConf_Thresh();
}

float ALG_Impl::GetIOU_Thresh()
{
    return m_inference.GetIOU_Thresh();
}

int ALG_Impl::SetParamTestASCII(const char *p_in_param, uint16_t param_len)
{
    m_paramTestASCII = string(p_in_param, param_len);

    Log("param_len:%d, p_in_param:%s", param_len, m_paramTestASCII.c_str());

    return 0;
}

int ALG_Impl::GetParamTestASCII(const char **p_out_param, uint16_t *p_param_len)
{
    *p_param_len = m_paramTestASCII.size() + 1;
    *p_out_param = m_paramTestASCII.c_str();
    Log("param_len:%d, p_out_param:%s", *p_param_len, *p_out_param);
    return 0;
}

int ALG_Impl::SetParamTestHEX(const char *p_in_param, uint16_t param_len)
{
    if(param_len > m_paramTestHEXBufCapacity)
    {
        m_paramTestHEXBufCapacity = param_len;
        m_paramTestHEX.reset(new uint8_t[m_paramTestHEXBufCapacity]);
    }
    m_paramTestHEXBufUsedSize = param_len;
    memcpy(m_paramTestHEX.get(), p_in_param, param_len);

    char buf[4] = {};
    std::string hexStr = "";
    for (uint16_t i = 0;i < param_len; i++)
    {
        sprintf(buf, "%02x ", p_in_param[i]);
        hexStr += buf;
    }
    Log("param_len:%d, p_in_param:%s", param_len, hexStr.c_str());

    return 0;
}

int ALG_Impl::GetParamTestHEX(const char **p_out_param, uint16_t *p_param_len)
{
    *p_param_len = m_paramTestHEXBufUsedSize;
    *p_out_param = reinterpret_cast<const char*>(m_paramTestHEX.get());

    char buf[4] = {};
    std::string hexStr = "";
    for (uint16_t i = 0;i < (*p_param_len); i++)
    {
        sprintf(buf, "%02x ", (*p_out_param)[i]);
        hexStr += buf;
    }
    Log("param_len:%d, p_out_param:%s", *p_param_len, hexStr.c_str());
    return 0;
}

 void ALG_Impl::SetInfo(int errorNo)
 {
    string info;
    switch (errorNo)
    {
    case ALGO_RET_CHECK_FRAME_COUNT_NG:
        info = "The count of input frames is incorrect.";
        break;
    case ALGO_RET_CHECK_FRAME_INDEX_NG:
        info = "The input frame has an incorrect index.";
        break;
    case ALGO_RET_CHECK_FRAME_DEPTH_PARAMS_NG:
        info = "The parameters of the depth frame are incorrect.";
        break;
    case ALGO_RET_CHECK_FRAME_IR_PARAMS_NG:
        info = "The parameters of the ir frame are incorrect.";
        break;
    case ALGO_RET_CHECK_FRAME_RGB_PARAMS_NG:
        info = "The parameters of the rgb frame are incorrect.";
        break;
    case ALGO_RET_CHECK_FRAME_SUPPORT_NG:
        info = "The type of frame is not supported.";
        break;
    case ALGO_RET_GET_FRAME_TIMEOUT:
        info = "Image reception timeout.";
        break;
    default:
        break;
    }

    if(false == info.empty())
    {
        SetInfo(info);
    }
 }

 void ALG_Impl::SetInfo(const string& info, bool isStopInfo)
 {
    lock_guard<mutex> lk(m_mutex);
    m_Info = string("Alg_info: ") + info;

    if(true == IsRunning() || true == isStopInfo)
    {
        CallBackFunc(LogCustom::GetTimeStampMS(), m_Info);
    }
 }

int ALG_Impl::Start()
{   
    int ret = 0;
    if(false == IsRunning())
    {
        ret = StoppableThread::Start();
        SetInfo("start");
    }
    return ret;
}

 void ALG_Impl::Process()
 {
    Log("CorePin:%d", CorePin(3));
    StoppableThread::Process();
 }

void ALG_Impl::Run()
{    
    int result = m_imgprocess.ProcessData();
    if (ALGO_RET_OK == result)
    {
        int64_t t0 = LogCustom::GetTimeStampMS();
        CameraSingleFrame frame;
        m_imgprocess.GetInferenceImg(frame);
        int64_t t1 = LogCustom::GetTimeStampMS();

        detect_result_group_t detect_result_group = {0};
        result = m_inference.Detect(frame, detect_result_group);
        if(0 != result)
        {
            SetInfo(Log("Detect:%d is failed.", result));
        }
        int64_t t2 = LogCustom::GetTimeStampMS();
        m_imgprocess.GetDepthImg(frame);
        cv::Mat depth = cv::Mat(frame.height, frame.width, CV_16UC1, frame.p_data);

        for (int i = 0; i < detect_result_group.count; i++)
        {
            detect_result_t& det_result = detect_result_group.results[i];
            cv::Rect detectBox = cv::Rect(det_result.box.left, det_result.box.top, (det_result.box.right - det_result.box.left), (det_result.box.bottom - det_result.box.top));
            m_imgprocess.UpdatePointCloud(depth, detectBox, det_result.centerPosInWorld);
        }
        int64_t t3 = LogCustom::GetTimeStampMS();
        CallBackFunc(frame.timestamp, detect_result_group);
        int64_t t4 = LogCustom::GetTimeStampMS();
        Log("cost:%lld %lld %lld %lld %lld", (t4 - t0), (t1 - t0), (t2 - t1), (t3 - t2), (t4 - t3));

        static int fps = 0;
        static int64_t start = LogCustom::GetTimeStampMS();
        int64_t current = LogCustom::GetTimeStampMS();
        fps++;
        if(current - start > 1000)
        {
            Log("Run fps:%.2f", fps * 1000.f / (current - start));
            fps = 0;
            start = current;
        }
    }
    else
    {
        SetInfo(result);
    }
}

void ALG_Impl::Stop()
{
    if(false == IsStoped())
    {
        StoppableThread::Stop();
        Join();
        SetInfo("stop", true);
    }
}

void ALG_Impl::CallBackFunc(uint64_t timestamp, const detect_result_group_t& detect__result_group)
{
    if (nullptr != m_p_callback_func)
    {
        uint8_t *pBuf = m_result_buf.get();
        char *jsonData = (char *)(pBuf);
        Serialization serialization;
        uint32_t jsonLen = 0;
        serialization.GetResultJson(jsonData, jsonLen, detect__result_group);

        {
            lock_guard<mutex> lk(m_mutex);
            m_p_callback_func(timestamp, (const char *)pBuf, jsonLen);
        }

        Log("timestamp:%" PRIu64 ", jsonLen:%d, pBuf:%s", timestamp, jsonLen, pBuf);
    }
}

void ALG_Impl::CallBackFunc(uint64_t timestamp, string& info)
{
    if (nullptr != m_p_callback_func && false == info.empty())
    {
        m_p_callback_func(timestamp, info.c_str(), info.size() + 1);
        Log("timestamp:%" PRIu64 ", len:%d, info:%s", timestamp, (info.size() + 1), info.c_str());
    }
}