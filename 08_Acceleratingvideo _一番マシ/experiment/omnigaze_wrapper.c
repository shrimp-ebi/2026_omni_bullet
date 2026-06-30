#include "omnigaze.h"
#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "../include/vector_math.h"
#include "objective_3param.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lm_estimate_frame_file(const char *base_path, const char *ref_path,
                          const double R_init[9], double sigma,
                          double R_out[9])
{
    Matrix3x3 R_init_mat;
    R_init_mat.m[0][0] = R_init[0]; R_init_mat.m[0][1] = R_init[1]; R_init_mat.m[0][2] = R_init[2];
    R_init_mat.m[1][0] = R_init[3]; R_init_mat.m[1][1] = R_init[4]; R_init_mat.m[1][2] = R_init[5];
    R_init_mat.m[2][0] = R_init[6]; R_init_mat.m[2][1] = R_init[7]; R_init_mat.m[2][2] = R_init[8];

    Image *base = image_load(base_path);
    if (!base) return 1;
    Image *ref  = image_load(ref_path);
    if (!ref)  { image_free(base); return 1; }

    Image *ref_blur = image_gaussian_blur(ref, 5, sigma);
    if (!ref_blur) { image_free(base); image_free(ref); return 1; }

    int W = base->width;
    int H = base->height;
    /* 修正版と同じ比率スケール比較窓 (380×190 @ 6080×3040 相当) */
    int cw = (int)(W * 380.0 / 6080.0 / 2.0);
    int ch = (int)(H * 190.0 / 3040.0 / 2.0);
    if (cw < 32) cw = 32;
    if (ch < 16) ch = 16;
    int u_min = W / 2 - cw / 2, u_max = W / 2 + cw / 2;
    int v_min = H / 2 - ch / 2, v_max = H / 2 + ch / 2;
    if (u_min < 0) u_min = 0;
    if (v_min < 0) v_min = 0;
    if (u_max >= W) u_max = W - 1;
    if (v_max >= H) v_max = H - 1;

    Matrix3x3 R = R_init_mat;
    double C = 0.0001;
    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);

    /* LM 進行ログ用 */
    double E_initial = E;
    int converged = 0;
    int solve_fail = 0;
    int iter;
    for (iter = 0; iter < 300; iter++) {
        double grad[3];
        compute_gradient_3param(base, ref_blur, R, u_min, v_min, u_max, v_max, grad);
        double hessian[3][3];
        compute_hessian_3param(base, ref_blur, R, u_min, v_min, u_max, v_max, hessian);

        double A[3][3], b[3];
        for (int k = 0; k < 3; k++) {
            for (int l = 0; l < 3; l++) A[k][l] = hessian[k][l];
            b[k] = -grad[k];
        }
        A[0][0] *= (1.0 + C);
        A[1][1] *= (1.0 + C);
        A[2][2] *= (1.0 + C);

        double delta_omega[3];
        if (solve_linear_3x3(A, b, delta_omega) != 0) {
            C *= 10.0;
            if (++solve_fail >= 30)
                break;
            continue;
        }
        solve_fail = 0;

        Matrix3x3 dR = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);
        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        if (E_new < E) {
            R = R_new; E = E_new; C /= 10.0;
        } else {
            C *= 10.0;
        }

        if (norm_dw < 1e-10) { converged = 1; break; }
    }

    /* LM 結果ログ: 反復回数・初期/最終目的関数・差分・収束フラグ */
    {
        double E_final = E;
        double dE = E_initial - E_final;
        /* iters    : LM反復回数
         * E_init   : 最適化前の目的関数値 E = (1/2N)Σ(Sr-Sb)²
         * E_final  : 最適化後の目的関数値
         * dE       : 目的関数の減少量 (E_init - E_final)
         * converged: 1=収束(‖Δω‖<1e-8), 0=最大反復(500回)到達 */
        printf("  [LM] iters=%-3d  E_init=%.6f  E_final=%.6f  dE=%.2e  converged=%d\n",
               iter, E_initial, E_final, dE, converged);
    }

    /* 出力 */
    R_out[0] = R.m[0][0]; R_out[1] = R.m[0][1]; R_out[2] = R.m[0][2];
    R_out[3] = R.m[1][0]; R_out[4] = R.m[1][1]; R_out[5] = R.m[1][2];
    R_out[6] = R.m[2][0]; R_out[7] = R.m[2][1]; R_out[8] = R.m[2][2];

    image_free(ref_blur);
    image_free(base);
    image_free(ref);
    return 0;
}

