#pragma once
#include "alg_define.h"

static const int TOF_IMG_W = 640;
static const int TOF_IMG_H = 480;

#define OBJ_NAME_MAX_SIZE 64
#define OBJ_NUMB_MAX_SIZE 128

#pragma pack (push, 1)

typedef struct {
    uint16_t x;
    uint16_t y;
}Point2uint16;

typedef struct{
    int16_t x;
    int16_t y;
    int16_t z;
}Point3int16;

typedef struct _BOX_RECT
{
    int left;
    int right;
    int top;
    int bottom;
} BOX_RECT;

typedef struct __detect_result_t
{
    char name[OBJ_NAME_MAX_SIZE];
    BOX_RECT box;
    float prop;
    Point3int16 centerPosInWorld;
} detect_result_t;

typedef struct _detect_result_group_t
{
    int id;
    int count;
    detect_result_t results[OBJ_NUMB_MAX_SIZE];
} detect_result_group_t;

#pragma pack (pop)