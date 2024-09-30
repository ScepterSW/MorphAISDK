#pragma once
#include<string>
#include<vector>
#include "alg_define.h"
#include "alg_types.h"

using std::string;
using std::vector;

class Serialization
{
public:
    int GetSerialNumber(const string& json, string& serialNumber);
    int GetSerialNumberJson(string& json, string& serialNumber);
    int GetConf_Thresh(const string& json, float& conf_Thresh);
    int GetConf_ThreshJson(string& json, float conf_Thresh);
    int GetIOU_Thresh(const string& json, float& iou_Thresh);
    int GetIOU_ThreshJson(string& json, float iou_Thresh);
    int GetResultJson(char* jsonBuf, uint32_t& jsonLen, const detect_result_group_t& detect_result);

private:
    int GetInt(const string& json, const string& key, int& value);
    int GetFloat(const string& json, const string& key, float& value);
    int GetBool(const string& json, const string& key, bool& value);
    int GetString(const string& json, const string& key, string& value);
    template <class T>
    int GetValueJson(string& json, const string& key, T value);

public:
    static string jsonStr;
};



