/* bullet_time_video.c
 * 全方位動画からのバレットタイム映像生成（連続フレーム処理）
 *
 * 処理フロー:
 *   1. コマンドライン引数から注視点と平滑化設定を受け取る
 *   2. frames/ の連番 JPEG を順番に読み込む
 *   3. 第1フレーム: 注視点から初期 R を作り注視画像を生成
 *   4. 第2フレーム以降: 前フレームの注視画像を Ib、現フレームを Ir として
 *      LM 法で R を推定し注視画像を生成
 *   5. gaze_frames/ に注視画像を保存、lm_log.csv にログを記録
 *
 * 使い方:
 *   ./build/bullet_time_video --gaze-u U --gaze-v V [オプション]
 *
 * オプション:
 *   --gaze-u U              注視点 u 座標 [px]（必須）
 *   --gaze-v V              注視点 v 座標 [px]（必須）
 *   --smooth-method METHOD  blur または deriv（デフォルト: blur）
 *   --sigma S               sigma 値（デフォルト: 3.0）
 *   --input-dir DIR         入力フレームディレクトリ（デフォルト: frames）
 *   --output-dir DIR        出力フレームディレクトリ（デフォルト: gaze_frames）
 *   --log-file FILE         CSV ログファイル（デフォルト: lm_log.csv）
 *
 * 平滑化方式:
 *   blur  ... image_gaussian_blur で ref をピクセルレベルで事前ぼかし→勾配・ヘッセに使用
 *             （06_smoothing 検証コードと同じ方式）
 *             kernel_size=5 固定、sigma はコマンドライン引数
 *   deriv ... 差分カーネル i×exp(-i²/2σ²) を使う加重差分方式
 *             sigma はコマンドライン引数、radius は ceil(3×sigma)
 *
 * どちらの方式でも E の計算には元画像（平滑化なし）を使う。
 *
 * ドリフト注意: 毎フレーム「前フレームの注視画像」を Ib とするため、
 * 推定誤差がフレームをまたいで積算する可能性がある（フレーム0固定方式との比較は将来課題）。
 * 基準画像の更新は generate_gaze_image() の後に1箇所で行う（変更しやすい構造を維持）。
 */

#include "../include/coord_transform.h"
#include "../include/image_utils.h"
#include "../include/rotation.h"
#include "../include/vector_math.h"
#include "../experiment/objective_3param.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===========================
 * 定数
 * =========================== */

/* LM 法パラメータ */
#define C_INIT      0.0001
#define EPS_OMEGA   1e-8
#define EPS_E       1e-6   /* 受理判定: |E_new - E| < EPS_E なら受理（論文準拠） */
#define MAX_ITER    500

/* 比較領域は画像サイズに対する割合で決める（解像度非依存）。
 * 半幅 = 幅 × COMPARE_FRAC, 半高 = 高さ × COMPARE_FRAC。
 * 例: 5760×2880, FRAC=1/8 → 半幅720×半高360 → 領域1440×720 */
#define COMPARE_FRAC  0.125

/* blur 方式の kernel_size（06_smoothing 検証コードと同一） */
#define BLUR_KERNEL_SIZE  5

/* deriv 方式の sigma/radius のデフォルト（通常は blur 方式を使うので参考値） */
#define DERIV_SIGMA_DEFAULT   1.0
#define DERIV_RADIUS_DEFAULT  3


/* ===========================
 * 設定構造体
 * =========================== */

typedef enum { SMOOTH_BLUR, SMOOTH_DERIV } SmoothMethod;

typedef struct {
    int    gaze_u;
    int    gaze_v;
    SmoothMethod method;
    double sigma;
    char   input_dir[256];
    char   output_dir[256];
    char   log_file[256];
} Config;


/* ===========================
 * ユーティリティ
 * =========================== */

static void usage(const char *prog) {
    fprintf(stderr,
        "使い方: %s --gaze-u U --gaze-v V [オプション]\n"
        "\n"
        "必須引数:\n"
        "  --gaze-u U             注視点の u 座標 [px]\n"
        "  --gaze-v V             注視点の v 座標 [px]\n"
        "\n"
        "オプション:\n"
        "  --smooth-method METHOD  blur または deriv（デフォルト: blur）\n"
        "  --sigma S               sigma 値（デフォルト: 3.0）\n"
        "  --input-dir DIR         入力フレームディレクトリ（デフォルト: frames）\n"
        "  --output-dir DIR        出力フレームディレクトリ（デフォルト: gaze_frames）\n"
        "  --log-file FILE         CSV ログ（デフォルト: lm_log.csv）\n",
        prog);
}

