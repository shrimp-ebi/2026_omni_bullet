/* vector_math.c
 * ベクトルと行列の基本演算の実装
 */

#include <stdio.h>
#include <math.h>
#include "vector_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ===========================
 * ベクトル演算
 * =========================== */

/* ベクトルの生成 */
Vector3D vector_create(double x, double y, double z) {
    Vector3D v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

/* ベクトルのノルム（長さ）を計算 */
double vector_norm(Vector3D v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/* ベクトルの正規化（単位ベクトル化） */
Vector3D vector_normalize(Vector3D v) {
    double norm = vector_norm(v);
    Vector3D result;
    
    if (norm < 1e-10) {
        /* ゼロベクトルの場合はエラー */
        fprintf(stderr, "警告: ゼロベクトルを正規化しようとしています\n");
        result.x = 0.0;
        result.y = 0.0;
        result.z = 0.0;
        return result;
    }
    
    result.x = v.x / norm;
    result.y = v.y / norm;
    result.z = v.z / norm;
    return result;
}

/* 外積: a × b
 * 
 * 計算式:
 *     | i    j    k  |
 * a×b=| ax   ay   az |
 *     | bx   by   bz |
 * 
 * = (ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx)
 */
Vector3D vector_cross(Vector3D a, Vector3D b) {
    Vector3D result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

/* 内積: a · b */
double vector_dot(Vector3D a, Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* ベクトルの表示（デバッグ用） */
void vector_print(const char *name, Vector3D v) {
    printf("%s = (%.6f, %.6f, %.6f)\n", name, v.x, v.y, v.z);
}


/* ===========================
 * 行列演算
 * =========================== */

/* 単位行列の生成 */
Matrix3x3 matrix_identity(void) {
    Matrix3x3 M;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            M.m[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
    return M;
}

/* 行列の積: C = A × B
 * 
 * C[i][j] = Σ(k=0 to 2) A[i][k] * B[k][j]
 */
Matrix3x3 matrix_multiply(Matrix3x3 A, Matrix3x3 B) {
    Matrix3x3 C;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            C.m[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                C.m[i][j] += A.m[i][k] * B.m[k][j];
            }
        }
    }
    
    return C;
}

/* 行列とベクトルの積: v' = M × v
 * 
 * v'[i] = Σ(j=0 to 2) M[i][j] * v[j]
 */
Vector3D matrix_vector_multiply(Matrix3x3 M, Vector3D v) {
    Vector3D result;
    
    result.x = M.m[0][0] * v.x + M.m[0][1] * v.y + M.m[0][2] * v.z;
    result.y = M.m[1][0] * v.x + M.m[1][1] * v.y + M.m[1][2] * v.z;
    result.z = M.m[2][0] * v.x + M.m[2][1] * v.y + M.m[2][2] * v.z;
    
    return result;
}

/* 行列の転置: M^T
 * 
 * M^T[i][j] = M[j][i]
 */
Matrix3x3 matrix_transpose(Matrix3x3 M) {
    Matrix3x3 M_T;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            M_T.m[i][j] = M.m[j][i];
        }
    }
    
    return M_T;
}

/* 行列の表示（デバッグ用） */
void matrix_print(const char *name, Matrix3x3 M) {
    printf("%s =\n", name);
    for (int i = 0; i < 3; i++) {
        printf("  [ %.6f  %.6f  %.6f ]\n", 
               M.m[i][0], M.m[i][1], M.m[i][2]);
    }
}