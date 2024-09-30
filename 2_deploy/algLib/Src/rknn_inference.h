#ifndef RKNNINFERENCE_H
#define RKNNINFERENCE_H

#include <vector>
#include "rknn_utils.h"
#include "post_process.h"
#include "rga/im2d.hpp"
#include "rga/rga.h"

using std::vector;
struct CameraSingleFrame;

class RKNNInference 
{
public:
	RKNNInference();
	~RKNNInference();
	int Init(const char* pWeightFilePath);
	int SetConf_Thresh(float threshold);
	int SetIOU_Thresh(float threshold);
	int SetSaveResultImgEnable(bool enable);
	float GetConf_Thresh(){ return m_conf_thres;}
	float GetIOU_Thresh(){ return m_iou_thres;}
    int Detect(const CameraSingleFrame &img, detect_result_group_t& detect_result_group);

private:
	PostProcess *m_postprocess_p;
	MODEL_INFO m_model_info;
    float m_conf_thres;
    float m_iou_thres;
	rga_buffer_t m_dst_img;
	bool m_used_zero_copy;
	bool m_save_result_img_enable;
};

#endif //RKNNINFERENCE_H