static int parse_args(int argc, char *argv[], Config *cfg) {
    /* デフォルト値 */
    cfg->gaze_u = -1;
    cfg->gaze_v = -1;
    cfg->method = SMOOTH_BLUR;
    cfg->sigma  = 3.0;
    strcpy(cfg->input_dir,  "frames");
    strcpy(cfg->output_dir, "gaze_frames");
    strcpy(cfg->log_file,   "lm_log.csv");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--gaze-u") == 0 && i + 1 < argc) {
            cfg->gaze_u = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--gaze-v") == 0 && i + 1 < argc) {
            cfg->gaze_v = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--smooth-method") == 0 && i + 1 < argc) {
            ++i;
            if (strcmp(argv[i], "blur") == 0) {
                cfg->method = SMOOTH_BLUR;
            } else if (strcmp(argv[i], "deriv") == 0) {
                cfg->method = SMOOTH_DERIV;
            } else {
                fprintf(stderr, "エラー: --smooth-method は blur または deriv を指定してください\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--sigma") == 0 && i + 1 < argc) {
            cfg->sigma = atof(argv[++i]);
            if (cfg->sigma <= 0.0) {
                fprintf(stderr, "エラー: --sigma は正の値を指定してください\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--input-dir") == 0 && i + 1 < argc) {
            strncpy(cfg->input_dir, argv[++i], 255);
        } else if (strcmp(argv[i], "--output-dir") == 0 && i + 1 < argc) {
            strncpy(cfg->output_dir, argv[++i], 255);
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            strncpy(cfg->log_file, argv[++i], 255);
        } else {
            fprintf(stderr, "エラー: 未知の引数: %s\n", argv[i]);
            return -1;
        }
    }

    if (cfg->gaze_u < 0 || cfg->gaze_v < 0) {
        fprintf(stderr, "エラー: --gaze-u と --gaze-v は必須です\n");
        return -1;
    }
    return 0;
}

/* ディレクトリ作成（存在しても OK） */
static void mkdir_if_needed(const char *dir) {
    struct stat st;
    if (stat(dir, &st) != 0) {
        mkdir(dir, 0755);
    }
}

/* 入力フレームの総数を数える */
static int count_frames(const char *dir) {
    char path[512];
    int n = 0;
    while (1) {
        snprintf(path, sizeof(path), "%s/frame_%05d.jpg", dir, n + 1);
        FILE *f = fopen(path, "rb");
        if (!f) break;
        fclose(f);
        n++;
    }
    return n;
}


/* ===========================
 * 注視画像生成（逆写像）
 * ===========================
 *
 * 出力画像の各画素 (u', v') について:
 *   X' = image_to_world(u', v')
 *   X  = R * X'  （R は行形式。R * Xp で入力画像の点に戻る）
 *   (u, v) = world_to_image(X)  → 入力画像の参照点
 *   u は周期境界で巻き込まれる（world_to_image 内で処理済み）
 *
 * 注: 既存実装では行形式の R をそのまま掛けて逆写像する（create_reference_3param.c と同じ方式）。
 */
static Image* generate_gaze_image(Image *input, Matrix3x3 R) {
    int W = input->width;
    int H = input->height;
    Image *out = image_create_like(input);
    if (!out) return NULL;

    /* LM側は順変換 X' = R*X で R を使う（検証済み）。
     * 画像生成は逆変換（出力画素→入力画素）なので転置 R^T を使う。
     * 転置はループ外で1回だけ計算する。 */
    Matrix3x3 Rt;
    Rt.m[0][0] = R.m[0][0]; Rt.m[0][1] = R.m[1][0]; Rt.m[0][2] = R.m[2][0];
    Rt.m[1][0] = R.m[0][1]; Rt.m[1][1] = R.m[1][1]; Rt.m[1][2] = R.m[2][1];
    Rt.m[2][0] = R.m[0][2]; Rt.m[2][1] = R.m[1][2]; Rt.m[2][2] = R.m[2][2];

    for (int vp = 0; vp < H; vp++) {
        for (int up = 0; up < W; up++) {
            Vector3D Xp = image_to_world(up, vp, W, H);
            Vector3D X  = matrix_vector_multiply(Rt, Xp);
            double u_in, v_in;
            world_to_image(X, W, H, &u_in, &v_in);
            uint8_t rgb[3];
            get_pixel_bilinear(input, u_in, v_in, rgb);
            set_pixel(out, up, vp, rgb);
        }
    }
    return out;
}


/* ===========================
 * LM 法による R の更新
 * ===========================
 *
 * 引数:
 *   base      ... 基準画像 Ib（前フレームの注視画像）
 *   ref       ... 参照画像 Ir（現フレームの生画像）
 *   R_init    ... LM 初期値（前フレームで使った R）
 *   cfg       ... 平滑化設定（method, sigma）
 *   out_iters ... 反復回数の出力先
 *   out_E     ... 最終 E の出力先
 *   out_omega ... 最終フレームで積算した Δω ノルムの出力先
 *
 * 戻り値: 推定された回転行列 R
 */
