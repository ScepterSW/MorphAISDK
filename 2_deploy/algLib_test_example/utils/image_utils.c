#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <sys/time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "turbojpeg.h"

#include "image_utils.h"
#include "file_utils.h"

static const char* filter_image_names[] = {
    "jpg",
    "jpeg",
    "JPG",
    "JPEG",
    "png",
    "PNG",
    "data",
    NULL
};

static const char* subsampName[TJ_NUMSAMP] = {"4:4:4", "4:2:2", "4:2:0", "Grayscale", "4:4:0", "4:1:1"};

static const char* colorspaceName[TJ_NUMCS] = {"RGB", "YCbCr", "GRAY", "CMYK", "YCCK"};

static int image_file_filter(const struct dirent *entry)
{
    const char ** filter;

    for (filter = filter_image_names; *filter; ++filter) {
        if(strstr(entry->d_name, *filter) != NULL) {
            return 1;
        }
    }
    return 0;
}

static int read_image_jpeg(const char* path, image_buffer_t* image)
{
    FILE* jpegFile = NULL;
    unsigned long jpegSize;
    int flags = 0;
    int width, height;
    int origin_width, origin_height;
    unsigned char* imgBuf = NULL;
    unsigned char* jpegBuf = NULL;
    unsigned long size;
    unsigned short orientation = 1;
    struct timeval tv1, tv2;

    if ((jpegFile = fopen(path, "rb")) == NULL) {
        printf("open input file failure\n");
    }
    if (fseek(jpegFile, 0, SEEK_END) < 0 || (size = ftell(jpegFile)) < 0 || fseek(jpegFile, 0, SEEK_SET) < 0) {
        printf("determining input file size failure\n");
    }
    if (size == 0) {
        printf("determining input file size, Input file contains no data\n");
    }
    jpegSize = (unsigned long)size;
    if ((jpegBuf = (unsigned char*)malloc(jpegSize * sizeof(unsigned char))) == NULL) {
        printf("allocating JPEG buffer\n");
    }
    if (fread(jpegBuf, jpegSize, 1, jpegFile) < 1) {
        printf("reading input file");
    }
    fclose(jpegFile);
    jpegFile = NULL;

    tjhandle handle = NULL;
    int subsample, colorspace;
    int padding = 1;
    int ret = 0;

    handle = tjInitDecompress();
    ret = tjDecompressHeader3(handle, jpegBuf, size, &origin_width, &origin_height, &subsample, &colorspace);
    if (ret < 0) {
        printf("header file error, errorStr:%s, errorCode:%d\n", tjGetErrorStr(), tjGetErrorCode(handle));
        return -1;
    }

    // 对图像做裁剪16对齐，利于后续rga操作
    int crop_width = origin_width / 16 * 16;
    int crop_height = origin_height / 16 * 16;

    // gettimeofday(&tv1, NULL);
    ret = tjDecompressHeader3(handle, jpegBuf, size, &width, &height, &subsample, &colorspace);
    if (ret < 0) {
        printf("header file error, errorStr:%s, errorCode:%d\n", tjGetErrorStr(), tjGetErrorCode(handle));
        return -1;
    }

    int sw_out_size = width * height * 3;
    unsigned char* sw_out_buf = image->virt_addr;
    if (sw_out_buf == NULL) {
        sw_out_buf = (unsigned char*)malloc(sw_out_size * sizeof(unsigned char));
    }
    if (sw_out_buf == NULL) {
        printf("sw_out_buf is NULL\n");
        goto out;
    }

    flags |= 0;

    // 错误码为0时，表示警告，错误码为-1时表示错误
    int pixelFormat = TJPF_RGB;
    ret = tjDecompress2(handle, jpegBuf, size, sw_out_buf, width, 0, height, pixelFormat, flags);
    // ret = tjDecompressToYUV2(handle, jpeg_buf, size, dst_buf, *width, padding, *height, flags);
    if ((0 != tjGetErrorCode(handle)) && (ret < 0)) {
        printf("error : decompress to yuv failed, errorStr:%s, errorCode:%d\n", tjGetErrorStr(),
               tjGetErrorCode(handle));
        goto out;
    }
    if ((0 == tjGetErrorCode(handle)) && (ret < 0)) {
        printf("warning : errorStr:%s, errorCode:%d\n", tjGetErrorStr(), tjGetErrorCode(handle));
    }
    tjDestroy(handle);
    // gettimeofday(&tv2, NULL);
    // printf("decode time %ld ms\n", (tv2.tv_sec-tv1.tv_sec)*1000 + (tv2.tv_usec-tv1.tv_usec)/1000);

    image->width = width;
    image->height = height;
    image->format = IMAGE_FORMAT_RGB888;
    image->virt_addr = sw_out_buf;
    image->size = sw_out_size;
out:
    if (jpegBuf) {
        free(jpegBuf);
    }
    return 0;
}

