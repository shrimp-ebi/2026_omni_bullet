/* scan_gradient_1d.c
 * 勾配の1次元断面スキャン
 *
 * ケースA（base.jpg / ref_w1_15_w2_30.jpg）を使用
 * 真値: ω₁=15°, ω₂=30°, ω₃=0°（固定）
 *
 * スライス1: ω₂=30°固定、ω₁を5°〜25°で0.1°刻み → grad_1d_omega1.csv
 * スライス2: ω₁=15°固定、ω₂を20°〜40°で0.1°刻み → grad_1d_omega2.csv
 *
 * 勾配は中心差分 (h=0.01°) で計算:
 *   ∂E/∂ω₁ ≈ (E(ω₁+h, ω₂) - E(ω₁-h, ω₂)) / (2h)
 *   ∂E/∂ω₂ ≈ (E(ω₁, ω₂+h) - E(ω₁, ω₂-h)) / (2h)
 *   |∇E| = sqrt((∂E/∂ω₁)² + (∂E/∂ω₂)²)
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

static const char *BASE_PATH = "images/base/base.jpg";
static const char *REF_PATH  = "images/reference/ref_w1_15_w2_30.jpg";

static const double H_DEG = 0.01;   /* 中心差分の刻み幅 [度] */

/* E(omega1_deg, omega2_deg) を計算する */
static double eval_E(Image *base, Image *ref,
                     double w1_deg, double w2_deg,
                     int u_min, int v_min, int u_max, int v_max)
{
    double omega[3] = {
        -(w1_deg * M_PI / 180.0),
        -(w2_deg * M_PI / 180.0),
        0.0
    };
    Matrix3x3 R = rodrigues(omega);
    return compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);
}

int main(void)
{
    Image *base = image_load(BASE_PATH);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません: %s\n", BASE_PATH);
        return 1;
    }
    Image *ref = image_load(REF_PATH);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません: %s\n", REF_PATH);
        image_free(base);
        return 1;
    }

    int W = base->width,  H = base->height;
    int u_min = W / 4, u_max = 3 * W / 4;
    int v_min = H / 4, v_max = 3 * H / 4;

    printf("画像サイズ: %d x %d\n", W, H);
    printf("比較領域: u=[%d,%d]  v=[%d,%d]\n", u_min, u_max, v_min, v_max);
    printf("h = %.3f°\n\n", H_DEG);

    /* ── スライス1: ω₂=30°固定、ω₁ = 5°〜25° ─────────────────────────── */
    {
        const double W2_FIXED = 30.0;
        const double W1_MIN   =  5.0;
        const double W1_MAX   = 25.0;
        const double STEP     =  0.1;

        FILE *fp = fopen("results/grad_1d_omega1.csv", "w");
        if (!fp) { fprintf(stderr, "エラー: grad_1d_omega1.csv を開けません\n"); goto cleanup; }
        fprintf(fp, "omega1_deg,E,dE_dw1,dE_dw2,grad_norm\n");

        int n = (int)((W1_MAX - W1_MIN) / STEP) + 1;
        printf("スライス1: ω₂=%.1f°固定、ω₁ を %d 点スキャン中...\n", W2_FIXED, n);

        for (int i = 0; i < n; i++) {
            double w1 = W1_MIN + STEP * i;

            double E      = eval_E(base, ref, w1,       W2_FIXED, u_min, v_min, u_max, v_max);
            double E_p1   = eval_E(base, ref, w1 + H_DEG, W2_FIXED, u_min, v_min, u_max, v_max);
            double E_m1   = eval_E(base, ref, w1 - H_DEG, W2_FIXED, u_min, v_min, u_max, v_max);
            double E_p2   = eval_E(base, ref, w1,       W2_FIXED + H_DEG, u_min, v_min, u_max, v_max);
            double E_m2   = eval_E(base, ref, w1,       W2_FIXED - H_DEG, u_min, v_min, u_max, v_max);

            double dEdw1 = (E_p1 - E_m1) / (2.0 * H_DEG * M_PI / 180.0);
            double dEdw2 = (E_p2 - E_m2) / (2.0 * H_DEG * M_PI / 180.0);
            double grad  = sqrt(dEdw1 * dEdw1 + dEdw2 * dEdw2);

            fprintf(fp, "%.1f,%.10f,%.6f,%.6f,%.6f\n", w1, E, dEdw1, dEdw2, grad);

            if ((i + 1) % 50 == 0 || i + 1 == n)
                printf("  %d/%d  (ω₁=%.1f°, |∇E|=%.4f)\n", i + 1, n, w1, grad);
        }
        fclose(fp);
        printf("保存: results/grad_1d_omega1.csv\n\n");
    }

    /* ── スライス2: ω₁=15°固定、ω₂ = 20°〜40° ─────────────────────────── */
    {
        const double W1_FIXED = 15.0;
        const double W2_MIN   = 20.0;
        const double W2_MAX   = 40.0;
        const double STEP     =  0.1;

        FILE *fp = fopen("results/grad_1d_omega2.csv", "w");
        if (!fp) { fprintf(stderr, "エラー: grad_1d_omega2.csv を開けません\n"); goto cleanup; }
        fprintf(fp, "omega2_deg,E,dE_dw1,dE_dw2,grad_norm\n");

        int n = (int)((W2_MAX - W2_MIN) / STEP) + 1;
        printf("スライス2: ω₁=%.1f°固定、ω₂ を %d 点スキャン中...\n", W1_FIXED, n);

        for (int i = 0; i < n; i++) {
            double w2 = W2_MIN + STEP * i;

            double E      = eval_E(base, ref, W1_FIXED,       w2, u_min, v_min, u_max, v_max);
            double E_p1   = eval_E(base, ref, W1_FIXED + H_DEG, w2, u_min, v_min, u_max, v_max);
            double E_m1   = eval_E(base, ref, W1_FIXED - H_DEG, w2, u_min, v_min, u_max, v_max);
            double E_p2   = eval_E(base, ref, W1_FIXED,       w2 + H_DEG, u_min, v_min, u_max, v_max);
            double E_m2   = eval_E(base, ref, W1_FIXED,       w2 - H_DEG, u_min, v_min, u_max, v_max);

            double dEdw1 = (E_p1 - E_m1) / (2.0 * H_DEG * M_PI / 180.0);
            double dEdw2 = (E_p2 - E_m2) / (2.0 * H_DEG * M_PI / 180.0);
            double grad  = sqrt(dEdw1 * dEdw1 + dEdw2 * dEdw2);

            fprintf(fp, "%.1f,%.10f,%.6f,%.6f,%.6f\n", w2, E, dEdw1, dEdw2, grad);

            if ((i + 1) % 50 == 0 || i + 1 == n)
                printf("  %d/%d  (ω₂=%.1f°, |∇E|=%.4f)\n", i + 1, n, w2, grad);
        }
        fclose(fp);
        printf("保存: results/grad_1d_omega2.csv\n\n");
    }

    image_free(base);
    image_free(ref);
    return 0;

cleanup:
    image_free(base);
    image_free(ref);
    return 1;
}
