/* experiment_3param.c
 * 3パラメータ回転の検証実験
 *
 * 目的:
 *   1. 解析微分（∂E/∂ωk）と数値微分が一致することを確認
 *   2. Y軸回転のみの場合に、ω₂のみが有効でω₁,ω₃≈0 になることを確認
 *   3. LM法の更新ステップで回転を正しく推定できることを確認
 *
 * 使い方:
 *   ./build/experiment_3param <基準画像> <参照画像> [期待角度(度)]
 *
 * 例:
 *   ./build/experiment_3param images/base/base.jpg images/reference/reference_5deg.jpg 5.0
 *
 * 出力ファイル（results/ 以下）:
 *   gradient_3param.csv      <- 各ωk 方向の解析微分と数値微分の比較
 *   hessian_3param.csv       <- ヘッセ行列の値
 *   lm_convergence.csv       <- LM法の収束過程（目的関数値の推移）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/image_utils.h"
#include "../include/vector_math.h"
#include "../include/coord_transform.h"
#include "../include/y_rotation.h"
#include "objective_3param.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 比較領域（validate_y_rotation.c と同じ） */
#define REGION_U_MIN 2850
#define REGION_V_MIN 1425
#define REGION_U_MAX 3229
#define REGION_V_MAX 1614

/* LM法のパラメータ */
#define LM_C_INIT    0.0001  /* 初期値（資料 アルゴリズム 手順2） */
#define LM_MAX_ITER  50      /* 最大反復回数 */
#define LM_EPS_E     1e-10   /* 受理判定 |E' - E| < epsilon（資料 手順5g） */
#define LM_EPS_OMEGA 1e-8    /* 収束判定 ||domega|| < epsilon_omega（資料 手順5h） */
#define LM_C_MAX     1e10    /* Cがこれを超えたら発散とみなして打ち切り */


/* ===========================
 * 勾配・ヘッセ行列の検証（解析 vs 数値）
 * =========================== */
static void verify_gradient(Image *base, Image *ref, Matrix3x3 R,
                             FILE *fp_grad)
{
    printf("\n--- 勾配の検証（解析微分 vs 数値微分）---\n");
    printf("  ※ 両者が近い値であれば実装が正しい\n\n");

    /* 数値微分の微小変化量（ラジアン） */
    double delta = 1e-3;

    /* 解析微分を計算 */
    double grad[3];
    compute_gradient_3param(base, ref, R,
                             REGION_U_MIN, REGION_V_MIN,
                             REGION_U_MAX, REGION_V_MAX,
                             grad);

    printf("  %-10s  %15s  %15s  %15s\n",
           "param", "analytical", "numerical", "diff");
    printf("  %-10s  %15s  %15s  %15s\n",
           "----------", "----------", "----------", "----------");

    if (fp_grad) {
        fprintf(fp_grad, "param,analytical,numerical,diff\n");
    }

    const char *pname[3] = {"omega1", "omega2", "omega3"};
    for (int k = 0; k < 3; k++) {
        double num = compute_numerical_gradient_3param(
            base, ref, R, k, delta,
            REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX);

        double diff = grad[k] - num;

        printf("  omega%d      %15.8f  %15.8f  %15.8f\n",
               k + 1, grad[k], num, diff);

        if (fp_grad) {
            fprintf(fp_grad, "%s,%.10f,%.10f,%.10f\n",
                    pname[k], grad[k], num, diff);
        }
    }

    printf("\n  ヒント: Y軸回転なので omega2 だけが非ゼロになるはずです\n");
}


/* ===========================
 * ヘッセ行列の表示
 * =========================== */
static void print_hessian(double H[3][3], FILE *fp_hess)
{
    printf("\n--- ヘッセ行列（d^2E/domegak*domegal）---\n");
    printf("  ガウス・ニュートン近似（資料 式4）\n\n");
    printf("          omega1        omega2        omega3\n");

    if (fp_hess) {
        fprintf(fp_hess, "row,col,value\n");
    }

    for (int i = 0; i < 3; i++) {
        printf("  omega%d", i + 1);
        for (int j = 0; j < 3; j++) {
            printf("  %12.6f", H[i][j]);
            if (fp_hess) {
                fprintf(fp_hess, "%d,%d,%.10f\n", i, j, H[i][j]);
            }
        }
        printf("\n");
    }
}


/* ===========================
 * LM法による最適化
 * （資料 第6章 アルゴリズム に準拠）
 * =========================== */
