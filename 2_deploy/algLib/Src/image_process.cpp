#include <algorithm>
#include <chrono>
#include <inttypes.h>
#include "image_process.h"
#include "log.h"

static const int COLOR = 114;
static const int IMG_W = 640;
static const int IMG_H = 480;
static const int IMG_W_HALF = IMG_W / 2;
static const int IMG_H_HALF = IMG_H / 2;

ImgProcess::ImgProcess():
    m_mutex(),
    m_cv(),
    m_frame_input{},
    m_currentIndex(0),
    m_LensParameters_input{},
    m_frame_Inference{0, 0, FRAME_RGB, PIXEL_FORMAT_RGB_888, IMG_W, IMG_W, IMG_W * IMG_W * GetElemSize(PIXEL_FORMAT_RGB_888)},
    m_frame_Depth{0, 0, FRAME_DEPTH, PIXEL_FORMAT_DEPTH_MM16, IMG_W, IMG_H, IMG_W* IMG_H * GetElemSize(PIXEL_FORMAT_DEPTH_MM16)},
    m_PointCloud_Calculate_Tab(),
    m_LensParameters{},
    m_input_img_size(IMG_W_HALF, IMG_H_HALF),
    m_model_img_size(IMG_W_HALF, IMG_W_HALF),
    m_model_img_border_h(40),
    m_img_resize_factor(1.f)
{
    m_frame_input[0][0].p_data = new uint8_t[IMG_W * IMG_W * 3];
    m_frame_input[0][1].p_data = new uint8_t[IMG_W * IMG_W * 2];
    m_frame_input[1][0].p_data = new uint8_t[IMG_W * IMG_W * 3];
    m_frame_input[1][1].p_data = new uint8_t[IMG_W * IMG_W * 2];

    m_frame_Inference.p_data = new uint8_t[m_frame_Inference.data_len];
    memset(m_frame_Inference.p_data, 114, m_frame_Inference.data_len);
    m_frame_Depth.p_data = new uint8_t[m_frame_Depth.data_len];
}

ImgProcess::~ImgProcess()
{
    delete[] m_frame_Depth.p_data;
    delete[] m_frame_Inference.p_data;
    delete[] m_frame_input[0][0].p_data;
    delete[] m_frame_input[0][1].p_data;
    delete[] m_frame_input[1][0].p_data;
    delete[] m_frame_input[1][1].p_data;
}

int ImgProcess::SetModelInputImageSize(int img_w, int img_h)
{
    if(img_w > m_frame_Inference.width || img_h > m_frame_Inference.height)
    {
        Log("The size of the input image for the model is too large, exceeding the size of the pre-requested buffer: model size(%d, %d), buffer size((%d, %d)",
        img_w, img_h, m_frame_Inference.width, m_frame_Inference.height);

        return ALGO_RET_INIT_MODEL_NG;
    }

    m_model_img_size.width = img_w;
    m_model_img_size.height = img_h;

    return ALGO_RET_OK;
}

void ImgProcess::GetRealBox(BOX_RECT box, BOX_RECT& realBox)
{
    realBox.left = int(box.left / m_img_resize_factor + 0.5f); 
    realBox.right = int(box.right / m_img_resize_factor + 0.5f); 
    realBox.top = int((box.top - m_model_img_border_h) / m_img_resize_factor + 0.5f);
    realBox.bottom = int((box.bottom - m_model_img_border_h) / m_img_resize_factor + 0.5f);

    realBox.left = realBox.left > 0 ? (realBox.left < m_input_img_size.width ? realBox.left  : (m_input_img_size.width -1)) : 0;
    realBox.right = realBox.right > 0 ? (realBox.right < m_input_img_size.width ? realBox.right  : (m_input_img_size.width -1)) : 0;
    realBox.top = realBox.top > 0 ? (realBox.top < m_input_img_size.height ? realBox.top  : (m_input_img_size.height -1)) : 0;
    realBox.bottom = realBox.bottom > 0 ? (realBox.bottom < m_input_img_size.height ? realBox.bottom  : (m_input_img_size.height -1)) : 0;

    return;
}

int ImgProcess::GetElemSize(PIXEL_FORMAT_E e)
{
    int elemSize = 1;
    switch (e)
    {
    case PIXEL_FORMAT_DEPTH_MM16:
        elemSize = 2;
        break;
    case PIXEL_FORMAT_GRAY_8:
        elemSize = 1;
        break;
    case PIXEL_FORMAT_RGB_888:
        elemSize = 3;
        break;
    default:
        Log("e:%d is invalid.", e);
        break;
    }
    return elemSize;
}

