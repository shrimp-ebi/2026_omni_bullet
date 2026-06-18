/* objective_3param.h
 * 3パラメータ回転の目的関数・微分計算
 */

#ifndef OBJECTIVE_3PARAM_H
#define OBJECTIVE_3PARAM_H

#include "../include/image_utils.h"
#include "../include/vector_math.h"

double compute_objective_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max
);

void compute_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double grad[3]
);

void compute_hessian_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double hessian[3][3]
);

double compute_numerical_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int param_idx, double delta,
    int u_min, int v_min, int u_max, int v_max
);

Matrix3x3 rodrigues(double omega[3]);

int solve_linear_3x3(double A[3][3], double b[3], double x[3]);

#endif /* OBJECTIVE_3PARAM_H */