static Matrix3x3 build_R_gaze(Vector3D G)
{
    Vector3D ez = vector_normalize(G);
    Vector3D up = vector_create(0.0, 1.0, 0.0);
    Vector3D ex_raw = vector_cross(up, ez);
    if (vector_norm(ex_raw) < 1e-10) {
        Vector3D alt = vector_create(1.0, 0.0, 0.0);
        ex_raw = vector_cross(alt, ez);
    }
    Vector3D ex = vector_normalize(ex_raw);
    Vector3D ey = vector_normalize(vector_cross(ez, ex));
    Matrix3x3 R;
    R.m[0][0] = ex.x;  R.m[0][1] = ex.y;  R.m[0][2] = ex.z;
    R.m[1][0] = ey.x;  R.m[1][1] = ey.y;  R.m[1][2] = ey.z;
    R.m[2][0] = ez.x;  R.m[2][1] = ez.y;  R.m[2][2] = ez.z;
    return R;
}

int generate_gaze_frame_file(const char *in_path, const char *out_path,
                             int u_g, int v_g, const double R_cumulative[9])
{
    Matrix3x3 R_cum;
    R_cum.m[0][0] = R_cumulative[0]; R_cum.m[0][1] = R_cumulative[1]; R_cum.m[0][2] = R_cumulative[2];
    R_cum.m[1][0] = R_cumulative[3]; R_cum.m[1][1] = R_cumulative[4]; R_cum.m[1][2] = R_cumulative[5];
    R_cum.m[2][0] = R_cumulative[6]; R_cum.m[2][1] = R_cumulative[7]; R_cum.m[2][2] = R_cumulative[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width; int H = img->height;

    Vector3D G = image_to_world(u_g, v_g, W, H);
    Matrix3x3 R_gaze = build_R_gaze(G);
    Matrix3x3 R_gaze_T = matrix_transpose(R_gaze);

    int u_min = W / 4; int u_max = 3 * W / 4;
    int v_min = H / 4; int v_max = 3 * H / 4;
    int W_out = u_max - u_min; int H_out = v_max - v_min;

    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = v_min; v < v_max; v++) {
        for (int u = u_min; u < u_max; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X_ib = matrix_vector_multiply(R_gaze_T, Xp);
            Vector3D X = matrix_vector_multiply(R_cum, X_ib);
            double u_in, v_in; world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3]; get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u - u_min, v - v_min, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out);
    image_free(img);
    return ok ? 0 : 1;
}

int generate_gaze_frame_world_file(const char *in_path, const char *out_path,
                                   double Gx, double Gy, double Gz,
                                   const double R_cumulative[9])
{
    Matrix3x3 R_cum;
    R_cum.m[0][0] = R_cumulative[0]; R_cum.m[0][1] = R_cumulative[1]; R_cum.m[0][2] = R_cumulative[2];
    R_cum.m[1][0] = R_cumulative[3]; R_cum.m[1][1] = R_cumulative[4]; R_cum.m[1][2] = R_cumulative[5];
    R_cum.m[2][0] = R_cumulative[6]; R_cum.m[2][1] = R_cumulative[7]; R_cum.m[2][2] = R_cumulative[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width; int H = img->height;

    Vector3D G = vector_create(Gx, Gy, Gz);
    Matrix3x3 R_gaze = build_R_gaze(G);
    Matrix3x3 R_gaze_T = matrix_transpose(R_gaze);

    int u_min = W / 4; int u_max = 3 * W / 4;
    int v_min = H / 4; int v_max = 3 * H / 4;
    int W_out = u_max - u_min; int H_out = v_max - v_min;

    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = v_min; v < v_max; v++) {
        for (int u = u_min; u < u_max; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X_ib = matrix_vector_multiply(R_gaze_T, Xp);
            Vector3D X = matrix_vector_multiply(R_cum, X_ib);
            double u_in, v_in; world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3]; get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u - u_min, v - v_min, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out);
    image_free(img);
    return ok ? 0 : 1;
}

/* ============================================================
 * 2点方式 API
 * ============================================================ */

/* 注視点(u_gaze,v_gaze)と補助点(u_ref,v_ref)の画像座標から
 * 初期回転行列 R_0 を計算して R_out[9](row-major)に格納する。
 *
 * 論文式:
 *   ez = normalize(G)
 *   ey = normalize(G × Gs)   ← この外積の順序を厳守
 *   ex = ey × ez
 *   R_0 の各列に [ex | ey | ez] を配置
 */
