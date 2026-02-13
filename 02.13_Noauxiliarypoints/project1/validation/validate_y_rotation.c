/* validate_y_rotation.c
 * Y軸回りの1パラメータ検証実験
 * 
 * 目的:
 *   1. 目的関数E(ψ)が期待角度で最小値をとることを確認
 *   2. 理論微分と数値微分が一致することを確認
 * 
 * 使い方:
 *   ./validate_y_rotation <基準画像> <参照画像> [期待角度(度)]
 * 
 * 例:
 *   ./validate_y_rotation images/base/base.jpg images/reference/reference_18_5deg.jpg 18.5
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/y_rotation.h"
#include "../include/image_utils.h"

/* 比較領域の設定（資料より） */
#define REGION_U_MIN 2850
#define REGION_V_MIN 1425
#define REGION_U_MAX 3229
#define REGION_V_MAX 1614

/* 角度範囲の設定 */
#define ANGLE_HALF_RANGE 10.0
#define ANGLE_STEP 0.1

int main(int argc, char *argv[]) {
    printf("===== Y軸回りの回転検証実験 =====\n\n");
    
    /* コマンドライン引数のチェック */
    if (argc < 3) {
        fprintf(stderr, "使い方: %s <基準画像> <参照画像> [期待角度(度)]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "引数:\n");
        fprintf(stderr, "  基準画像: 注視点が中心にある画像（I_b）\n");
        fprintf(stderr, "  参照画像: 任意角度で回転させた画像（I_r）\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "例:\n");
        fprintf(stderr, "  %s images/base/base.jpg images/reference/reference_18_5deg.jpg 18.5\n", argv[0]);
        return 1;
    }
    
    const char *base_filename = argv[1];
    const char *ref_filename = argv[2];
    double expected_angle_deg = 5.0;
    if (argc >= 4) {
        expected_angle_deg = atof(argv[3]);
    }
    
    printf("基準画像: %s\n", base_filename);
    printf("参照画像: %s\n", ref_filename);
    printf("期待角度: %.2f°\n", expected_angle_deg);
    printf("比較領域: (%d, %d) - (%d, %d)\n", 
           REGION_U_MIN, REGION_V_MIN, REGION_U_MAX, REGION_V_MAX);
    double angle_min = expected_angle_deg - ANGLE_HALF_RANGE;
    double angle_max = expected_angle_deg + ANGLE_HALF_RANGE;

    printf("角度範囲: %.1f° ~ %.1f° (刻み %.1f°)\n\n", 
           angle_min, angle_max, ANGLE_STEP);
    
    /* 画像の読み込み */
    printf("【画像読み込み】\n");
    Image *base = image_load(base_filename);
    if (!base) {
        fprintf(stderr, "エラー: 基準画像の読み込みに失敗\n");
        return 1;
    }
    
    Image *ref = image_load(ref_filename);
    if (!ref) {
        fprintf(stderr, "エラー: 参照画像の読み込みに失敗\n");
        image_free(base);
        return 1;
    }
    
    /* 比較領域を切り出して保存 */
    printf("\n【比較領域の保存】\n");
    int region_width = REGION_U_MAX - REGION_U_MIN;
    int region_height = REGION_V_MAX - REGION_V_MIN;
    
    Image *region = image_create(region_width, region_height, 3);
    if (region) {
        for (int v = 0; v < region_height; v++) {
            for (int u = 0; u < region_width; u++) {
                uint8_t rgb[3];
                get_pixel(base, REGION_U_MIN + u, REGION_V_MIN + v, rgb);
                set_pixel(region, u, v, rgb);
            }
        }
        
        if (image_save_jpg("results/region_base.jpg", region, 95)) {
            printf("  比較領域を保存: results/region_base.jpg (%d × %d)\n", 
                   region_width, region_height);
        } else {
            fprintf(stderr, "  警告: 比較領域の保存に失敗\n");
        }
        image_free(region);
    }
    
    /* 出力ファイルを開く */
    printf("\n【計算開始】\n");
    FILE *fp_obj = fopen("results/objective_function.csv", "w");
    FILE *fp_der = fopen("results/derivatives.csv", "w");
    
    if (!fp_obj || !fp_der) {
        fprintf(stderr, "エラー: 出力ファイルが開けません\n");
        fprintf(stderr, "  results/ディレクトリを作成してください\n");
        image_free(base);
        image_free(ref);
        return 1;
    }
    
    /* CSVヘッダー */
    fprintf(fp_obj, "angle_deg,objective_function\n");
    fprintf(fp_der, "angle_deg,analytical_derivative,numerical_derivative\n");
    
    /* 角度を変化させて計算 */
    int total_points = (int)((angle_max - angle_min) / ANGLE_STEP) + 1;
    int progress_step = total_points / 10;
    if (progress_step <= 0) {
        progress_step = 1;
    }
    
    printf("  計算点数: %d点\n", total_points);
    printf("  処理中");
    fflush(stdout);
    
    for (double psi = angle_min; psi <= angle_max + 1e-9; psi += ANGLE_STEP) {
        int point_num = (int)((psi - angle_min) / ANGLE_STEP);
        
        if (point_num % progress_step == 0) {
            printf(".");
            fflush(stdout);
        }
        
        /* 目的関数を計算 */
        double E = compute_objective_function(
            base, ref, psi,
            REGION_U_MIN, REGION_V_MIN,
            REGION_U_MAX, REGION_V_MAX
        );
        
        /* 理論微分を計算 */
        double dE_analytical = compute_analytical_derivative(
            base, ref, psi,
            REGION_U_MIN, REGION_V_MIN,
            REGION_U_MAX, REGION_V_MAX
        );
        
        /* 数値微分を計算 */
        double dE_numerical = compute_numerical_derivative(
            base, ref, psi, ANGLE_STEP,
            REGION_U_MIN, REGION_V_MIN,
            REGION_U_MAX, REGION_V_MAX
        );
        
        /* CSVに書き込み */
        fprintf(fp_obj, "%.2f,%.6f\n", psi, E);
        fprintf(fp_der, "%.2f,%.6f,%.6f\n", psi, dE_analytical, dE_numerical);
    }
    
    printf(" 完了！\n");
    
    /* ファイルを閉じる */
    fclose(fp_obj);
    fclose(fp_der);

    /* グラフ描画用に期待角度を保存 */
    FILE *fp_expected = fopen("results/expected_angle.txt", "w");
    if (fp_expected) {
        fprintf(fp_expected, "%.6f\n", expected_angle_deg);
        fclose(fp_expected);
    } else {
        fprintf(stderr, "警告: results/expected_angle.txt の保存に失敗\n");
    }
    
    /* メモリ解放 */
    image_free(base);
    image_free(ref);
    
    printf("\n【結果保存】\n");
    printf("  目的関数: results/objective_function.csv\n");
    printf("  微分値: results/derivatives.csv\n");
    
    printf("\n===== 計算完了 =====\n");
    printf("次のステップ: Pythonでグラフを描画してください\n");
    printf("  python3 validation/plot_results.py --expected-angle %.2f\n", expected_angle_deg);
    
    return 0;
}