void ImgProcess::CopyFrame(const CAM_SINGLE_FRAME_T &src, CAM_SINGLE_FRAME_T &dest)
{
    uint8_t* pTemp = dest.p_data;
    memcpy(&dest, &src, sizeof(CAM_SINGLE_FRAME_T));
    dest.p_data = pTemp;
    memcpy(dest.p_data, src.p_data, src.data_len);
}

void ImgProcess::SwapFrame(CAM_SINGLE_FRAME_T &src, CAM_SINGLE_FRAME_T &dest)
{
    uint8_t* pTemp = dest.p_data;
    memcpy(&dest, &src, sizeof(CAM_SINGLE_FRAME_T));
    src.p_data = pTemp;
}

bool ImgProcess::CheckFrame(const CAM_SINGLE_FRAME_T& frame)
{
    int elemSize = GetElemSize(frame.pixel_format);
    if ( nullptr == frame.p_data
        || IMG_W != frame.width
        || IMG_H != frame.height
        || frame.width * frame.height * elemSize != frame.data_len)
    {
        Log("Error: frame: no.:%d type:%d, w:%d, h:%d, pxF:%d len:%d, p:%p", 
            frame.frame_no, frame.frame_type, frame.width, frame.height, frame.pixel_format, frame.data_len, frame.p_data);
        return false;
    }

    return true;
}

int ImgProcess::UpdateData(const CAM_JOINT_FRAME_T *p_joint_frame)
{
    static int fps = 0;
    static int64_t start = LogCustom::GetTimeStampMS();
    int64_t current = LogCustom::GetTimeStampMS();
    fps++;
    if (current - start > 1000)
    {
        Log("UpdateData fps:%.2f", fps * 1000.f / (current - start));
        fps = 0;
        start = current;
    }

    CAM_SINGLE_FRAME_T* p_rgb_frame = nullptr;
    CAM_SINGLE_FRAME_T* p_ir_frame = nullptr;
    CAM_SINGLE_FRAME_T* p_depth_frame = nullptr;

    int ret = 0;
    //
    if(p_joint_frame->frame_cnt!=2)
    {
        ret = ALGO_RET_CHECK_FRAME_COUNT_NG;
        return ret;
    }
    if(FRAME_RGB != p_joint_frame->p_frame[0].frame_type
        || (FRAME_TRANSFORM_DEPTH_IMG_TO_COLOR_SENSOR != p_joint_frame->p_frame[1].frame_type))
    {
        Log("frame check failed. frame_cnt:%d p_frame[0].frame_type:%d p_frame[1].frame_type:%d.", 
        p_joint_frame->frame_cnt, p_joint_frame->p_frame[0].frame_type, p_joint_frame->p_frame[1].frame_type);
        ret = ALGO_RET_CHECK_FRAME_INDEX_NG;
        return ret;
    }

    for (uint8_t i = 0; i < p_joint_frame->frame_cnt; i++)
    {
        CAM_SINGLE_FRAME_T& frame = p_joint_frame->p_frame[i];
        switch (frame.frame_type)
        {
        case FRAME_RGB:
        case FRAME_TRANSFORM_COLOR_IMG_TO_DEPTH_SENSOR:
            if(true == CheckFrame(frame))
            {
                p_rgb_frame = &frame;
            }
            else
            {
                ret = ALGO_RET_CHECK_FRAME_RGB_PARAMS_NG;
                return ret;
            }
            break;
        case FRAME_IR:
            if(true == CheckFrame(frame))
            {
                p_ir_frame = &frame;
            }
            else
            {
                ret = ALGO_RET_CHECK_FRAME_IR_PARAMS_NG;
                return ret;
            }
            break;
        case FRAME_DEPTH:
        case FRAME_TRANSFORM_DEPTH_IMG_TO_COLOR_SENSOR:
            if(true == CheckFrame(frame))
            {
                p_depth_frame = &frame;
            }
            else
            {
                ret = ALGO_RET_CHECK_FRAME_DEPTH_PARAMS_NG;
                return ret;
            }
            break;
        default:
            Log("frame_type:%d is not support.", frame.frame_type);
            ret = ALGO_RET_CHECK_FRAME_SUPPORT_NG;
            return ret;
        }
    }

    if (nullptr != p_rgb_frame && nullptr != p_depth_frame)
    {
        lock_guard<mutex> lk(m_mutex);
        CopyFrame(*p_rgb_frame, m_frame_input[m_currentIndex][0]);
        CopyFrame(*p_depth_frame, m_frame_input[m_currentIndex][1]);
        m_currentIndex = 1 - m_currentIndex;
    }
    memcpy(&m_LensParameters_input, &(p_joint_frame->lens_parameters), sizeof(m_LensParameters_input));
    m_cv.notify_one();
	return ret;
}

