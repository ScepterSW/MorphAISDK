#include <unistd.h>
#include <thread>
#include <string>
#include "alg.h"
#include "jsonConfig.h"

#define STB_IMAGE_IMPLEMENTATION
#include "utils/image_utils.h"

using std::string;

bool load_image(const char *p_image_path, CAM_SINGLE_FRAME_T& frame);

int main(int argc, char **argv)
{
    string jsonConfigPath = "/userdata/algorithm_data/algTest/config/algLib_test_example.json";
    
    if (argc > 1)
    {
        jsonConfigPath = argv[1];
    }

    JsonConfig jsonConfig;
    if(false == jsonConfig.Init(jsonConfigPath))
    {
        printf("Failed to parse the configuration file:%s, please check.\n", jsonConfigPath.c_str());
        return -1;
    }

    string rgbFileName = jsonConfig.GetColorFileRelativePath();
    string depthFileName = jsonConfig.GetDepthFileRelativePath();
    LensParameters lensParameters = jsonConfig.GetLensParameters();

    CAM_SINGLE_FRAME_T frame[2] = {{1, 1844323}, {1, 1844323}};
    if (false == load_image(rgbFileName.c_str(), frame[0]) || false == load_image(depthFileName.c_str(), frame[1]))
    {
        return -1;
    }

    Alg alg;
    int result = alg.Initialize(jsonConfig.GetRootDirectoryOfAlg());
    if (ALGO_RET_OK != result)
    {
        printf("Initialize:%d is failed\n.", result);
        return -1;
    }

    bool save_result_img_enable = true;
    alg.Set_param(PARAM_SAVE_RESULT_IMAGE_ENABLE, (char*)&save_result_img_enable, sizeof(save_result_img_enable));
    const string conf_ThreshJson = "{\"Conf_Thresh\" : 0.25}";
    alg.Set_param(PARAM_CONF_THRESH, conf_ThreshJson.c_str(), conf_ThreshJson.size());
    const string iou_ThreshJson = "{\"IOU_Thresh\" : 0.45}";
    alg.Set_param(PARAM_IOU_THRESH, iou_ThreshJson.c_str(), iou_ThreshJson.size());
    
    result = alg.Ctrl_start();
    
    CAM_JOINT_FRAME_T joint_frame = {2, frame, lensParameters};
    result = alg.Push_frame(joint_frame);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    free(frame[0].p_data);
    free(frame[1].p_data);
    alg.Ctrl_stop();    
    alg.Release();

    return 0;
}

bool load_image(const char *p_image_path, CAM_SINGLE_FRAME_T& frame)
{
    bool ret = false;
    image_buffer_t src_image = {0};
    
    int result = read_image(p_image_path, &src_image);
    if (result != 0)
    {
        printf("read_image:%s is failed \n", p_image_path);
        return ret;
    }

    frame.width = src_image.width;
    frame.height = src_image.height;
    frame.data_len = src_image.size;
    frame.p_data = src_image.virt_addr;
    switch (src_image.format)
    {
    case IMAGE_FORMAT_RGB888:
        frame.pixel_format = PIXEL_FORMAT_RGB_888;
        frame.frame_type = FRAME_RGB;
        break;
    case IMAGE_FORMAT_GRAY8:
        frame.pixel_format = PIXEL_FORMAT_GRAY_8;
        frame.frame_type = FRAME_IR;
        break;
    case IMAGE_FORMAT_GRAY16:
        frame.pixel_format = PIXEL_FORMAT_DEPTH_MM16;
        frame.frame_type = FRAME_TRANSFORM_DEPTH_IMG_TO_COLOR_SENSOR;
        break;
    default:
        break;
    }
    ret = true;

    return ret;
}