#include <memory>
#include "alg_api.h"
#include "alg_impl.h"
#include "log.h"
#include "serialization.h"

static const string VERSION = "24.09.8";
static unique_ptr<ALG_Impl> gALG_ImplPtr = nullptr;

#define CHECKINIT if(nullptr == gALG_ImplPtr)                                                                               \
{                                                                                                                           \
    Log("The algorithm is not initialized, please call the algo initialize function first.");   \
    return ret = ALGO_RET_NOT_INIT;                                                                                         \
}

ALGO_RET_E algo_initialize(const ALGO_INIT_PARAM_T *p_init_param)
{
    ALGO_RET_E ret = ALGO_RET_OK;

    LogCustom::Initance(p_init_param->p_alg_root_dir, "ALG", "");
    Log("===============================================================================");
    Log("Ver:%s", VERSION.c_str());

    if (nullptr == gALG_ImplPtr)
    {
        gALG_ImplPtr = make_unique<ALG_Impl>();
    }

    if (nullptr !=  p_init_param && nullptr != p_init_param->p_alg_root_dir)
    {
        ret = (ALGO_RET_E)gALG_ImplPtr->Init(p_init_param);
        if(ALGO_RET_OK != ret)
        {
            gALG_ImplPtr.reset();
            Log("ret:%d", ret);
        }
    }
    else
    {
        if (nullptr == p_init_param)
        {
            Log("p_init_param is NULL.");
            ret = ALGO_RET_NG;
        }
        else
        {
            Log("p_init_param->p_alg_root_dir is NULL.");
            ret = ALGO_RET_NG;
        }
    }
    
    return ret;
}

ALGO_RET_E algo_ctrl_start(void)
{
    ALGO_RET_E ret = ALGO_RET_OK;

    CHECKINIT;

    gALG_ImplPtr->Start();

    return ret;
}

ALGO_RET_E algo_ctrl_stop(void)
{
    ALGO_RET_E ret = ALGO_RET_OK;
    CHECKINIT;

    gALG_ImplPtr->Stop();
    return ret;
}

ALGO_RET_E algo_push_frame(const CAM_JOINT_FRAME_T *p_joint_frame)
{
    ALGO_RET_E ret = ALGO_RET_OK;
    CHECKINIT;
    if (nullptr == p_joint_frame)
    {
        Log("p_joint_frame is NULL.");
        ret = ALGO_RET_CHECK_PARAMS_NG;
        return ret;
    }
   
    ret = (true == gALG_ImplPtr->UpdateData(p_joint_frame))? ALGO_RET_OK : ALGO_RET_CHECK_FRAME_NG;

    return ret;
}