int ImgProcess::ProcessData()
{
    int result = ALGO_RET_GET_FRAME_TIMEOUT;
    unique_lock<mutex> lk(m_mutex);
    if (false == m_cv.wait_for(lk, std::chrono::milliseconds(1000), [this] {
        return 0 != m_frame_input[1-m_currentIndex][0].frame_no; }))
    {
        return result;
    }

    int64_t t0 = LogCustom::GetTimeStampMS();
    CAM_SINGLE_FRAME_T& color = m_frame_input[1-m_currentIndex][0];
    m_frame_Inference.frame_no = color.frame_no;
    m_frame_Inference.timestamp = color.timestamp;
    m_frame_Inference.width = m_model_img_size.width;
    m_frame_Inference.height = m_model_img_size.height;
    m_frame_Inference.data_len = m_frame_Inference.width * m_frame_Inference.height * GetElemSize(m_frame_Inference.pixel_format);

    m_img_resize_factor = m_model_img_size.width * 1.f / color.width;
    m_model_img_border_h = int(m_model_img_size.height - color.height * m_img_resize_factor) / 2;

    cv::Mat src = cv::Mat(color.height, color.width, CV_8UC3, color.p_data);
    m_input_img_size = src.size();
    uint8_t *pDest = m_frame_Inference.p_data + m_model_img_border_h * m_model_img_size.width * GetElemSize(m_frame_Inference.pixel_format);
    cv::Mat dest = cv::Mat(std::round(m_input_img_size.height * m_img_resize_factor), std::round(m_input_img_size.width * m_img_resize_factor), CV_8UC3, pDest);
    cv::resize(src, dest, dest.size(), .0, .0, cv::INTER_NEAREST);

    SwapFrame(m_frame_input[1-m_currentIndex][1], m_frame_Depth);

    m_frame_input[1-m_currentIndex][0].frame_no = 0;
    int64_t t1 = LogCustom::GetTimeStampMS();
    Log("cost:%lld", (t1 - t0));

    return 0;
}

int ImgProcess::GetInferenceImg(CAM_SINGLE_FRAME_T &img)
{
    img = m_frame_Inference;
    return 0;
}

int ImgProcess::GetDepthImg(CAM_SINGLE_FRAME_T &img)
{
    img = m_frame_Depth;
    return 0;
}

bool ImgProcess::WriteData(const char* fileName, const uint8_t* pData, uint32_t dataLen)
{
    bool ret = false;
    FILE *fp = fopen(fileName, "wb");
    if (NULL != fp)
    {
        fwrite(pData, dataLen, 1, fp);
        fflush(fp);
        fclose(fp);
        ret = true;
    }
 
    return ret;
}

void ImgProcess::InitPointCloudCalculateTab(const float* tofIntrinsic, const int w, const int h)
{
    if (true == m_PointCloud_Calculate_Tab.empty())
    {
        m_PointCloud_Calculate_Tab = cv::Mat::zeros(h, w, CV_32FC2);
    }

	for (int i = 0; i < h; i++)
	{
		cv::Vec2f *p = m_PointCloud_Calculate_Tab.ptr<cv::Vec2f>(i);
		for (int j = 0; j < w; j++)
		{
			p[j][0] = ((j - tofIntrinsic[0]) / tofIntrinsic[2]);
			p[j][1] = ((i - tofIntrinsic[1]) / tofIntrinsic[3]);
		}
	}

	return;
}