static Matrix3x3 lm_estimate(Image *base, Image *ref,
                              Matrix3x3 R_init,
                              const Config *cfg,
                              int *out_iters, double *out_E,
                              double *out_omega_norm)
{
    int W = base->width;
    int H = base->height;
    int half_u = (int)(W * COMPARE_FRAC);
    int half_v = (int)(H * COMPARE_FRAC);
    int u_min = W / 2 - half_u;
    int u_max = W / 2 + half_u;
    int v_min = H / 2 - half_v;
    int v_max = H / 2 + half_v;

    /* blur 方式: 参照画像を事前ぼかし */
    Image *ref_for_deriv = NULL;
    int    deriv_radius  = DERIV_RADIUS_DEFAULT;

    if (cfg->method == SMOOTH_BLUR) {
        ref_for_deriv = image_gaussian_blur(ref, BLUR_KERNEL_SIZE, cfg->sigma);
        if (!ref_for_deriv) {
            fprintf(stderr, "エラー: image_gaussian_blur 失敗\n");
            *out_iters      = 0;
            *out_E          = -1.0;
            *out_omega_norm = 0.0;
            return R_init;
        }
    } else {
        /* deriv 方式: radius = ceil(3 * sigma) */
        deriv_radius = (int)ceil(3.0 * cfg->sigma);
        if (deriv_radius < 3) deriv_radius = 3;
        ref_for_deriv = ref;  /* ぼかしなし */
    }

    Matrix3x3 R = R_init;
    double     C = C_INIT;
    double     E = compute_objective_3param(base, ref, R, u_min, v_min, u_max, v_max);

    int    iter          = 0;
    double norm_dw_final = 0.0;

    for (iter = 0; iter < MAX_ITER; iter++) {
        double grad[3];
        double hessian[3][3];

        if (cfg->method == SMOOTH_BLUR) {
            /* blur 方式: ぼかした ref_for_deriv を勾配・ヘッセに渡す */
            compute_gradient_3param(base, ref_for_deriv, R,
                                    u_min, v_min, u_max, v_max, grad);
            compute_hessian_3param(base, ref_for_deriv, R,
                                   u_min, v_min, u_max, v_max, hessian);
        } else {
            /* deriv 方式: sigma 可変の差分カーネル版 */
            compute_gradient_3param_sigma(base, ref_for_deriv, R,
                                          u_min, v_min, u_max, v_max,
                                          grad, cfg->sigma, deriv_radius);
            compute_hessian_3param_sigma(base, ref_for_deriv, R,
                                         u_min, v_min, u_max, v_max,
                                         hessian, cfg->sigma, deriv_radius);
        }

        /* (H + C*diag(H)) * Δω = -g */
        double A[3][3], b[3];
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
            continue;
        }

        Matrix3x3 dR    = rodrigues(delta_omega);
        Matrix3x3 R_new = matrix_multiply(dR, R);

        /* E 評価は元画像（平滑化なし） */
        double E_new = compute_objective_3param(base, ref, R_new,
                                                u_min, v_min, u_max, v_max);

        double norm_dw = sqrt(delta_omega[0] * delta_omega[0]
                            + delta_omega[1] * delta_omega[1]
                            + delta_omega[2] * delta_omega[2]);
        norm_dw_final = norm_dw;

        /* 受理判定（論文準拠）: E_new < E または |E_new - E| < EPS_E なら受理 */
        if (E_new < E || fabs(E_new - E) < EPS_E) {
            R = R_new;
            E = E_new;
            C /= 10.0;
        } else {
            C *= 10.0;
        }

        if (norm_dw < EPS_OMEGA) {
            iter++;
            break;
        }
    }

    if (cfg->method == SMOOTH_BLUR)
        image_free(ref_for_deriv);

    *out_iters      = iter;
    *out_E          = E;
    *out_omega_norm = norm_dw_final;
    return R;
}


/* ===========================
 * main
 * =========================== */

