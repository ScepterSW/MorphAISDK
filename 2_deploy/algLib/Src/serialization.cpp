#include <strstream>
#include "serialization.h"
#include "log.h"
#include "json.h"

#define Camera_SN_STR "Camera_SN"
#define Conf_Thresh_STR "Conf_Thresh"
#define IOU_Thresh_STR "IOU_Thresh"

string Serialization::jsonStr = "";

int Serialization::GetInt(const string& json, const string& key, int& value)
{
    int ret = -1;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    Json::Value jsonValue;
    JSONCPP_STRING errs;
    std::istringstream in(json);
    try
    {
        if (true == parseFromStream(reader, in, &jsonValue, &errs))
        {
            value = jsonValue[key].asInt();
            ret = 0;
        }
    }
    catch (const std::exception &e)
    {
        Log("e:%s", e.what());
    }

    return ret;
}

template <class T>
int Serialization::GetValueJson(string& json, const string& key, T value)
{
    int ret = -1;

    Json::Value jsonValue;
    jsonValue[key] = value;

    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter>jsWriter(builder.newStreamWriter());
    std::ostringstream os;
    jsWriter->write(jsonValue, &os);
    if (false == os.fail())
    {
        json = os.str();
        ret = 0;
    }

    return ret;
}

int Serialization::GetFloat(const string& json, const string& key, float& value)
{
    int ret = -1;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    Json::Value jsonValue;
    JSONCPP_STRING errs;
    std::istringstream in(json);
    try
    {
        if (true == parseFromStream(reader, in, &jsonValue, &errs))
        {
            value = jsonValue[key].asFloat();
            ret = 0;
        }
    }
    catch (const std::exception &e)
    {
        Log("e:%s", e.what());
    }

    return ret;
}

int Serialization::GetBool(const string& json, const string& key, bool& value)
{
    int ret = -1;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    Json::Value jsonValue;
    JSONCPP_STRING errs;
    std::istringstream in(json);
    try
    {
        if (true == parseFromStream(reader, in, &jsonValue, &errs))
        {
            value = jsonValue[key].asBool();
            ret = 0;
        }
    }
    catch (const std::exception &e)
    {
        Log("e:%s", e.what());
    }

    return ret;
}

int Serialization::GetString(const string& json, const string& key, string& value)
{
    int ret = -1;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    Json::Value jsonValue;
    JSONCPP_STRING errs;
    std::istringstream in(json);
    try
    {
        if (true == parseFromStream(reader, in, &jsonValue, &errs))
        {        
            value = jsonValue[key].asCString();
            ret = 0;
        }
    }
    catch(const std::exception& e)
    {
        Log("e:%s", e.what());
    }

    return ret;
}

int Serialization::GetSerialNumber(const string& json, string& serialNumber)
{
    return GetString(json, Camera_SN_STR, serialNumber);
}

int Serialization::GetSerialNumberJson(string& json, string& serialNumber)
{
    return GetValueJson<string>(json, Camera_SN_STR, serialNumber);
}

int Serialization::GetConf_Thresh(const string& json, float& conf_Thresh)
{
    return GetFloat(json, Conf_Thresh_STR, conf_Thresh);
}

int Serialization::GetConf_ThreshJson(string& json, float conf_Thresh)
{
    return GetValueJson<float>(json, Conf_Thresh_STR, conf_Thresh);
}

int Serialization::GetIOU_Thresh(const string& json, float& iou_Thresh)
{
    return GetFloat(json, IOU_Thresh_STR, iou_Thresh);
}

int Serialization::GetIOU_ThreshJson(string& json, float iou_Thresh)
{
    return GetValueJson<float>(json, IOU_Thresh_STR, iou_Thresh);
}

/*
eg:
{
	"mark": {
		"label": [{
			"origin": [328, 277],
			"text": "vzense box"
		}, {
			"origin": [328, 311],
			"text": "163, 255, 1493"
		}],
		"line": [
			[328, 277, 409, 277],
			[409, 277, 409, 346],
			[409, 346, 328, 346],
			[328, 346, 328, 277]
		],
		"point": [
			[368, 311]
		],
		"title": {
			"origin": [200, 15],
			"text": "count:  1"
		}
	}
}
*/
int Serialization::GetResultJson(char* jsonBuf, uint32_t& jsonLen, const detect_result_group_t& detect_result)
{
    int ret = 0;
    Json::Value json;
    char buf[64] ="";
    Json::Value jsonTitle;
    jsonTitle["origin"].append(200);
    jsonTitle["origin"].append(25);
    sprintf(buf, "count:%3d", detect_result.count);
    jsonTitle["text"] = buf;
    json["mark"]["title"] = jsonTitle;

    Json::Value objectInfoArray;
    for (int i = 0; i < detect_result.count; i++)
    {
        const detect_result_t& result = detect_result.results[i];
        Json::Value jsonLabel;
        sprintf(buf, "%s", result.name);
        jsonLabel["text"] = buf;
        jsonLabel["origin"].append(result.box.left);
        jsonLabel["origin"].append(result.box.top);
        json["mark"]["label"].append(jsonLabel);

        jsonLabel.clear();
        sprintf(buf, "%d, %d, %d", result.centerPosInWorld.x, result.centerPosInWorld.y, result.centerPosInWorld.z);
        jsonLabel["text"] = buf;
        jsonLabel["origin"].append(result.box.left);
        jsonLabel["origin"].append((result.box.top + result.box.bottom)/2);
        json["mark"]["label"].append(jsonLabel);

        Json::Value jsonPoint;
        jsonPoint.append((result.box.left + result.box.right) / 2);
        jsonPoint.append((result.box.top + result.box.bottom)/2);
        json["mark"]["point"].append(jsonPoint);


        Json::Value jsonLine;
        // left,top -> right,top
        jsonLine.append(result.box.left);
        jsonLine.append(result.box.top);
        jsonLine.append(result.box.right);
        jsonLine.append(result.box.top);
        json["mark"]["line"].append(jsonLine);
        jsonLine.clear();

        // right,top -> right,bottom
        jsonLine.append(result.box.right);
        jsonLine.append(result.box.top);
        jsonLine.append(result.box.right);
        jsonLine.append(result.box.bottom);
        json["mark"]["line"].append(jsonLine);
        jsonLine.clear();

        // right,bottom -> left,bottom
        jsonLine.append(result.box.right);
        jsonLine.append(result.box.bottom);
        jsonLine.append(result.box.left);
        jsonLine.append(result.box.bottom);
        json["mark"]["line"].append(jsonLine);
        jsonLine.clear();

        // left,bottom -> left,top
        jsonLine.append(result.box.left);
        jsonLine.append(result.box.bottom);
        jsonLine.append(result.box.left);
        jsonLine.append(result.box.top);
        json["mark"]["line"].append(jsonLine);
    }
    
    string StyledString = json.toStyledString();
    memcpy(jsonBuf, StyledString.c_str(), StyledString.size());
    jsonLen += StyledString.size();
    jsonBuf[jsonLen] = '\0';
    jsonLen += 1;

    return ret;
}