static void run_lm_optimization(Image *base, Image *ref,
                                 double expected_angle_deg,
                                 FILE *fp_conv)
{
    printf("\n===== LM法による最適化 =====\n");
    printf("  資料 第6章 アルゴリズムに準拠\n");
    printf("  初期値: 単位行列（回転なし）\n");
    printf("  期待値: Y軸回転 %.2f deg\n\n", expected_angle_deg);

    /* 手順1: R <- R0（初期値 = 単位行列） */
    Matrix3x3 R = matrix_identity();

    /* 手順2: C = 0.0001 */
    double C = LM_C_INIT;

    /* 初期目的関数 */
    double E = compute_objective_3param(base, ref, R,
                                         REGION_U_MIN, REGION_V_MIN,
                                         REGION_U_MAX, REGION_V_MAX);
    printf("  初期目的関数 E0 = %.8f\n\n", E);

    if (fp_conv) {
        fprintf(fp_conv, "iter,objective,domega1,domega2,domega3,C,accepted\n");
        fprintf(fp_conv, "0,%.10f,0.0,0.0,0.0,%.6f,1\n", E, C);
    }

    printf("  %-5s  %-14s  %-10s  %-10s  %-10s  %-12s  %s\n",
           "iter", "objective", "domega1", "domega2", "domega3", "C", "ok?");
    printf("  %-5s  %-14s  %-10s  %-10s  %-10s  %-12s  %s\n",
           "-----", "----------", "--------", "--------", "--------", "--------", "----");

    /* 手順5: 反復 */
    for (int iter = 1; iter <= LM_MAX_ITER; iter++) {

        /* 手順5b,c: 勾配・ヘッセ行列を計算 */
        double grad[3];
        compute_gradient_3param(base, ref, R,
                                 REGION_U_MIN, REGION_V_MIN,
                                 REGION_U_MAX, REGION_V_MAX,
                                 grad);

        double H[3][3];
        compute_hessian_3param(base, ref, R,
                                REGION_U_MIN, REGION_V_MIN,
                                REGION_U_MAX, REGION_V_MAX,
                                H);

        /* 手順5d: LM法の更新式（資料 式19）
         *
         * A[k][k] = (1+C) * H[k][k]  （対角要素のみ (1+C) 倍）
         * A[k][l] = H[k][l]           （非対角要素はそのまま）
         * b = -grad
         *
         * A * domega = b を解く
         */
        double A[3][3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                A[i][j] = H[i][j];
            }
            A[i][i] = H[i][i] * (1.0 + C);
        }
        double b[3] = {-grad[0], -grad[1], -grad[2]};

        double domega[3];
        int solve_ok = solve_linear_3x3(A, b, domega);

        if (solve_ok != 0) {
            C *= 10.0;
            if (C > LM_C_MAX) {
                printf("  C が上限を超えました。終了します。\n");
                break;
            }
            continue;
        }

        /* ||Δω|| を計算（収束判定用） */
        double omega_norm = sqrt(domega[0]*domega[0]
                                + domega[1]*domega[1]
                                + domega[2]*domega[2]);

        /* 手順5e: ΔR をロドリゲス公式で計算（資料 式22） */
        Matrix3x3 dR = rodrigues(domega);

        /* 手順5f: R' <- ΔR * R */
        Matrix3x3 R_new = matrix_multiply(dR, R);

        /* E' を計算 */
        double E_new = compute_objective_3param(base, ref, R_new,
                                                 REGION_U_MIN, REGION_V_MIN,
                                                 REGION_U_MAX, REGION_V_MAX);

        /* 手順5g: 受理判定
         *   E' < E  または  |E' - E| < epsilon  -> 受理
         *   それ以外 -> 棄却して C *= 10
         */
        int accepted = (E_new < E || fabs(E_new - E) < LM_EPS_E) ? 1 : 0;

        printf("  %-5d  %-14.8f  %-10.6f  %-10.6f  %-10.6f  %-12.2e  %s\n",
               iter, E_new,
               domega[0], domega[1], domega[2],
               C, accepted ? "o" : "x");

        if (fp_conv) {
            fprintf(fp_conv, "%d,%.10f,%.8f,%.8f,%.8f,%.6e,%d\n",
                    iter, E_new,
                    domega[0], domega[1], domega[2],
                    C, accepted);
        }

        if (accepted) {
            R = R_new;
            E = E_new;
            C /= 10.0;
            if (C < LM_C_INIT) C = LM_C_INIT;
        } else {
            C *= 10.0;
            if (C > LM_C_MAX) {
                printf("  C が上限を超えました。終了します。\n");
                break;
            }
        }

        /* 手順5h: 収束判定 ||Δω|| < epsilon_omega */
        if (omega_norm < LM_EPS_OMEGA) {
            printf("\n  -> 収束 (||domega|| = %.2e < %.2e)\n",
                   omega_norm, LM_EPS_OMEGA);
            break;
        }
    }

    /* ===== 最終結果の表示 ===== */

    /* Y軸回転角を推定
     * R_Y(psi) = [cos  0  -sin; 0 1 0; sin 0 cos]
     * psi = atan2(R[2][0], R[2][2]) が安定
     */
    double est_rad = atan2(R.m[2][0], R.m[2][2]);
    double est_deg = est_rad * 180.0 / M_PI;

    printf("\n===== 最適化結果 =====\n");
    printf("  推定回転角（Y軸）: %8.4f deg\n", est_deg);
    printf("  期待回転角:        %8.4f deg\n", expected_angle_deg);
    printf("  誤差:              %8.4f deg\n", fabs(est_deg - expected_angle_deg));
    printf("  最終目的関数 E:    %.8f\n", E);

    printf("\n  最終回転行列 R:\n");
    matrix_print("  R", R);
}