ALGO_RET_E algo_set_param(uint32_t param_id, const char *p_in_param, uint16_t param_len)
{
    string in_param_str(p_in_param, param_len);
    Log("id:%d len:%d, p_in_param:%s.", param_id, param_len, in_param_str.c_str());

    ALGO_RET_E ret = ALGO_RET_OK;
    CHECKINIT;

    if (nullptr == p_in_param || 0 == param_len)
    {
        Log("p_in_param(%p) or param_len(%d) is invalid.", p_in_param, param_len);
        ret = ALGO_RET_CHECK_PARAMS_NG;
        return ret;
    }

    switch (ParamID(param_id))
    {
    case PARAM_SERIALNUMBER:
    {
        string json(p_in_param, param_len);
        if(strlen(json.c_str()) != json.length() && (strlen(json.c_str()) + 1) != json.length())
        {
            Log("the length of p_in_param(%s) is not equal to param_len(%d != %d).", json.c_str(), strlen(json.c_str()), param_len);
            ret = ALGO_RET_CHECK_PARAMS_NG;
            return ret;
        }
        Serialization serialization;
        string SN;
        if (0 == serialization.GetSerialNumber(json, SN))
        {
            ret = (ALGO_RET_E)gALG_ImplPtr->SetSN(SN);
        }
        else
        {
            Log("p_in_param:%s, an error occurred while parsing json content.",json.c_str());
            ret = ALGO_RET_JSON_PARSE_NG;
        }
    }
    break;
    case PARAM_CONF_THRESH:
    {
        string json(p_in_param, param_len);
        if(strlen(json.c_str()) != json.length() && (strlen(json.c_str()) + 1) != json.length())
        {
            Log("the length of p_in_param(%s) is not equal to param_len(%d != %d).", json.c_str(), strlen(json.c_str()), param_len);
            ret = ALGO_RET_CHECK_PARAMS_NG;
            return ret;
        }
        float thresh = 0.f;
        Serialization serialization;
        if (0 == serialization.GetConf_Thresh(json, thresh))
        {
            ret = (ALGO_RET_E)gALG_ImplPtr->SetConf_Thresh(thresh);
        }
        else
        {
            Log("p_in_param:%s, an error occurred while parsing json content.",json.c_str());
            ret = ALGO_RET_JSON_PARSE_NG;
        }
    }
    break;
    case PARAM_IOU_THRESH:
    {
        string json(p_in_param, param_len);
        if(strlen(json.c_str()) != json.length() && (strlen(json.c_str()) + 1) != json.length())
        {
            Log("the length of p_in_param(%s) is not equal to param_len(%d != %d).", json.c_str(), strlen(json.c_str()), param_len);
            ret = ALGO_RET_CHECK_PARAMS_NG;
            return ret;
        }
        float thresh = 0.f;
        Serialization serialization;
        if (0 == serialization.GetIOU_Thresh(json, thresh))
        {
            ret = (ALGO_RET_E)gALG_ImplPtr->SetIOU_Thresh(thresh);
        }
        else
        {
            Log("p_in_param:%s, an error occurred while parsing json content.",json.c_str());
            ret = ALGO_RET_JSON_PARSE_NG;
        }
    }
    break;
    case PARAM_SAVE_RESULT_IMAGE_ENABLE:
    {
        if (param_len == sizeof(bool))
        {
            ret = (ALGO_RET_E)gALG_ImplPtr->SetSaveResultImgEnable(*((bool *)p_in_param));
        }
        else
        {
            Log("param_len:%d, sizeof(bool):%d is unequal.", param_len, sizeof(bool));
            ret = ALGO_RET_CHECK_PARAMS_NG;
        }
    }
    break;
    case PARAM_TEST_ASCII:
        {
            gALG_ImplPtr->SetParamTestASCII(p_in_param, param_len);
        }
        break;
    case PARAM_TEST_HEX:
        {
            gALG_ImplPtr->SetParamTestHEX(p_in_param, param_len);
        }
        break;
    default:
        Log("param_id:%d is not support.", param_id);
        ret = ALGO_RET_NOT_SUPPORT;
        break;
    }

    return ret;
}

ALGO_RET_E algo_get_param(uint32_t param_id, const char **p_out_param, uint16_t *p_param_len)
{
    Log("id:%d len:%d, str:%p.", param_id, *p_param_len, *p_out_param);

    ALGO_RET_E ret = ALGO_RET_OK;
    CHECKINIT;

    if (nullptr == p_param_len)
    {
        Log("p_param_len(%p) is invalid.", p_param_len);
        ret = ALGO_RET_CHECK_PARAMS_NG;
        return ret;
    }

    switch (ParamID(param_id))
    {
    case PARAM_SERIALNUMBER:
    {
        Serialization serialization;
        string sn = gALG_ImplPtr->GetSN();
        int result = serialization.GetSerialNumberJson(Serialization::jsonStr, sn);
        if (0 == result)
        {
            *p_out_param = Serialization::jsonStr.c_str();
            *p_param_len = Serialization::jsonStr.length() + 1;
        }
        else
        {
            ret = ALGO_RET_JSON_GENERATE_NG;
        }
    }
    break;
    case PARAM_CONF_THRESH:
    {
        Serialization serialization;
        int result = serialization.GetConf_ThreshJson(Serialization::jsonStr, gALG_ImplPtr->GetConf_Thresh());
        if (0 == result)
        {
            *p_out_param = Serialization::jsonStr.c_str();
            *p_param_len = Serialization::jsonStr.length() + 1;
        }
        else
        {
            ret = ALGO_RET_JSON_GENERATE_NG;
        }
    }
    break;
    case PARAM_IOU_THRESH:
    {
        Serialization serialization;
        int result = serialization.GetIOU_ThreshJson(Serialization::jsonStr, gALG_ImplPtr->GetIOU_Thresh());
        if (0 == result)
        {
            *p_out_param = Serialization::jsonStr.c_str();
            *p_param_len = Serialization::jsonStr.length() + 1;
        }
        else
        {
            ret = ALGO_RET_JSON_GENERATE_NG;
        }
    }
    break;
    case PARAM_TEST_ASCII:
        {
            gALG_ImplPtr->GetParamTestASCII(p_out_param, p_param_len);
        }
        break;
    case PARAM_TEST_HEX:
        {
            gALG_ImplPtr->GetParamTestHEX(p_out_param, p_param_len);
        }
        break;
    default:
        Log("param_id:%d is invalid.", param_id);
        ret = ALGO_RET_NOT_SUPPORT;
        break;
    }

    if(ALGO_RET_JSON_GENERATE_NG == ret)
    {
        Log("param_id:%s, failed to generate json format string.", param_id);
    }

    return ret;
}
