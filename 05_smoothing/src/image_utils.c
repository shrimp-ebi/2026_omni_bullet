/* image_utils.c
 * 画像処理ユーティリティの実装
 * 
 * stb_imageを使用
 */

/* image_utils.c */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>



/* ===========================
 * 画像の読み込み・保存
 * =========================== */

/* 画像ファイルを読み込む */
Image* image_load(const char *filename) {
    Image *img = (Image*)malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "エラー: メモリ確保失敗\n");
        return NULL;
    }
    
    /* stb_imageで画像を読み込む */
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
    
    if (!img->data) {
        fprintf(stderr, "エラー: 画像ファイルが読み込めません: %s\n", filename);
        fprintf(stderr, "       理由: %s\n", stbi_failure_reason());
        free(img);
        return NULL;
    }
    
    printf("画像読み込み成功: %s\n", filename);
    printf("  サイズ: %d × %d\n", img->width, img->height);
    printf("  チャンネル数: %d\n", img->channels);
    
    return img;
}

/* 画像をJPEGファイルとして保存 */
int image_save_jpg(const char *filename, Image *img, int quality) {
    if (!img || !img->data) {
        fprintf(stderr, "エラー: 無効な画像データ\n");
        return 0;
    }
    
    /* stb_image_writeでJPEG保存 */
    int result = stbi_write_jpg(filename, img->width, img->height, 
                                 img->channels, img->data, quality);
    
    if (result) {
        printf("画像保存成功: %s\n", filename);
    } else {
        fprintf(stderr, "エラー: 画像保存失敗: %s\n", filename);
    }
    
    return result;
}

/* 画像をPNGファイルとして保存 */
int image_save_png(const char *filename, Image *img) {
    if (!img || !img->data) {
        fprintf(stderr, "エラー: 無効な画像データ\n");
        return 0;
    }
    
    /* stb_image_writeでPNG保存 */
    int stride = img->width * img->channels;
    int result = stbi_write_png(filename, img->width, img->height, 
                                 img->channels, img->data, stride);
    
    if (result) {
        printf("画像保存成功: %s\n", filename);
    } else {
        fprintf(stderr, "エラー: 画像保存失敗: %s\n", filename);
    }
    
    return result;
}

/* メモリ解放 */
void image_free(Image *img) {
    if (img) {
        if (img->data) {
            stbi_image_free(img->data);
        }
        free(img);
    }
}


/* ===========================
 * 画像の生成
 * =========================== */

/* 空の画像を作成 */
Image* image_create(int width, int height, int channels) {
    Image *img = (Image*)malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "エラー: メモリ確保失敗\n");
        return NULL;
    }
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    
    /* 画素データのメモリ確保（0で初期化） */
    size_t data_size = width * height * channels;
    img->data = (uint8_t*)calloc(data_size, sizeof(uint8_t));
    
    if (!img->data) {
        fprintf(stderr, "エラー: 画像データのメモリ確保失敗\n");
        free(img);
        return NULL;
    }
    
    return img;
}

/* 既存の画像と同じサイズの空画像を作成 */
Image* image_create_like(Image *src) {
    if (!src) return NULL;
    return image_create(src->width, src->height, src->channels);
}

/* Gaussian blur を適用した画像を生成 */
Image* image_gaussian_blur(Image *src, int ksize, double sigma) {
    if (!src || !src->data) {
        fprintf(stderr, "エラー: Gaussian blur の入力画像が無効です\n");
        return NULL;
    }
    if (ksize <= 0 || (ksize % 2) == 0) {
        fprintf(stderr, "エラー: ksize は奇数で指定してください\n");
        return NULL;
    }
    if (sigma <= 0.0) {
        sigma = 1.0;
    }

    Image *dst = image_create(src->width, src->height, src->channels);
    if (!dst) {
        return NULL;
    }

    int radius = ksize / 2;
    double *kernel = (double*)malloc((size_t)ksize * sizeof(double));
    if (!kernel) {
        fprintf(stderr, "エラー: Gaussian kernel のメモリ確保に失敗しました\n");
        image_free(dst);
        return NULL;
    }

    double sum = 0.0;
    for (int i = -radius; i <= radius; i++) {
        kernel[i + radius] = exp(-(double)(i * i) / (2.0 * sigma * sigma));
        sum += kernel[i + radius];
    }
    for (int i = 0; i < ksize; i++) {
        kernel[i] /= sum;
    }

    for (int v = 0; v < src->height; v++) {
        for (int u = 0; u < src->width; u++) {
            double accum[4] = {0.0, 0.0, 0.0, 0.0};

            for (int ky = -radius; ky <= radius; ky++) {
                int yy = v + ky;
                if (yy < 0) yy = 0;
                if (yy >= src->height) yy = src->height - 1;

                for (int kx = -radius; kx <= radius; kx++) {
                    int xx = u + kx;
                    if (xx < 0) xx = 0;
                    if (xx >= src->width) xx = src->width - 1;

                    int idx = (yy * src->width + xx) * src->channels;
                    double w = kernel[ky + radius] * kernel[kx + radius];

                    for (int c = 0; c < src->channels; c++) {
                        accum[c] += (double)src->data[idx + c] * w;
                    }
                }
            }

            int out_idx = (v * src->width + u) * src->channels;
            for (int c = 0; c < src->channels; c++) {
                double val = accum[c];
                if (val < 0.0) val = 0.0;
                if (val > 255.0) val = 255.0;
                dst->data[out_idx + c] = (uint8_t)(val + 0.5);
            }
        }
    }

    free(kernel);
    return dst;
}

