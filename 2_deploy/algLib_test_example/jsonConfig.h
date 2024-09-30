#pragma once
#include <string>
#include "json.h"
#include "alg_api.h"

using std::string;

class JsonConfig
{
public:
    JsonConfig();
    bool Init(const string &jsonConfigPath);
    const LensParameters &GetLensParameters();
    const string &GetRootDirectoryOfAlg();
    const string &GetColorFileRelativePath();
    const string &GetDepthFileRelativePath();
private:
    int Read(const string& configName);
    template<typename T>
    T GetValue(const Json::Value& root, const string& key, T defalut);

private:
    Json::Value m_jsonRoot;
    LensParameters m_lensParameters;
    string m_rootDirectoryOfAlg;
    string m_colorFileRelativePath;
    string m_depthFileRelativePath;
};