#include "rknn_inference.h"
#include "log.h"

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "image_process.h"
#include "post_process_custom.h"
#include "post_process_yolov5.h"
#include "post_process_yolov8.h"
#include "post_process_vzense_box.h"

const int  N_CLASS_COLORS = 20;
unsigned char class_colors[N_CLASS_COLORS][3] = {
    {255, 56, 56},   // 'FF3838'
    {255, 157, 151}, // 'FF9D97'
    {255, 112, 31},  // 'FF701F'
    {255, 178, 29},  // 'FFB21D'
    {207, 210, 49},  // 'CFD231'
    {72, 249, 10},   // '48F90A'
    {146, 204, 23},  // '92CC17'
    {61, 219, 134},  // '3DDB86'
    {26, 147, 52},   // '1A9334'
    {0, 212, 187},   // '00D4BB'
    {44, 153, 168},  // '2C99A8'
    {0, 194, 255},   // '00C2FF'
    {52, 69, 147},   // '344593'
    {100, 115, 255}, // '6473FF'
    {0, 24, 236},    // '0018EC'
    {132, 56, 255},  // '8438FF'
    {82, 0, 133},    // '520085'
    {203, 56, 255},  // 'CB38FF'
    {255, 149, 200}, // 'FF95C8'
    {255, 55, 199}   // 'FF37C7'
};

#define NMS_THRESH 0.45f
#define BOX_THRESH 0.25f

RKNNInference::RKNNInference():
#ifdef YOLOV5
    m_postprocess_p(new PostProcessYolov5),
#elif defined YOLOV8
    m_postprocess_p(new PostProcessYolov8),
#elif defined VZENSE_BOX
    m_postprocess_p(new PostProcessVzenseBox),
#else
    m_postprocess_p(new PostProcessCustom),
#endif
    
    m_model_info{0},
    m_conf_thres(BOX_THRESH),
    m_iou_thres(NMS_THRESH),
    m_dst_img{0},
    m_used_zero_copy(false),
    m_save_result_img_enable(false)
{}

RKNNInference::~RKNNInference()
{    
    rkdemo_model_release(&m_model_info);
    if (m_postprocess_p)
    {
        delete m_postprocess_p;
        m_postprocess_p = nullptr;
    }
    
}

int RKNNInference::Init(const char* pWeightFilePath)
{
	int ret = -1;
        
    m_model_info.use_zp = m_used_zero_copy;
    Log("pWeightFilePath:%s", pWeightFilePath);
    ret = rkdemo_model_init(pWeightFilePath, &m_model_info);

    if (true == m_used_zero_copy)
    {
        m_dst_img = wrapbuffer_fd(m_model_info.in_mems[0]->fd, m_model_info.img_w, m_model_info.img_h, RK_FORMAT_RGB_888);
        if (0 == m_dst_img.width) {
            Log("wrapbuffer_fd failed!");
            return ret;
        }
    }

    return ret;
}

int RKNNInference::SetConf_Thresh(float threshold)
{
    Log("confidence threshold:%f.", threshold);
    m_conf_thres = threshold;
    return 0;
}

int RKNNInference::SetIOU_Thresh(float threshold)
{
    Log("iou threshold:%f.", threshold);
    m_iou_thres = threshold;
    return 0;
}

int RKNNInference::SetSaveResultImgEnable(bool enable)
{
    Log("enable:%d.", enable);
    m_save_result_img_enable = enable;
    return 0;
}

int RKNNInference::Detect(const CameraSingleFrame &img, detect_result_group_t& detect_result_group)
{
    int ret = 0;

    int64_t t0 = LogCustom::GetTimeStampMS();
    if (true == m_used_zero_copy)
    {
        rga_buffer_t src_img = wrapbuffer_virtualaddr(img.p_data, img.width, img.height, RK_FORMAT_RGB_888);
        if (0 == src_img.width) {
            Log("wrapbuffer_fd failed!");
            ret = -1;
            return ret;
        }

        ret = imcopy(src_img, m_dst_img);
        if(ret < 0)
        {
            Log("imcopy is failed, result: %d", ret);
            return ret;
        }

        ret = rknn_set_io_mem(m_model_info.ctx, m_model_info.in_mems[0], &m_model_info.in_attrs[0]);
        if(ret < 0)
        {
            Log("result is failed, result: %d", ret);
            return ret;
        }
    }
    else
    {
        memcpy(m_model_info.inputs[0].buf, img.p_data, img.data_len);
        ret = rknn_inputs_set(m_model_info.ctx, m_model_info.n_input, m_model_info.inputs);
    }

    int64_t t1 = LogCustom::GetTimeStampMS();
    ret = rknn_run(m_model_info.ctx, NULL);
    if(ret < 0)
    {
        Log("result: %d", ret);
        return ret;
    }
    int64_t t2 = LogCustom::GetTimeStampMS();
    m_postprocess_p->Process(&m_model_info, m_conf_thres, m_iou_thres, &detect_result_group);

    int64_t t3 = LogCustom::GetTimeStampMS();
    Log("detect_result_group.count:%d - %lld %lld %lld %lld", detect_result_group.count, t3-t0, t1-t0, t2-t1, t3-t2);

    if( true == m_save_result_img_enable)
    {
        cv::Mat imgShow = cv::Mat(img.height, img.width, CV_8UC3, img.p_data);
        for (int i = 0; i < detect_result_group.count; i++)
        {
            detect_result_t &det_result = detect_result_group.results[i];

            cv::rectangle(imgShow, cv::Rect(det_result.box.left, det_result.box.top, (det_result.box.right - det_result.box.left), (det_result.box.bottom - det_result.box.top)),
            cv::Scalar(255,255,255));

            det_result.box.top = ((det_result.box.top - LABEL_BORDER) < 0) ? 0 : (det_result.box.top - LABEL_BORDER);
            det_result.box.bottom = ((det_result.box.bottom - LABEL_BORDER) < 0) ? 0 : (det_result.box.bottom - LABEL_BORDER);
            det_result.box.bottom = (det_result.box.bottom > (img.height - (2 * LABEL_BORDER))) ? (img.height - (2 * LABEL_BORDER)) : det_result.box.bottom;
            Log("%s @ (%d %d %d %d) %f",
                det_result.name,
                det_result.box.left, det_result.box.top, det_result.box.right, det_result.box.bottom,
                det_result.prop);
        }
        cv::Mat imgRGB;
        cv::cvtColor(imgShow, imgRGB, cv::COLOR_BGR2RGB);
        cv::imwrite("result.png", imgRGB);
    }
    else
    {
        for (int i = 0; i < detect_result_group.count; i++)
        {
            detect_result_t &det_result = detect_result_group.results[i];

            det_result.box.top = ((det_result.box.top - LABEL_BORDER) < 0) ? 0 : (det_result.box.top - LABEL_BORDER);
            det_result.box.bottom = ((det_result.box.bottom - LABEL_BORDER) < 0) ? 0 : (det_result.box.bottom - LABEL_BORDER);
            det_result.box.bottom = (det_result.box.bottom > (img.height - (2 * LABEL_BORDER))) ? (img.height - (2 * LABEL_BORDER)) : det_result.box.bottom;
            Log("%s @ (%d %d %d %d) %f",
                det_result.name,
                det_result.box.left, det_result.box.top, det_result.box.right, det_result.box.bottom,
                det_result.prop);
        }
    }
    
    ret = 0;
    return ret;
}