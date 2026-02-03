/* image_utils.h
 * 画像処理ユーティリティ
 */

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <stdint.h>

/* 画像構造体 */
typedef struct {
    int width;
    int height;
    int channels;
    uint8_t *data;
} Image;

/* 関数宣言 */
Image* image_load(const char *filename);
int image_save_jpg(const char *filename, Image *img, int quality);
int image_save_png(const char *filename, Image *img);
void image_free(Image *img);
Image* image_create(int width, int height, int channels);
Image* image_create_like(Image *src);
void get_pixel(Image *img, int u, int v, uint8_t *rgb);
void set_pixel(Image *img, int u, int v, const uint8_t *rgb);
void get_pixel_bilinear(Image *img, double u, double v, uint8_t *rgb);
void image_info(Image *img);

#endif /* IMAGE_UTILS_H */
