/* image_utils.c
 * 画像処理ユーティリティの実装
 * 
 * stb_imageを使用
 */

#include "image_utils.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* stb_imageの実装を有効化 */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


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


/* ===========================
 * 画素値の取得・設定
 * =========================== */

/* 画素値を取得 */
void get_pixel(Image *img, int u, int v, uint8_t *rgb) {
    /* 範囲チェック */
    if (u < 0 || u >= img->width || v < 0 || v >= img->height) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return;
    }
    
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
