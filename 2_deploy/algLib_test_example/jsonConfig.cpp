#include "jsonConfig.h"

const string APP_STR = "app";
const string LENSPARAMETERS_STR = "lensParameters";
const string TOFINTRINSIC_STR = "tofIntrinsic";
const string COLORINTRINSIC_STR = "colorIntrinsic";
const string ROTATION_STR = "rotation";
const string TRANSLATION_STR = "translation";
const string COLORFILERELATIVEPATH_STR = "colorFileRelativePath";
const string DEPTHFILERELATIVEPATH_STR = "depthFileRelativePath";
const string ROOTDIRECTORYOFALG_STR = "rootDirectoryOfAlg";

JsonConfig::JsonConfig() : m_jsonRoot(),
                           m_lensParameters{0},
                           m_rootDirectoryOfAlg("/userdata/algorithm_data"),
                           m_colorFileRelativePath("./testdata/Color_00000001.jpg"),
                           m_depthFileRelativePath("./testdata/DepthImgToColorSensor_00000001.png")
{
}

bool JsonConfig::Init(const string &jsonConfigPath)
{
    bool ret = false;
    if (0 != Read(jsonConfigPath))
    {
        return ret;
    }
    
    Json::Value appParams = m_jsonRoot[APP_STR];
    Json::Value lensParametersValue = appParams[LENSPARAMETERS_STR];
    if(true == lensParametersValue.empty())
    {
        printf("%s is not found in the json configuration.\n", LENSPARAMETERS_STR.c_str());
        return ret;
    }

    Json::Value tofIntrinsicValue = lensParametersValue[TOFINTRINSIC_STR];
    if (tofIntrinsicValue.size() == sizeof(m_lensParameters.tofIntrinsic)/sizeof(m_lensParameters.tofIntrinsic[0]))
    {
        for (Json::Value::ArrayIndex i = 0; i < tofIntrinsicValue.size(); i++)
        {
            m_lensParameters.tofIntrinsic[i] = tofIntrinsicValue[i].asFloat();
        }
    }

    Json::Value colorIntrinsicValue = lensParametersValue[COLORINTRINSIC_STR];
    if (colorIntrinsicValue.size() == sizeof(m_lensParameters.colorIntrinsic)/sizeof(m_lensParameters.colorIntrinsic[0]))
    {
        for (Json::Value::ArrayIndex i = 0; i < colorIntrinsicValue.size(); i++)
        {
            m_lensParameters.colorIntrinsic[i] = colorIntrinsicValue[i].asFloat();
        }
    }
    
    Json::Value rotationValue = lensParametersValue[ROTATION_STR];
    if (rotationValue.size() == sizeof(m_lensParameters.rotation)/sizeof(m_lensParameters.rotation[0]))
    {
        for (Json::Value::ArrayIndex i = 0; i < rotationValue.size(); i++)
        {
            m_lensParameters.rotation[i] = rotationValue[i].asFloat();
        }
    }

    Json::Value translationValue = lensParametersValue[TRANSLATION_STR];
    if (translationValue.size() == sizeof(m_lensParameters.translation)/sizeof(m_lensParameters.translation[0]))
    {
        for (Json::Value::ArrayIndex i = 0; i < translationValue.size(); i++)
        {
            m_lensParameters.translation[i] = translationValue[i].asFloat();
        }
    }

    m_rootDirectoryOfAlg = GetValue(appParams, ROOTDIRECTORYOFALG_STR, m_rootDirectoryOfAlg);
    m_colorFileRelativePath = GetValue(appParams, COLORFILERELATIVEPATH_STR, m_colorFileRelativePath);
    m_depthFileRelativePath = GetValue(appParams, DEPTHFILERELATIVEPATH_STR, m_depthFileRelativePath);
    
    ret = true;

    return ret;
}

int JsonConfig::Read(const string& configName)
{
    int ret = -1;

    FILE* fp = fopen(configName.c_str(), "r");
	if (fp == NULL) {
		printf("Error: Unable to open the file %s.\n", configName.c_str());
		return ret;
	}

    fseek(fp, 0, SEEK_END);
    const int FSize = ftell(fp);
    rewind(fp);

    char* pBuf = new char[FSize + 1];
    memset(pBuf, 0, (FSize + 1));
    fread(pBuf, 1, FSize, fp);
	fclose(fp);

    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    JSONCPP_STRING errs;
    std::istringstream in(pBuf);
    delete[] pBuf;

    if (true == parseFromStream(reader, in, &m_jsonRoot, &errs))
    {
        ret = 0;
    }

    return ret;
}

template<typename T>
T JsonConfig::GetValue(const Json::Value& root, const string& key, T defalut)
{
    T value;
    Json::Value json = root[key];
    if (true == json.empty())
    {
        value = defalut;
    }
    else
    {
        value = json.as<T>();
    }
    return value;
}

const LensParameters &JsonConfig::GetLensParameters()
{
    return m_lensParameters;
}

const string &JsonConfig::GetRootDirectoryOfAlg()
{
    return m_rootDirectoryOfAlg;
}

const string &JsonConfig::GetColorFileRelativePath()
{
    return m_colorFileRelativePath;
}

const string &JsonConfig::GetDepthFileRelativePath()
{
    return m_depthFileRelativePath;
}