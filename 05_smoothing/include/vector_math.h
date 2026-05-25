/* vector_math.h
 * ベクトルと行列の基本演算
 */

#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <math.h>

/* 3次元ベクトル構造体 */
typedef struct {
    double x;
    double y;
    double z;
} Vector3D;

/* 3x3行列構造体 */
typedef struct {
    double m[3][3];
} Matrix3x3;


/* ===========================
 * ベクトル演算
 * =========================== */

/* ベクトルの生成 */
Vector3D vector_create(double x, double y, double z);

/* ベクトルのノルム（長さ）を計算 */
double vector_norm(Vector3D v);

/* ベクトルの正規化（単位ベクトル化） */
Vector3D vector_normalize(Vector3D v);

/* 外積: a × b */
Vector3D vector_cross(Vector3D a, Vector3D b);

/* 内積: a · b */
double vector_dot(Vector3D a, Vector3D b);

/* ベクトルの表示（デバッグ用） */
void vector_print(const char *name, Vector3D v);


/* ===========================
 * 行列演算
 * =========================== */

/* 単位行列の生成 */
Matrix3x3 matrix_identity(void);

/* 行列の積: C = A × B */
Matrix3x3 matrix_multiply(Matrix3x3 A, Matrix3x3 B);

/* 行列とベクトルの積: v' = M × v */
Vector3D matrix_vector_multiply(Matrix3x3 M, Vector3D v);

/* 行列の転置: M^T */
Matrix3x3 matrix_transpose(Matrix3x3 M);

/* 行列の表示（デバッグ用） */
void matrix_print(const char *name, Matrix3x3 M);

#endif /* VECTOR_MATH_H */