/* ===========================
 * main
 * =========================== */
int main(int argc, char *argv[])
{
    printf("===== 3パラメータ回転の検証実験 =====\n\n");

    if (argc < 3) {
        fprintf(stderr, "使い方: %s <基準画像> <参照画像> [期待角度(度)]\n\n", argv[0]);
        fprintf(stderr, "例:\n");
        fprintf(stderr, "  %s images/base/base.jpg images/reference/reference_5deg.jpg 5.0\n\n",
                argv[0]);
        fprintf(stderr, "出力:\n");
        fprintf(stderr, "  results/gradient_3param.csv   勾配の解析/数値比較\n");
        fprintf(stderr, "  results/hessian_3param.csv    ヘッセ行列\n");
        fprintf(stderr, "  results/lm_convergence.csv    LM法の収束過程\n");
        return 1;
    }

    const char *base_file = argv[1];
    const char *ref_file  = argv[2];
    double expected_deg   = (argc >= 4) ? atof(argv[3]) : 5.0;

    printf("基準画像: %s\n", base_file);
    printf("参照画像: %s\n", ref_file);
    printf("期待角度: %.2f deg\n", expected_deg);
    printf("比較領域: (%d,%d) - (%d,%d)\n\n",
           REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX);

    /* 画像読み込み */
    Image *base = image_load(base_file);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像を読み込めません\n");
        return 1;
    }

    Image *ref = image_load(ref_file);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像を読み込めません\n");
        image_free(base);
        return 1;
    }

    /* 出力ファイルを開く */
    FILE *fp_grad = fopen("results/gradient_3param.csv", "w");
    FILE *fp_hess = fopen("results/hessian_3param.csv",  "w");
    FILE *fp_conv = fopen("results/lm_convergence.csv",  "w");

    if (!fp_grad || !fp_hess || !fp_conv) {
        fprintf(stderr, "エラー: results/ フォルダがありません\n");
        fprintf(stderr, "  mkdir -p results\n");
        image_free(base);
        image_free(ref);
        return 1;
    }

    /* 初期回転行列（単位行列） */
    Matrix3x3 R0 = matrix_identity();

    /* ===== 検証1: 勾配 ===== */
    printf("===== 検証1: 勾配（dE/domegak）解析 vs 数値 =====\n");
    verify_gradient(base, ref, R0, fp_grad);

    /* ===== 検証2: ヘッセ行列 ===== */
    printf("\n===== 検証2: ヘッセ行列 =====\n");
    double H[3][3];
    compute_hessian_3param(base, ref, R0,
                            REGION_U_MIN, REGION_V_MIN,
                            REGION_U_MAX, REGION_V_MAX,
                            H);
    print_hessian(H, fp_hess);

    /* ===== 検証3: LM法 ===== */
    printf("\n");
    run_lm_optimization(base, ref, expected_deg, fp_conv);

    fclose(fp_grad);
    fclose(fp_hess);
    fclose(fp_conv);
    image_free(base);
    image_free(ref);

    printf("\n===== 完了 =====\n");
    printf("結果ファイル:\n");
    printf("  results/gradient_3param.csv\n");
    printf("  results/hessian_3param.csv\n");
    printf("  results/lm_convergence.csv\n");
    printf("\nグラフ描画:\n");
    printf("  python3 experiment/plot_results_3param.py --expected-angle %.2f\n",
           expected_deg);

    return 0;
}