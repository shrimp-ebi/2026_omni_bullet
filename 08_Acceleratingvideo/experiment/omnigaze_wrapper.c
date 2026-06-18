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
    int u_min = W / 4,  u_max = 3 * W / 4;
    int v_min = H / 4,  v_max = 3 * H / 4;

    Matrix3x3 R = R_init_mat;
    double C = 0.0001;
    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);

    /* LM 進行ログ用 */
    double E_initial = E;
    int converged = 0;
    int iter;
    for (iter = 0; iter < 500; iter++) {
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
            C *= 10.0; continue;
        }

        Matrix3x3 dR = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);
        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0]*delta_omega[0]
                            + delta_omega[1]*delta_omega[1]
                            + delta_omega[2]*delta_omega[2]);

        if (E_new < E || fabs(E_new - E) < 1e-6) {
            R = R_new; E = E_new; C /= 10.0;
        } else {
            C *= 10.0;
        }

        if (norm_dw < 1e-8) { converged = 1; break; }
    }

    /* LM 結果ログ: 反復回数・初期/最終目的関数・差分・収束フラグ */
    {
        double E_final = E;
        double dE = E_initial - E_final;
        fprintf(stderr, "[LM] iters=%d E_init=%.6f E_final=%.6f dE=%.6f converged=%d\n",
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
    Matrix3x3 R_total = matrix_multiply(R_gaze, R_cum);
    Matrix3x3 R_T = matrix_transpose(R_total);

    int u_min = W / 4; int u_max = 3 * W / 4;
    int v_min = H / 4; int v_max = 3 * H / 4;
    int W_out = u_max - u_min; int H_out = v_max - v_min;

    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = v_min; v < v_max; v++) {
        for (int u = u_min; u < u_max; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X = matrix_vector_multiply(R_T, Xp);
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
    Matrix3x3 R_total = matrix_multiply(R_gaze, R_cum);
    Matrix3x3 R_T = matrix_transpose(R_total);

    int u_min = W / 4; int u_max = 3 * W / 4;
    int v_min = H / 4; int v_max = 3 * H / 4;
    int W_out = u_max - u_min; int H_out = v_max - v_min;

    Image *out = image_create(W_out, H_out, img->channels);
    if (!out) { image_free(img); return 1; }

    for (int v = v_min; v < v_max; v++) {
        for (int u = u_min; u < u_max; u++) {
            Vector3D Xp = image_to_world(u, v, W, H);
            Vector3D X = matrix_vector_multiply(R_T, Xp);
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
