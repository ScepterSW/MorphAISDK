#ifndef _IMAGE_UTILS_H_
#define _IMAGE_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "common.h"

/**
 * @brief LetterBox
 * 
 */
typedef struct {
    int x_pad;
    int y_pad;
    float scale;
} letterbox_t;

/**
 * @brief Read image file (support png/jpeg/bmp)
 * 
 * @param path [in] Image path
 * @param image [out] Read image
 * @return int 0: success; -1: error
 */
int read_image(const char* path, image_buffer_t* image);

/**
 * @brief Write image file (support jpg/png)
 * 
 * @param path [in] Image path
 * @param image [in] Image for write (only support IMAGE_FORMAT_RGB888)
 * @return int 0: success; -1: error
 */
int write_image(const char* path, const image_buffer_t* image);

/**
 * @brief Get the image size
 * 
 * @param image [in] Image
 * @return int image size
 */
int get_image_size(image_buffer_t* image);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _IMAGE_UTILS_H_