static int read_image_raw(const char* path, image_buffer_t* image)
{
    FILE *fp = fopen(path, "rb");
    if(fp == NULL) {
        printf("fopen %s fail!\n", path);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    unsigned char *data = image->virt_addr;
    if (image->virt_addr == NULL) {
        data = (unsigned char *)malloc(file_size+1);
    }
    data[file_size] = 0;
    fseek(fp, 0, SEEK_SET);
    if(file_size != fread(data, 1, file_size, fp)) {
        printf("fread %s fail!\n", path);
        free(data);
        return -1;
    }
    if(fp) {
        fclose(fp);
    }
    if (image->virt_addr == NULL) {
        image->virt_addr = data;
        image->size = file_size;
    }

    return 0;
}

static int write_image_jpeg(const char* path, int quality, const image_buffer_t* image)
{
    int ret;
    int jpegSubsamp = TJSAMP_422;
    unsigned char* jpegBuf = NULL;
    unsigned long jpegSize = 0;
    int flags = 0;

    const unsigned char* data = image->virt_addr;
    int width = image->width;
    int height = image->height;
    int pixelFormat = TJPF_RGB;

	tjhandle handle = tjInitCompress();

    if (image->format == IMAGE_FORMAT_RGB888) {
        ret = tjCompress2(handle, data, width, 0, height, pixelFormat, &jpegBuf, &jpegSize, jpegSubsamp, quality, flags);
    } else {
        printf("write_image_jpeg: pixel format %d not support\n", image->format);
        return -1;
    }

	// printf("ret=%d jpegBuf=%p jpegSize=%d\n", ret, jpegBuf, jpegSize);
    if (jpegBuf != NULL && jpegSize > 0) {
        write_data_to_file(path, (const char*)jpegBuf, jpegSize);
        tjFree(jpegBuf);
    }
    tjDestroy(handle);

	return 0;
}

static int read_image_stb(const char* path, image_buffer_t* image)
{
    const int is16bit = stbi_is_16_bit(path);
    int w, h, c;
    unsigned char* pixeldata = (1 == is16bit) ? stbi_load_16(path, &w, &h, &c, 0) : stbi_load(path, &w, &h, &c, 0);
    if (!pixeldata) {
        printf("error: read image %s fail\n", path);
        return -1;
    }
    // printf("load image wxhxc=%dx%dx%d path=%s\n", w, h, c, path);
    int size = w * h * c;

    // 设置图像数据
    if (image->virt_addr != NULL) {
        memcpy(image->virt_addr, pixeldata, size);
        stbi_image_free(pixeldata);
    } else {
        image->virt_addr = pixeldata;
    }
    image->width = w;
    image->height = h;
    image->size = size;
    if (c == 4) {
        image->format = IMAGE_FORMAT_RGBA8888;
    } else if (c == 1) {
        if (1 == is16bit)
        {
            image->format = IMAGE_FORMAT_GRAY16;
            image->size = image->size * 2;
        }
        else
        {
            image->format = IMAGE_FORMAT_GRAY8;
        }
        
    } else {
        image->format = IMAGE_FORMAT_RGB888;
    }
    return 0;
}

int read_image(const char* path, image_buffer_t* image)
{
    const char* _ext = strrchr(path, '.');
    if (!_ext) {
        // missing extension
        return -1;
    }
    if (strcmp(_ext, ".data") == 0) {
        return read_image_raw(path, image);
    } else if (strcmp(_ext, ".jpg") == 0 || strcmp(_ext, ".jpeg") == 0 || strcmp(_ext, ".JPG") == 0 ||
        strcmp(_ext, ".JPEG") == 0) {
        return read_image_jpeg(path, image);
    } else {
        return read_image_stb(path, image);
    }
}

int write_image(const char* path, const image_buffer_t* img)
{
    int ret;
    int width = img->width;
    int height = img->height;
    int channel = 3;
    void* data = img->virt_addr;

    const char* _ext = strrchr(path, '.');
    if (!_ext) {
        // missing extension
        return -1;
    }
    if (strcmp(_ext, ".jpg") == 0 || strcmp(_ext, ".jpeg") == 0 || strcmp(_ext, ".JPG") == 0 ||
        strcmp(_ext, ".JPEG") == 0) {
        int quality = 95;
        ret = write_image_jpeg(path, quality, img);
    } else if (strcmp(_ext, ".png") == 0 | strcmp(_ext, ".PNG") == 0) {
        ret = stbi_write_png(path, width, height, channel, data, 0);
    } else if (strcmp(_ext, ".data") == 0 | strcmp(_ext, ".DATA") == 0) {
        int size = get_image_size(img);
        ret = write_data_to_file(path, data, size);
    } else {
        // unknown extension type
        return -1;
    }
    return ret;
}

static int crop_and_scale_image_c(int channel, unsigned char *src, int src_width, int src_height,
                                    int crop_x, int crop_y, int crop_width, int crop_height,
                                    unsigned char *dst, int dst_width, int dst_height,
                                    int dst_box_x, int dst_box_y, int dst_box_width, int dst_box_height) {
    if (dst == NULL) {
        printf("dst buffer is null\n");
        return -1;
    }

    float x_ratio = (float)crop_width / (float)dst_box_width;
    float y_ratio = (float)crop_height / (float)dst_box_height;

    // printf("src_width=%d src_height=%d crop_x=%d crop_y=%d crop_width=%d crop_height=%d\n",
    //     src_width, src_height, crop_x, crop_y, crop_width, crop_height);
    // printf("dst_width=%d dst_height=%d dst_box_x=%d dst_box_y=%d dst_box_width=%d dst_box_height=%d\n",
    //     dst_width, dst_height, dst_box_x, dst_box_y, dst_box_width, dst_box_height);
    // printf("channel=%d x_ratio=%f y_ratio=%f\n", channel, x_ratio, y_ratio);

    // 从原图指定区域取数据，双线性缩放到目标指定区域
    for (int dst_y = dst_box_y; dst_y < dst_box_y + dst_box_height; dst_y++) {
        for (int dst_x = dst_box_x; dst_x < dst_box_x + dst_box_width; dst_x++) {
            int dst_x_offset = dst_x - dst_box_x;
            int dst_y_offset = dst_y - dst_box_y;

            int src_x = (int)(dst_x_offset * x_ratio) + crop_x;
            int src_y = (int)(dst_y_offset * y_ratio) + crop_y;

            float x_diff = (dst_x_offset * x_ratio) - (src_x - crop_x);
            float y_diff = (dst_y_offset * y_ratio) - (src_y - crop_y);

            int index1 = src_y * src_width * channel + src_x * channel;
            int index2 = index1 + src_width * channel;    // down
            if (src_y == src_height - 1) {
                // 如果到图像最下边缘，变成选择上面的像素
                index2 = index1 - src_width * channel;
            }
            int index3 = index1 + 1 * channel;            // right
            int index4 = index2 + 1 * channel;            // down right
            if (src_x == src_width - 1) {
                // 如果到图像最右边缘，变成选择左边的像素
                index3 = index1 - 1 * channel;
                index4 = index2 - 1 * channel;
            }

            // printf("dst_x=%d dst_y=%d dst_x_offset=%d dst_y_offset=%d src_x=%d src_y=%d x_diff=%f y_diff=%f src index=%d %d %d %d\n",
            //     dst_x, dst_y, dst_x_offset, dst_y_offset,
            //     src_x, src_y, x_diff, y_diff,
            //     index1, index2, index3, index4);

            for (int c = 0; c < channel; c++) {
                unsigned char A = src[index1+c];
                unsigned char B = src[index3+c];
                unsigned char C = src[index2+c];
                unsigned char D = src[index4+c];

                unsigned char pixel = (unsigned char)(
                    A * (1 - x_diff) * (1 - y_diff) +
                    B * x_diff * (1 - y_diff) +
                    C * y_diff * (1 - x_diff) +
                    D * x_diff * y_diff
                );

                dst[(dst_y * dst_width  + dst_x) * channel + c] = pixel;
            }
        }
    }

    return 0;
}

static int crop_and_scale_image_yuv420sp(unsigned char *src, int src_width, int src_height,
                                    int crop_x, int crop_y, int crop_width, int crop_height,
                                    unsigned char *dst, int dst_width, int dst_height,
                                    int dst_box_x, int dst_box_y, int dst_box_width, int dst_box_height) {

    unsigned char* src_y = src;
    unsigned char* src_uv = src + src_width * src_height;

    unsigned char* dst_y = dst;
    unsigned char* dst_uv = dst + dst_width * dst_height;

    crop_and_scale_image_c(1, src_y, src_width, src_height, crop_x, crop_y, crop_width, crop_height,
        dst_y, dst_width, dst_height, dst_box_x, dst_box_y, dst_box_width, dst_box_height);
    
    crop_and_scale_image_c(2, src_uv, src_width / 2, src_height / 2, crop_x / 2, crop_y / 2, crop_width / 2, crop_height / 2,
        dst_uv, dst_width / 2, dst_height / 2, dst_box_x, dst_box_y, dst_box_width, dst_box_height);

    return 0;
}

static int convert_image_cpu(image_buffer_t *src, image_buffer_t *dst, image_rect_t *src_box, image_rect_t *dst_box, char color) {
    int ret;
    if (dst->virt_addr == NULL) {
        return -1;
    }
    if (src->virt_addr == NULL) {
        return -1;
    }
    if (src->format != dst->format) {
        return -1;
    }

    int src_box_x = 0;
    int src_box_y = 0;
    int src_box_w = src->width;
    int src_box_h = src->height;
    if (src_box != NULL) {
        src_box_x = src_box->left;
        src_box_y = src_box->top;
        src_box_w = src_box->right - src_box->left + 1;
        src_box_h = src_box->bottom - src_box->top + 1;
    }
    int dst_box_x = 0;
    int dst_box_y = 0;
    int dst_box_w = dst->width;
    int dst_box_h = dst->height;
    if (dst_box != NULL) {
        dst_box_x = dst_box->left;
        dst_box_y = dst_box->top;
        dst_box_w = dst_box->right - dst_box->left + 1;
        dst_box_h = dst_box->bottom - dst_box->top + 1;
    }

    // fill pad color
    if (dst_box_w != dst->width || dst_box_h != dst->height) {
        int dst_size = get_image_size(dst);
        memset(dst->virt_addr, color, dst_size);
    }

    int need_release_dst_buffer = 0;
    int reti = 0;
    if (src->format == IMAGE_FORMAT_RGB888) {
        reti = crop_and_scale_image_c(3, src->virt_addr, src->width, src->height,
            src_box_x, src_box_y, src_box_w, src_box_h,
            dst->virt_addr, dst->width, dst->height,
            dst_box_x, dst_box_y, dst_box_w, dst_box_h);
    } else if (src->format == IMAGE_FORMAT_RGBA8888) {
        reti = crop_and_scale_image_c(4, src->virt_addr, src->width, src->height,
            src_box_x, src_box_y, src_box_w, src_box_h,
            dst->virt_addr, dst->width, dst->height,
            dst_box_x, dst_box_y, dst_box_w, dst_box_h);
    } else if (src->format == IMAGE_FORMAT_GRAY8) {
        reti = crop_and_scale_image_c(1, src->virt_addr, src->width, src->height,
            src_box_x, src_box_y, src_box_w, src_box_h,
            dst->virt_addr, dst->width, dst->height,
            dst_box_x, dst_box_y, dst_box_w, dst_box_h);
    } else if (src->format == IMAGE_FORMAT_YUV420SP_NV12 || src->format == IMAGE_FORMAT_YUV420SP_NV21) {
        reti = crop_and_scale_image_yuv420sp(src->virt_addr, src->width, src->height,
            src_box_x, src_box_y, src_box_w, src_box_h,
            dst->virt_addr, dst->width, dst->height,
            dst_box_x, dst_box_y, dst_box_w, dst_box_h);
    } else {
        printf("no support format %d\n", src->format);
    }
    if (reti != 0) {
        printf("convert_image_cpu fail %d\n", reti);
        return -1;
    }
    return 0;
}

int get_image_size(image_buffer_t* image)
{
    if (image == NULL) {
        return 0;
    }
    switch (image->format)
    {
    case IMAGE_FORMAT_GRAY8:
        return image->width * image->height;
    case IMAGE_FORMAT_RGB888:
        return image->width * image->height * 3;    
    case IMAGE_FORMAT_RGBA8888:
        return image->width * image->height * 4;
    case IMAGE_FORMAT_YUV420SP_NV12:
    case IMAGE_FORMAT_YUV420SP_NV21:
        return image->width * image->height * 3 / 2;
    default:
        break;
    }
}