int calc_R_2points_file(const char *img_path,
                        int u_gaze, int v_gaze,
                        int u_ref,  int v_ref,
                        double R_out[9])
{
    Image *img = image_load(img_path);
    if (!img) return 1;
    int W = img->width, H = img->height;
    image_free(img);

    Vector3D G  = image_to_world(u_gaze, v_gaze, W, H);
    Vector3D Gs = image_to_world(u_ref,  v_ref,  W, H);

    Vector3D ez = vector_normalize(G);
    Vector3D ey_raw = vector_cross(G, Gs);   /* G × Gs の順 */
    if (vector_norm(ey_raw) < 1e-10) return 1; /* G と Gs が平行 */
    Vector3D ey = vector_normalize(ey_raw);
    Vector3D ex = vector_cross(ey, ez);      /* ey × ez */

    /* R_0 は列に [ex | ey | ez] を配置: R_0[i][j] = {ex,ey,ez}[j][i] */
    R_out[0] = ex.x;  R_out[1] = ey.x;  R_out[2] = ez.x;
    R_out[3] = ex.y;  R_out[4] = ey.y;  R_out[5] = ez.y;
    R_out[6] = ex.z;  R_out[7] = ey.z;  R_out[8] = ez.z;
    return 0;
}

/* 2点方式用フルサイズ出力 (Ib 生成専用)。
 * R_0_param[9]: calc_R_2points_file が返す初期回転行列(row-major)。
 * R_cum が単位行列のとき、出力の中心が注視点 G 方向になる。
 *
 * 逆マッピング:
 *   X_ib  = R_0 × Xp          (出力方向を Ib 座標へ)
 *   X     = R_cum × X_ib      (Ib 座標を現フレームへ)
 */
