/* test_rotation.c
 * rotation.cの動作確認
 */

#include <stdio.h>
#include <math.h>
#include "rotation.h"
#include "coord_transform.h"
#include "vector_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    printf("===== 回転行列計算のテスト =====\n\n");

    int W = 6080;
    int H = 3040;

    /* ===== テスト1: 画像中心が注視点の場合 ===== */
    printf("【テスト1】画像中心を注視点とする場合\n");
    printf("画像サイズ: W=%d, H=%d\n\n", W, H);

    int u_g1 = W / 2;
    int v_g1 = H / 2;
    printf("注視点(u_g, v_g) = (%d, %d)\n\n", u_g1, v_g1);

    Vector3D G1 = image_to_world(u_g1, v_g1, W, H);
    vector_print("G (注視点)", G1);

    Matrix3x3 R1 = compute_rotation_matrix(G1);

    rotation_matrix_info(R1);
    rotation_matrix_verify(R1);

    printf("\n期待される結果:\n");
    printf("  ez ≈ (0, 0, 1) : Z軸正方向（画像中心）\n");

    /* ===== テスト2: 画像左端を注視点とする場合 ===== */
    printf("\n\n=====================================\n\n");
    printf("【テスト2】画像左端中央を注視点とする場合\n\n");

    int u_g2 = 100;
    int v_g2 = H / 2;
    printf("注視点(u_g, v_g) = (%d, %d)\n\n", u_g2, v_g2);

    Vector3D G2 = image_to_world(u_g2, v_g2, W, H);
    vector_print("G (注視点)", G2);

    Matrix3x3 R2 = compute_rotation_matrix(G2);

    rotation_matrix_info(R2);
    rotation_matrix_verify(R2);

    /* ===== テスト3: 回転の動作確認 ===== */
    printf("\n\n=====================================\n\n");
    printf("【テスト3】回転の動作確認\n\n");

    printf("回転前の注視点Gを回転行列Rで回転:\n");
    vector_print("G", G2);

    Vector3D G2_rotated = matrix_vector_multiply(R2, G2);
    vector_print("R × G", G2_rotated);
    printf("期待値: (0, 0, 1) つまりZ軸正方向\n");
    printf("(回転後、注視点が画像中心方向を向く)\n\n");

    printf("逆回転で元に戻すテスト:\n");
    Matrix3x3 R2_T = matrix_transpose(R2);
    Vector3D G2_back = matrix_vector_multiply(R2_T, G2_rotated);
    vector_print("R^T × (R × G)", G2_back);
    vector_print("元のG", G2);
    printf("(ほぼ一致するはず)\n");

    /* ===== テスト4: 特異点に近いケース（天頂方向） ===== */
    printf("\n\n=====================================\n\n");
    printf("【テスト4】特異点に近いケース（upと光軸が平行）\n\n");

    Vector3D G_pole = vector_create(0.0, 1.0, 0.0);
    vector_print("G_pole", G_pole);

    Matrix3x3 R_pole = compute_rotation_matrix(G_pole);
    rotation_matrix_info(R_pole);
    rotation_matrix_verify(R_pole);

    printf("\n===== テスト完了 =====\n");

    return 0;
}
