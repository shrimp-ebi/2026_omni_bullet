/* test_gradient.c
 * 解析微分と数値微分の一致を検証するテスト
 *
 * compute_gradient_3param() と compute_numerical_gradient_3param() を
 * 複数の回転ケースで比較し、相対誤差が 5% 未満であることを確認する。
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DELTA       1e-4   /* 数値微分のステップ（ラジアン） */
#define ERROR_LIMIT 0.05   /* 相対誤差の許容上限（5%）       */

/* テスト1ケース分を実行し、OK 数を返す */
static int run_case(const char *label,
                    Image *base, Image *ref,
                    double omega[3],
                    int u_min, int v_min, int u_max, int v_max)
{
    printf("=== %s ===\n", label);

    /* 回転行列を生成 */
    Matrix3x3 R = rodrigues(omega);

    /* 解析微分 */
    double grad_a[3];
    compute_gradient_3param(base, ref, R, u_min, v_min, u_max, v_max, grad_a);

    /* 数値微分 */
    double grad_n[3];
    for (int k = 0; k < 3; k++) {
        grad_n[k] = compute_numerical_gradient_3param(
            base, ref, R, k, DELTA, u_min, v_min, u_max, v_max);
    }

    /* 比較・表示 */
    int ok_count = 0;
    for (int k = 0; k < 3; k++) {
        double err = fabs(grad_a[k] - grad_n[k]) / (fabs(grad_n[k]) + 1e-10);
        const char *result = (err < ERROR_LIMIT) ? "OK" : "NG";
        if (err < ERROR_LIMIT) ok_count++;
        printf("  grad[%d]  解析: %12.6f  数値: %12.6f  相対誤差: %5.2f%%  [%s]\n",
               k, grad_a[k], grad_n[k], err * 100.0, result);
    }
    printf("\n");

    return ok_count;
}

int main(void)
{
    const char *base_path = "images/base/base.jpg";
    const char *ref_path  = "images/reference/reference_30deg.jpg";

    /* 画像の読み込み */
    Image *base = image_load(base_path);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません: %s\n", base_path);
        return 1;
    }
    Image *ref = image_load(ref_path);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません: %s\n", ref_path);
        image_free(base);
        return 1;
    }

    int W = base->width;
    int H = base->height;
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    printf("画像サイズ: %d x %d\n", W, H);
    printf("比較領域: u=[%d,%d]  v=[%d,%d]\n\n", u_min, u_max, v_min, v_max);

    int total_ok = 0;

    /* ケース1: Y軸のみ */
    {
        double omega[3] = {0.0, 30.0 * M_PI / 180.0, 0.0};
        total_ok += run_case(
            "テストケース1: Y軸のみ (omega=[0, 30, 0]deg)",
            base, ref, omega, u_min, v_min, u_max, v_max);
    }

    /* ケース2: X軸のみ */
    {
        double omega[3] = {30.0 * M_PI / 180.0, 0.0, 0.0};
        total_ok += run_case(
            "テストケース2: X軸のみ (omega=[30, 0, 0]deg)",
            base, ref, omega, u_min, v_min, u_max, v_max);
    }

    /* ケース3: Z軸のみ */
    {
        double omega[3] = {0.0, 0.0, 30.0 * M_PI / 180.0};
        total_ok += run_case(
            "テストケース3: Z軸のみ (omega=[0, 0, 30]deg)",
            base, ref, omega, u_min, v_min, u_max, v_max);
    }

    /* ケース4: 3軸同時 */
    {
        double omega[3] = {
            15.0 * M_PI / 180.0,
            30.0 * M_PI / 180.0,
            10.0 * M_PI / 180.0
        };
        total_ok += run_case(
            "テストケース4: 3軸同時 (omega=[15, 30, 10]deg)",
            base, ref, omega, u_min, v_min, u_max, v_max);
    }

    /* サマリ */
    printf("=== サマリ ===\n");
    printf("  合計: %d / 12 テストがOK\n", total_ok);

    image_free(base);
    image_free(ref);

    return (total_ok == 12) ? 0 : 1;
}