int generate_gaze_full_R0_file(const char *in_path, const char *out_path,
                                const double R_0_param[9],
                                const double R_cumulative[9])
{
    Matrix3x3 R_0;
    R_0.m[0][0]=R_0_param[0]; R_0.m[0][1]=R_0_param[1]; R_0.m[0][2]=R_0_param[2];
    R_0.m[1][0]=R_0_param[3]; R_0.m[1][1]=R_0_param[4]; R_0.m[1][2]=R_0_param[5];
    R_0.m[2][0]=R_0_param[6]; R_0.m[2][1]=R_0_param[7]; R_0.m[2][2]=R_0_param[8];

    Matrix3x3 R_cum;
    R_cum.m[0][0]=R_cumulative[0]; R_cum.m[0][1]=R_cumulative[1]; R_cum.m[0][2]=R_cumulative[2];
    R_cum.m[1][0]=R_cumulative[3]; R_cum.m[1][1]=R_cumulative[4]; R_cum.m[1][2]=R_cumulative[5];
    R_cum.m[2][0]=R_cumulative[6]; R_cum.m[2][1]=R_cumulative[7]; R_cum.m[2][2]=R_cumulative[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width, H = img->height;

    Image *out = image_create(W, H, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = 0; v < H; v++) {
        for (int u = 0; u < W; u++) {
            Vector3D Xp   = image_to_world(u, v, W, H);
            Vector3D X_ib = matrix_vector_multiply(R_0,   Xp);
            Vector3D X    = matrix_vector_multiply(R_cum,  X_ib);
            double u_in, v_in; world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3]; get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u, v, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out); image_free(img);
    return ok ? 0 : 1;
}

/* 2点方式用クリップ出力 (各フレーム用)。
 * 逆マッピングは generate_gaze_full_R0_file と同一、切り出し範囲のみ異なる。
 */
int generate_gaze_frame_R0_file(const char *in_path, const char *out_path,
                                 const double R_0_param[9],
                                 const double R_cumulative[9])
{
    Matrix3x3 R_0;
    R_0.m[0][0]=R_0_param[0]; R_0.m[0][1]=R_0_param[1]; R_0.m[0][2]=R_0_param[2];
    R_0.m[1][0]=R_0_param[3]; R_0.m[1][1]=R_0_param[4]; R_0.m[1][2]=R_0_param[5];
    R_0.m[2][0]=R_0_param[6]; R_0.m[2][1]=R_0_param[7]; R_0.m[2][2]=R_0_param[8];

    Matrix3x3 R_cum;
    R_cum.m[0][0]=R_cumulative[0]; R_cum.m[0][1]=R_cumulative[1]; R_cum.m[0][2]=R_cumulative[2];
    R_cum.m[1][0]=R_cumulative[3]; R_cum.m[1][1]=R_cumulative[4]; R_cum.m[1][2]=R_cumulative[5];
    R_cum.m[2][0]=R_cumulative[6]; R_cum.m[2][1]=R_cumulative[7]; R_cum.m[2][2]=R_cumulative[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width, H = img->height;

    int u_min = W / 4, u_max = 3 * W / 4;
    int v_min = H / 4, v_max = 3 * H / 4;
    int W_out = u_max - u_min, H_out = v_max - v_min;

    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = v_min; v < v_max; v++) {
        for (int u = u_min; u < u_max; u++) {
            Vector3D Xp   = image_to_world(u, v, W, H);
            Vector3D X_ib = matrix_vector_multiply(R_0,   Xp);
            Vector3D X    = matrix_vector_multiply(R_cum,  X_ib);
            double u_in, v_in; world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3]; get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u - u_min, v - v_min, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out); image_free(img);
    return ok ? 0 : 1;
}

/* ============================================================
 * 逐次追跡型 API（修正版 lm_video.c と同等）
 * ============================================================ */

/* 注視点 G の世界座標から初期回転行列 R_0 を計算する。
 * R_0 は列に [ex, ey, ez] を格納 → R_0 · e_z = ez = G が成立。
 * 逐次追跡の第 0 フレームで使用する。
 */
int get_initial_R_file(double Gx, double Gy, double Gz, double R_out[9])
{
    Vector3D G = vector_create(Gx, Gy, Gz);
    Vector3D ez = vector_normalize(G);
    Vector3D up = vector_create(0.0, 1.0, 0.0);
    Vector3D ex_raw = vector_cross(up, ez);
    if (vector_norm(ex_raw) < 1e-10) {
        Vector3D alt = vector_create(1.0, 0.0, 0.0);
        ex_raw = vector_cross(alt, ez);
    }
    Vector3D ex = vector_normalize(ex_raw);
    Vector3D ey = vector_normalize(vector_cross(ez, ex));

    /* 列に [ex, ey, ez] を配置 (修正版 build_initial_rotation と同一規約) */
    R_out[0] = ex.x;  R_out[1] = ey.x;  R_out[2] = ez.x;
    R_out[3] = ex.y;  R_out[4] = ey.y;  R_out[5] = ez.y;
    R_out[6] = ex.z;  R_out[7] = ey.z;  R_out[8] = ez.z;
    return 0;
}

/* 逐次追跡用の注視画像生成（修正版 generate_gaze_frame と同等）。
 * 逆マッピング: X = R · Xp （R · e_z = G のとき中心が注視点を参照）
 * フルサイズ W×H を出力する。
 * R[9]: row-major で列に [ex, ey, ez] を格納した回転行列。
 */
int generate_gaze_sequential_file(const char *in_path, const char *out_path,
                                   const double R[9])
{
    Matrix3x3 Rmat;
    Rmat.m[0][0] = R[0]; Rmat.m[0][1] = R[1]; Rmat.m[0][2] = R[2];
    Rmat.m[1][0] = R[3]; Rmat.m[1][1] = R[4]; Rmat.m[1][2] = R[5];
    Rmat.m[2][0] = R[6]; Rmat.m[2][1] = R[7]; Rmat.m[2][2] = R[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width, H = img->height;

    Image *out = image_create(W, H, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = 0; v < H; v++) {
        for (int u = 0; u < W; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X  = matrix_vector_multiply(Rmat, Xp);
            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3];
            get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u, v, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out);
    image_free(img);
    return ok ? 0 : 1;
}

/* generate_gaze_frame_world_file のフルサイズ版。
 * 切り出しをせず W×H 全画素を出力する。回転計算は完全に同一。
 * LM 推定の基準画像 Ib 生成専用。 */
int generate_gaze_full_world_file(const char *in_path, const char *out_path,
                                   double Gx, double Gy, double Gz,
                                   const double R_cumulative[9])
{
    Matrix3x3 R_cum;
    R_cum.m[0][0] = R_cumulative[0]; R_cum.m[0][1] = R_cumulative[1]; R_cum.m[0][2] = R_cumulative[2];
    R_cum.m[1][0] = R_cumulative[3]; R_cum.m[1][1] = R_cumulative[4]; R_cum.m[1][2] = R_cumulative[5];
    R_cum.m[2][0] = R_cumulative[6]; R_cum.m[2][1] = R_cumulative[7]; R_cum.m[2][2] = R_cumulative[8];

    Image *img = image_load(in_path);
    if (!img) return 1;
    int W = img->width; int H = img->height;

    Vector3D G = vector_create(Gx, Gy, Gz);
    Matrix3x3 R_gaze = build_R_gaze(G);
    Matrix3x3 R_total = matrix_multiply(R_gaze, R_cum);
    Matrix3x3 R_T = matrix_transpose(R_total);

    Image *out = image_create(W, H, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = 0; v < H; v++) {
        for (int u = 0; u < W; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X = matrix_vector_multiply(R_T, Xp);
            double u_in, v_in; world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3]; get_pixel_bilinear(img, u_in, v_in, rgb);
            set_pixel(out, u, v, rgb);
        }
    }

    int ok = image_save_jpg(out_path, out, 95);
    image_free(out);
    image_free(img);
    return ok ? 0 : 1;
}