int main(int argc, char *argv[]) {
    Config cfg;
    if (parse_args(argc, argv, &cfg) != 0) {
        usage(argv[0]);
        return 1;
    }

    printf("===== バレットタイム映像生成 =====\n");
    printf("注視点    : u=%d, v=%d\n", cfg.gaze_u, cfg.gaze_v);
    printf("平滑化方式: %s\n", cfg.method == SMOOTH_BLUR ? "blur" : "deriv");
    printf("sigma     : %.2f\n", cfg.sigma);
    if (cfg.method == SMOOTH_BLUR) {
        printf("kernel_size: %d (固定)\n", BLUR_KERNEL_SIZE);
    } else {
        int r = (int)ceil(3.0 * cfg.sigma);
        if (r < 3) r = 3;
        printf("deriv radius: %d (= ceil(3×sigma))\n", r);
    }
    printf("入力 : %s/frame_NNNNN.jpg\n", cfg.input_dir);
    printf("出力 : %s/gaze_NNNNN.jpg\n", cfg.output_dir);
    printf("ログ : %s\n\n", cfg.log_file);

    mkdir_if_needed(cfg.output_dir);

    int n_frames = count_frames(cfg.input_dir);
    if (n_frames == 0) {
        fprintf(stderr, "エラー: %s にフレームが見つかりません\n", cfg.input_dir);
        return 1;
    }
    printf("フレーム数: %d\n\n", n_frames);

    /* CSV ログオープン */
    FILE *log_fp = fopen(cfg.log_file, "w");
    if (!log_fp) {
        fprintf(stderr, "エラー: ログファイルを開けません: %s\n", cfg.log_file);
        return 1;
    }
    /* ヘッダ行に実行設定を記録 */
    fprintf(log_fp, "# smooth_method=%s sigma=%.4f",
            cfg.method == SMOOTH_BLUR ? "blur" : "deriv", cfg.sigma);
    if (cfg.method == SMOOTH_BLUR) {
        fprintf(log_fp, " kernel_size=%d", BLUR_KERNEL_SIZE);
    } else {
        int r = (int)ceil(3.0 * cfg.sigma);
        if (r < 3) r = 3;
        fprintf(log_fp, " deriv_radius=%d", r);
    }
    fprintf(log_fp, " gaze_u=%d gaze_v=%d\n", cfg.gaze_u, cfg.gaze_v);
    fprintf(log_fp, "frame,iters,E,omega_norm\n");

    /* 注視点を世界座標に変換（第1フレームは任意解像度の1枚で初期化するため後回し） */
    Matrix3x3 R;
    Image    *gaze_img = NULL;   /* 前フレームの注視画像（= 次フレームの Ib） */
    int       first_frame_done = 0;

    for (int fi = 1; fi <= n_frames; fi++) {
        char path_in[512];
        snprintf(path_in, sizeof(path_in), "%s/frame_%05d.jpg", cfg.input_dir, fi);

        Image *frame = image_load(path_in);
        if (!frame) {
            fprintf(stderr, "警告: フレーム %d を読み込めません。スキップ。\n", fi);
            fprintf(log_fp, "%d,-1,-1.0,-1.0\n", fi);
            continue;
        }

        int W = frame->width;
        int H = frame->height;

        if (!first_frame_done) {
            /* 第1フレーム: 注視点から初期 R を作る */
            Vector3D G = image_to_world(cfg.gaze_u, cfg.gaze_v, W, H);
            R = compute_rotation_matrix_quiet(G);
            gaze_img = generate_gaze_image(frame, R);
            if (!gaze_img) {
                fprintf(stderr, "エラー: 第1フレームの注視画像生成失敗\n");
                image_free(frame);
                fclose(log_fp);
                return 1;
            }
            first_frame_done = 1;
            fprintf(log_fp, "%d,0,0.0,0.0\n", fi);
            printf("フレーム %d: 初期化（注視点から R 生成）\n", fi);
        } else {
            /* 第2フレーム以降: gaze_img を Ib、frame を Ir として LM 推定 */
            int    iters;
            double E, omega_norm;
            R = lm_estimate(gaze_img, frame, R, &cfg, &iters, &E, &omega_norm);

            /* 推定した R で注視画像を生成（次フレームの Ib になる） */
            Image *new_gaze = generate_gaze_image(frame, R);
            if (!new_gaze) {
                fprintf(stderr, "エラー: フレーム %d の注視画像生成失敗\n", fi);
                image_free(frame);
                continue;
            }
            image_free(gaze_img);
            gaze_img = new_gaze;   /* 基準画像の更新（ここ1箇所のみ） */

            fprintf(log_fp, "%d,%d,%.8f,%.8e\n", fi, iters, E, omega_norm);
            printf("フレーム %d: iters=%d  E=%.6f  |Δω|=%.2e\n",
                   fi, iters, E, omega_norm);
        }

        /* 注視画像を保存 */
        char path_out[512];
        snprintf(path_out, sizeof(path_out), "%s/gaze_%05d.jpg", cfg.output_dir, fi);
        image_save_jpg(path_out, gaze_img, 95);

        image_free(frame);
    }

    if (gaze_img) image_free(gaze_img);
    fclose(log_fp);

    printf("\n===== 処理完了 =====\n");
    printf("出力フレーム: %s/gaze_NNNNN.jpg\n", cfg.output_dir);
    printf("ログ        : %s\n", cfg.log_file);
    return 0;
}
