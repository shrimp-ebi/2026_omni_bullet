/* test_vector_math.c
 * vector_math.cの動作確認
 */

#include <stdio.h>
#include <math.h>
#include "vector_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    printf("===== ベクトル・行列演算のテスト =====\n\n");
    
    /* ===== ベクトル演算のテスト ===== */
    printf("【1】ベクトルの生成と表示\n");
    Vector3D v1 = vector_create(1.0, 2.0, 3.0);
    Vector3D v2 = vector_create(4.0, 5.0, 6.0);
    vector_print("v1", v1);
    vector_print("v2", v2);
    printf("\n");
    
    /* ===== ノルムのテスト ===== */
    printf("【2】ノルムの計算\n");
    double norm1 = vector_norm(v1);
    printf("||v1|| = %.6f\n", norm1);
    printf("期待値: %.6f\n", sqrt(1*1 + 2*2 + 3*3));
    printf("\n");
    
    /* ===== 正規化のテスト ===== */
    printf("【3】正規化（単位ベクトル化）\n");
    Vector3D v1_normalized = vector_normalize(v1);
    vector_print("v1_normalized", v1_normalized);
    printf("ノルム: %.6f (1.0であるべき)\n", vector_norm(v1_normalized));
    printf("\n");
    
    /* ===== 外積のテスト ===== */
    printf("【4】外積の計算\n");
    Vector3D v_cross = vector_cross(v1, v2);
    vector_print("v1 × v2", v_cross);
    printf("期待値: (-3.0, 6.0, -3.0)\n");
    /* 検証: v1×v2 = (2*6-3*5, 3*4-1*6, 1*5-2*4) = (-3, 6, -3) */
    printf("\n");
    
    /* ===== 内積のテスト ===== */
    printf("【5】内積の計算\n");
    double dot = vector_dot(v1, v2);
    printf("v1 · v2 = %.6f\n", dot);
    printf("期待値: %.6f\n", 1.0*4.0 + 2.0*5.0 + 3.0*6.0);
    printf("\n");
    
    /* ===== 直交性の確認 ===== */
    printf("【6】外積の性質確認（直交性）\n");
    printf("v1 · (v1×v2) = %.6f (0であるべき)\n", vector_dot(v1, v_cross));
    printf("v2 · (v1×v2) = %.6f (0であるべき)\n", vector_dot(v2, v_cross));
    printf("\n");
    
    /* ===== 行列演算のテスト ===== */
    printf("【7】単位行列の生成\n");
    Matrix3x3 I = matrix_identity();
    matrix_print("I", I);
    printf("\n");
    
    printf("【8】行列とベクトルの積\n");
    Vector3D v_result = matrix_vector_multiply(I, v1);
    vector_print("I × v1", v_result);
    printf("期待値: v1と同じ\n");
    printf("\n");
    
    /* ===== 回転行列のテスト ===== */
    printf("【9】Y軸周りの90度回転\n");
    double angle = M_PI / 2.0;  /* 90度 */
    Matrix3x3 R_y;
    R_y.m[0][0] = cos(angle);  R_y.m[0][1] = 0.0;  R_y.m[0][2] = sin(angle);
    R_y.m[1][0] = 0.0;         R_y.m[1][1] = 1.0;  R_y.m[1][2] = 0.0;
    R_y.m[2][0] = -sin(angle); R_y.m[2][1] = 0.0;  R_y.m[2][2] = cos(angle);
    matrix_print("R_y(90°)", R_y);
    
    Vector3D z_axis = vector_create(0.0, 0.0, 1.0);
    Vector3D rotated = matrix_vector_multiply(R_y, z_axis);
    vector_print("Z軸を90度回転", rotated);
    printf("期待値: (1.0, 0.0, 0.0) つまりX軸方向\n");
    printf("\n");
    
    /* ===== 転置行列のテスト ===== */
    printf("【10】転置行列\n");
    Matrix3x3 R_y_T = matrix_transpose(R_y);
    matrix_print("R_y^T", R_y_T);
    printf("\n");
    
    printf("【11】逆回転の確認 (R^T × R × v = v)\n");
    Vector3D rotated_back = matrix_vector_multiply(R_y_T, rotated);
    vector_print("元に戻したベクトル", rotated_back);
    printf("期待値: (0.0, 0.0, 1.0) つまり元のZ軸\n");
    printf("\n");
    
    printf("===== テスト完了 =====\n");
    
    return 0;
}