Image* image_difference(Image *a, Image *b)
{
    if (!a || !b) return NULL;
    if (a->width != b->width || a->height != b->height || a->channels != b->channels) return NULL;

    Image *dst = image_create_like(a);
    if (!dst) return NULL;

    int w = a->width;
    int h = a->height;
    int c = a->channels;
    for (int v = 0; v < h; ++v) {
        for (int u = 0; u < w; ++u) {
            for (int ch = 0; ch < c; ++ch) {
                int idx = (v * w + u) * c + ch;
                int va = a->data[idx];
                int vb = b->data[idx];
                int diff = va - vb;
                if (diff < 0) diff = -diff;
                if (diff > 255) diff = 255;
                dst->data[idx] = (uint8_t)diff;
            }
        }
    }

    return dst;
}

/* ===========================
 * 画素値の取得・設定
 * =========================== */

/* 【修正版】画素値を取得（周期境界条件対応）
 * 
 * 全方位画像は水平方向(u方向)が周期的
 * - u座標: 周期境界（左端と右端がつながる）
 * - v座標: 通常の境界（上下は範囲外で0）
 */
void get_pixel(Image *img, int u, int v, uint8_t *rgb) {
    /* v座標の範囲チェック（上下は周期的でない） */
    if (v < 0 || v >= img->height) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }
    
    /* 【修正】u座標は周期境界条件を適用 */
    while (u < 0) u += img->width;
    while (u >= img->width) u -= img->width;
    
    /* データ配列のインデックス計算 */
    int index = (v * img->width + u) * img->channels;
    
    /* RGB値をコピー */
    rgb[0] = img->data[index + 0];
    rgb[1] = img->data[index + 1];
    rgb[2] = img->data[index + 2];
}

/* 画素値を設定 */
void set_pixel(Image *img, int u, int v, const uint8_t *rgb) {
    /* 範囲チェック */
    if (u < 0 || u >= img->width || v < 0 || v >= img->height) {
        return;
    }
    
    /* データ配列のインデックス計算 */
    int index = (v * img->width + u) * img->channels;
    
    /* RGB値を設定 */
    img->data[index + 0] = rgb[0];
    img->data[index + 1] = rgb[1];
    img->data[index + 2] = rgb[2];
}

/* バイリニア補間で画素値を取得 */
void get_pixel_bilinear(Image *img, double u, double v, uint8_t *rgb) {
    /* 整数部と小数部に分解 */
    int u0 = (int)floor(u);
    int v0 = (int)floor(v);
    int u1 = u0 + 1;
    int v1 = v0 + 1;
    
    double du = u - u0;
    double dv = v - v0;
    
    /* 4つの近傍画素を取得 */
    uint8_t p00[3], p01[3], p10[3], p11[3];
    get_pixel(img, u0, v0, p00);
    get_pixel(img, u0, v1, p01);
    get_pixel(img, u1, v0, p10);
    get_pixel(img, u1, v1, p11);
    
    /* バイリニア補間 */
    for (int c = 0; c < 3; c++) {
        double val = (1.0 - du) * (1.0 - dv) * p00[c]
                   + (1.0 - du) * dv         * p01[c]
                   + du         * (1.0 - dv) * p10[c]
                   + du         * dv         * p11[c];
        
        /* クリッピング */
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        
        rgb[c] = (uint8_t)val;
    }
}


/* ===========================
 * デバッグ用
 * =========================== */

/* 画像情報を表示 */
void image_info(Image *img) {
    if (!img) {
        printf("画像: NULL\n");
        return;
    }
    
    printf("画像情報:\n");
    printf("  サイズ: %d × %d\n", img->width, img->height);
    printf("  チャンネル数: %d\n", img->channels);
    printf("  データサイズ: %ld bytes\n", 
           (long)(img->width * img->height * img->channels));
    
    /* 先頭画素の値を表示 */
    if (img->data) {
        printf("  先頭画素(0,0): RGB(%d, %d, %d)\n",
               img->data[0], img->data[1], img->data[2]);
    }
}