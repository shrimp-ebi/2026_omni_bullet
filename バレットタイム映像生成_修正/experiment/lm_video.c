/* lm_video.c
 * 1フレーム分のLM推定と注視画像生成（ctypes用共有ライブラリ）
 */

#include "coord_transform.h"
#include "image_utils.h"
#include "rotation.h"
#include "vector_math.h"
#include "objective_3param.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define C_INIT           0.0001
#define EPS_OMEGA        1e-10
#define MAX_ITER         300
#define MAX_SOLVE_FAIL   30

static Image *rgb_to_image(const unsigned char *rgb, int W, int H)
{
    Image *img = image_create(W, H, 3);
    if (!img)
        return NULL;
    memcpy(img->data, rgb, (size_t)W * H * 3);
    return img;
}

static void matrix_to_row_major(const Matrix3x3 *R, double *R_out)
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R_out[i * 3 + j] = R->m[i][j];
}

static Matrix3x3 row_major_to_matrix(const double *R_in)
{
    Matrix3x3 R;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            R.m[i][j] = R_in[i * 3 + j];
    return R;
}

/* 注視点 G から初期回転行列 R0 を構築
 * ez = N[G], ex = N[up × ez], ey = N[ez × ex]
 * R0 = [ex ey ez] を列ベクトルとして格納（R0·e_z = G → 中心が注視点）
 */
static Matrix3x3 build_initial_rotation(double gaze_x, double gaze_y, double gaze_z)
{
    Vector3D G = vector_create(gaze_x, gaze_y, gaze_z);
    Vector3D ez = vector_normalize(G);

    Vector3D up = vector_create(0.0, 1.0, 0.0);
    Vector3D cross = vector_cross(up, ez);
    if (vector_norm(cross) < 1e-10) {
        Vector3D alt_up = vector_create(1.0, 0.0, 0.0);
        cross = vector_cross(alt_up, ez);
    }
    Vector3D ex = vector_normalize(cross);
    Vector3D ey = vector_normalize(vector_cross(ez, ex));

    Matrix3x3 R0;
    R0.m[0][0] = ex.x; R0.m[0][1] = ey.x; R0.m[0][2] = ez.x;
    R0.m[1][0] = ex.y; R0.m[1][1] = ey.y; R0.m[1][2] = ez.y;
    R0.m[2][0] = ex.z; R0.m[2][1] = ey.z; R0.m[2][2] = ez.z;

    return R0;
}

int lm_estimate_frame(
    const unsigned char *base_rgb,
    const unsigned char *ref_rgb,
    int W, int H,
    const double *R_prev,
    double *R_out)
{
    Image *base = rgb_to_image(base_rgb, W, H);
    Image *ref  = rgb_to_image(ref_rgb, W, H);
    if (!base || !ref) {
        image_free(base);
        image_free(ref);
        return -1;
    }

    /* 比較領域: 画像中心付近（注視点が来る領域）の小窓
     * 資料の検証設定 380×190 @ 6080×3040 に相当する比率 */
    int cw = (int)(W * 380.0 / 6080.0 / 2.0);
    int ch = (int)(H * 190.0 / 3040.0 / 2.0);
    if (cw < 32) cw = 32;
    if (ch < 16) ch = 16;
    int u_min = W / 2 - cw / 2;
    int u_max = W / 2 + cw / 2;
    int v_min = H / 2 - ch / 2;
    int v_max = H / 2 + ch / 2;
    if (u_min < 0) u_min = 0;
    if (v_min < 0) v_min = 0;
    if (u_max >= W) u_max = W - 1;
    if (v_max >= H) v_max = H - 1;

    /* 前フレームの回転を初期値にする（参考実装 estimate_R と同じ） */
    Matrix3x3 R = row_major_to_matrix(R_prev);
    double C = C_INIT;

    double E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);

    int iter;
    int solve_fail = 0;
    for (iter = 0; iter < MAX_ITER; iter++) {

        double grad[3];
        compute_gradient_3param(base, ref, R, u_min, v_min, u_max, v_max, grad);

        double hessian[3][3];
        compute_hessian_3param(base, ref, R, u_min, v_min, u_max, v_max, hessian);

        double A[3][3];
        double b[3];
        for (int k = 0; k < 3; k++) {
            for (int l = 0; l < 3; l++)
                A[k][l] = hessian[k][l];
            b[k] = -grad[k];
        }
        A[0][0] *= (1.0 + C);
        A[1][1] *= (1.0 + C);
        A[2][2] *= (1.0 + C);

        double delta_omega[3];
        if (solve_linear_3x3(A, b, delta_omega) != 0) {
            C *= 10.0;
            if (++solve_fail >= MAX_SOLVE_FAIL)
                break;
            continue;
        }
        solve_fail = 0;

        Matrix3x3 dR    = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);

        double E_new = compute_objective_3param(base, ref, R_new, u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0] * delta_omega[0]
                            + delta_omega[1] * delta_omega[1]
                            + delta_omega[2] * delta_omega[2]);

        if (E_new < E) {
            R = R_new;
            E = E_new;
            C = C / 10.0;
        } else {
            C = C * 10.0;
        }

        if (norm_dw < EPS_OMEGA)
            break;
    }

    matrix_to_row_major(&R, R_out);

    image_free(base);
    image_free(ref);

    return (iter < MAX_ITER) ? iter + 1 : MAX_ITER;
}

void get_gaze_rotation(double gaze_x, double gaze_y, double gaze_z, double *R_out)
{
    Matrix3x3 R0 = build_initial_rotation(gaze_x, gaze_y, gaze_z);
    matrix_to_row_major(&R0, R_out);
}

void generate_gaze_frame(
    const unsigned char *in_rgb,
    const double *R,
    int W, int H,
    unsigned char *out_rgb)
{
    Image *input = rgb_to_image(in_rgb, W, H);
    if (!input)
        return;

    Matrix3x3 Rmat = row_major_to_matrix(R);

    for (int vp = 0; vp < H; vp++) {
        for (int up = 0; up < W; up++) {
            Vector3D Xp = image_to_world(up, vp, W, H);
            /* 列形式 R: 資料の R^T X' と等価な R·X'（R0·e_z = G） */
            Vector3D X  = matrix_vector_multiply(Rmat, Xp);

            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);

            uint8_t rgb[3];
            get_pixel_bilinear(input, u_in, v_in, rgb);

            size_t idx = ((size_t)vp * W + up) * 3;
            out_rgb[idx + 0] = rgb[0];
            out_rgb[idx + 1] = rgb[1];
            out_rgb[idx + 2] = rgb[2];
        }
    }

    image_free(input);
}

/* LMなし: 注視点GからR0を作り注視画像を生成（検証用） */
void generate_gaze_at_gaze_point(
    const unsigned char *in_rgb,
    int W, int H,
    double gaze_x, double gaze_y, double gaze_z,
    unsigned char *out_rgb)
{
    Matrix3x3 R = build_initial_rotation(gaze_x, gaze_y, gaze_z);
    double R_row[9];
    matrix_to_row_major(&R, R_row);
    generate_gaze_frame(in_rgb, R_row, W, H, out_rgb);
}