void ImgProcess::UpdatePointCloud(const cv::Mat &depth, const cv::Rect& detectBox, Point3int16& centerPosInWorld)
{
    if (0 != memcmp(m_LensParameters.tofIntrinsic, m_LensParameters_input.tofIntrinsic, sizeof(m_LensParameters.tofIntrinsic)))
    {
        memcpy(m_LensParameters.tofIntrinsic, m_LensParameters_input.tofIntrinsic, sizeof(m_LensParameters.tofIntrinsic));
        InitPointCloudCalculateTab(m_LensParameters.tofIntrinsic, depth.cols, depth.rows);
    }
    
    //The ROI was extracted using the detection box
    const cv::Mat roi_Mat = depth(detectBox);
    const cv::Mat roi_PointCloud_Calculate_Tab = m_PointCloud_Calculate_Tab(detectBox);

    int pointsCount = 0;
    cv::Vec3f pointCloudSum = {};
    //Compute the point cloud, and sum the point cloud
	for (int i = 0; i < roi_Mat.rows; i++)
	{
		const ushort *ptrDepth = roi_Mat.ptr<ushort>(i);
		const cv::Vec2f *ptrCalculateTab = roi_PointCloud_Calculate_Tab.ptr<cv::Vec2f>(i);
		for (int j = 0; j < roi_Mat.cols; j++)
		{
			uint16_t dep_val = ptrDepth[j];
            if(0 < dep_val && dep_val < 3600)
            {
                pointCloudSum[0] += ptrCalculateTab[j][0] * dep_val;
                pointCloudSum[1] += ptrCalculateTab[j][1] * dep_val;
                pointCloudSum[2] += dep_val;
                pointsCount++;
            }
        }
	}

    //Calculate the point cloud mean
    if(pointsCount > 0)
    {
        cv::Vec3f pointCloudCenter = pointCloudSum / pointsCount;
        centerPosInWorld.x = (int16_t)pointCloudCenter[0];
        centerPosInWorld.y = (int16_t)pointCloudCenter[1];
        centerPosInWorld.z = (int16_t)pointCloudCenter[2];
    }

    return;
}

/*
# .PCD v0.7 - Point Cloud Data file format
VERSION 0.7
FIELDS x y z
SIZE 4 4 4
TYPE F F F
COUNT 1 1 1
WIDTH 307200
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS 307200
DATA binary
*/
bool ImgProcess::SavePointsInPCD(const char* fileName, const cv::Mat& points)
{
    bool ret = false;

    const char comments[]="# .PCD v0.7 - Point Cloud Data file format\n";
    const char version[]="VERSION 0.7\n";
    const char FIELDS[]="FIELDS x y z\n";
    const char SIZE[]="SIZE 4 4 4\n";
    const char TYPE[]="TYPE F F F\n";
    const char COUNT[]="COUNT 1 1 1\n";
    const char HEIGHT[]="HEIGHT 1\n";
    const char VIEWPOINT[]="VIEWPOINT 0 0 0 1 0 0 0\n";
    const char DATA_TYPE[]="DATA binary\n";
    const char DATA_TYPE_2[]="DATA ascii\n";
    FILE *fp = fopen(fileName, "wb");
    if (NULL != fp)
    {
        char buf[100] = {};
        
        fwrite(comments, strlen(comments), 1, fp);
        fwrite(version, strlen(version), 1, fp);
        fwrite(FIELDS, strlen(FIELDS), 1, fp);
        fwrite(SIZE, strlen(SIZE), 1, fp);
        fwrite(TYPE, strlen(TYPE), 1, fp);
        fwrite(COUNT, strlen(COUNT), 1, fp);
        sprintf(buf, "WIDTH %d\n", points.rows*points.cols);
        fwrite(buf, strlen(buf), 1, fp);
        fwrite(HEIGHT, strlen(HEIGHT), 1, fp);
        fwrite(VIEWPOINT, strlen(VIEWPOINT), 1, fp);
        sprintf(buf, "POINTS %d\n", points.rows*points.cols);
        fwrite(buf, strlen(buf), 1, fp);
        #if 1
        fwrite(DATA_TYPE, strlen(DATA_TYPE), 1, fp);
        fwrite(points.data, points.rows*points.cols*points.elemSize(), 1, fp);
        #else
        fwrite(DATA_TYPE_2, strlen(DATA_TYPE_2), 1, fp);
        for (size_t i = 0; i < points.rows; i++)
        {
            const cv::Vec3f* p = points.ptr<cv::Vec3f>(i);
            for (size_t j = 0; j < points.cols; j++)
            {
                if(0 < p[j][2] && p[j][2]< 3800)
                {
                    sprintf(buf, "%.1f %.1f %.1f\n", p[j][0], p[j][1], p[j][2]);
                    fwrite(buf, strlen(buf), 1, fp);
                }
            }
        }
        #endif

        fflush(fp);
        fclose(fp);
        ret = true;
    }

    return ret;
}