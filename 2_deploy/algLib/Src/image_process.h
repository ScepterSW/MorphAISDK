#ifndef IMGPROCESS_H
#define IMGPROCESS_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <opencv2/opencv.hpp>
#include "alg_types.h"

using std::mutex;
using std::condition_variable;

static const int LABEL_BORDER = 80;

class ImgProcess
{
public:
	ImgProcess();
	~ImgProcess();
	int UpdateData(const CAM_JOINT_FRAME_T *p_joint_frame);
    int ProcessData();
	int GetInferenceImg(CAM_SINGLE_FRAME_T &img);
	int GetDepthImg(CAM_SINGLE_FRAME_T &img);
    void CopyFrame(const CAM_SINGLE_FRAME_T &src, CAM_SINGLE_FRAME_T &dest);
    void SwapFrame(CAM_SINGLE_FRAME_T &src, CAM_SINGLE_FRAME_T &dest);
    void UpdatePointCloud(const cv::Mat &depth, const cv::Rect& detectBox, Point3int16& centerPosInWorld);
	static int GetElemSize(PIXEL_FORMAT_E e);
    static bool WriteData(const char* fileName, const uint8_t* pData, uint32_t dataLen);
private:
    bool CheckFrame(const CAM_SINGLE_FRAME_T& frame);
    void InitPointCloudCalculateTab(const float* tofIntrinsic, const int w, const int h);
    bool SavePointsInPCD(const char* fileName, const cv::Mat& points);

private:
    mutex m_mutex;
    condition_variable m_cv;
    CAM_SINGLE_FRAME_T m_frame_input[2];
    LensParameters m_LensParameters_input;
    CAM_SINGLE_FRAME_T m_frame_Inference;
    CAM_SINGLE_FRAME_T m_frame_Depth;
    cv::Mat m_PointCloud_Calculate_Tab;
    LensParameters m_LensParameters;
};

#endif // !IMGPROCESS_H



