/* objective_3param.c
 * 3パラメータ回転の目的関数・微分計算
 * image_derivative_theta_phi_central を使用
 */

#include "objective_3param.h"
#include "../include/coord_transform.h"
#include "../include/rotation.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static Vector3D dXprime_domega(Matrix3x3 R, Vector3D X, int k) {
    Vector3D result;
    double Xx = X.x, Xy = X.y, Xz = X.z;

    double R1x = R.m[0][0], R1y = R.m[0][1], R1z = R.m[0][2];
    double R2x = R.m[1][0], R2y = R.m[1][1], R2z = R.m[1][2];
    double R3x = R.m[2][0], R3y = R.m[2][1], R3z = R.m[2][2];

    if (k == 0) {
        result.x = 0.0;
        result.y = -(R3x*Xx + R3y*Xy + R3z*Xz);
        result.z =   R2x*Xx + R2y*Xy + R2z*Xz;
    } else if (k == 1) {
        result.x =   R3x*Xx + R3y*Xy + R3z*Xz;
        result.y = 0.0;
        result.z = -(R1x*Xx + R1y*Xy + R1z*Xz);
    } else {
        result.x = -(R2x*Xx + R2y*Xy + R2z*Xz);
        result.y =   R1x*Xx + R1y*Xy + R1z*Xz;
        result.z = 0.0;
    }
    return result;
}

double compute_objective_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max)
{
    int W = base->width;
    int H = base->height;
    double sum = 0.0;
    int count = 0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {
            Vector3D X  = image_to_world(u, v, W, H);
            Vector3D Xp = matrix_vector_multiply(R, X);
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            uint8_t rgb_b[3];
            get_pixel(base, u, v, rgb_b);
            double Sb = (rgb_b[0] + rgb_b[1] + rgb_b[2]) / 3.0;
            double Sr = image_gray_bilinear(ref, u_ref, v_ref);

            double diff = Sr - Sb;
            sum += diff * diff;
            count++;
        }
    }
    return (count > 0) ? sum / (2.0 * count) : 0.0;
}

void compute_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double grad[3])
{
    int W = base->width;
    int H = base->height;
    double sum[3] = {0.0, 0.0, 0.0};
    int count = 0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {
            Vector3D X  = image_to_world(u, v, W, H);
            Vector3D Xp = matrix_vector_multiply(R, X);
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            uint8_t rgb_b[3];
            get_pixel(base, u, v, rgb_b);
            double Sb = (rgb_b[0] + rgb_b[1] + rgb_b[2]) / 3.0;
            double Sr = image_gray_bilinear(ref, u_ref, v_ref);
            double diff = Sr - Sb;

            double dS_dtheta, dS_dphi;
            image_derivative_theta_phi_central(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);

            double theta_p, phi_p;
            world_to_angle(Xp, &theta_p, &phi_p);
            double dth_dX, dth_dY, dth_dZ;
            double dph_dX, dph_dY, dph_dZ;
            angle_jacobian_xyz(theta_p, phi_p,
                               &dth_dX, &dth_dY, &dth_dZ,
                               &dph_dX, &dph_dY, &dph_dZ);

            double dSr_dXp = dS_dtheta * dth_dX + dS_dphi * dph_dX;
            double dSr_dYp = dS_dtheta * dth_dY + dS_dphi * dph_dY;
            double dSr_dZp = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;

            for (int k = 0; k < 3; k++) {
                Vector3D dXp_domk = dXprime_domega(R, X, k);
                double chain = dSr_dXp * dXp_domk.x
                             + dSr_dYp * dXp_domk.y
                             + dSr_dZp * dXp_domk.z;
                sum[k] += diff * chain;
            }
            count++;
        }
    }

    for (int k = 0; k < 3; k++)
        grad[k] = (count > 0) ? sum[k] / (double)count : 0.0;
}

void compute_hessian_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int u_min, int v_min, int u_max, int v_max,
    double hessian[3][3])
{
    int W = base->width;
    int H = base->height;
    double sum[3][3];
    int count = 0;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            sum[i][j] = 0.0;

    for (int v = v_min; v <= v_max; v++) {
        for (int u = u_min; u <= u_max; u++) {
            Vector3D X  = image_to_world(u, v, W, H);
            Vector3D Xp = matrix_vector_multiply(R, X);
            double u_ref, v_ref;
            world_to_image(Xp, W, H, &u_ref, &v_ref);

            double dS_dtheta, dS_dphi;
            image_derivative_theta_phi_central(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);

            double theta_p, phi_p;
            world_to_angle(Xp, &theta_p, &phi_p);
            double dth_dX, dth_dY, dth_dZ;
            double dph_dX, dph_dY, dph_dZ;
            angle_jacobian_xyz(theta_p, phi_p,
                               &dth_dX, &dth_dY, &dth_dZ,
                               &dph_dX, &dph_dY, &dph_dZ);

            double dSr_dXp = dS_dtheta * dth_dX + dS_dphi * dph_dX;
            double dSr_dYp = dS_dtheta * dth_dY + dS_dphi * dph_dY;
            double dSr_dZp = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;

            double j[3];
            for (int k = 0; k < 3; k++) {
                Vector3D dXp = dXprime_domega(R, X, k);
                j[k] = dSr_dXp * dXp.x
                      + dSr_dYp * dXp.y
                      + dSr_dZp * dXp.z;
            }

            for (int k = 0; k < 3; k++)
                for (int l = 0; l < 3; l++)
                    sum[k][l] += j[k] * j[l];

            count++;
        }
    }

    for (int k = 0; k < 3; k++)
        for (int l = 0; l < 3; l++)
            hessian[k][l] = (count > 0) ? sum[k][l] / (double)count : 0.0;
}

double compute_numerical_gradient_3param(
    Image *base, Image *ref, Matrix3x3 R,
    int param_idx, double delta,
    int u_min, int v_min, int u_max, int v_max)
{
    double omega_plus[3]  = {0.0, 0.0, 0.0};
    double omega_minus[3] = {0.0, 0.0, 0.0};
    omega_plus[param_idx]  = +delta;
    omega_minus[param_idx] = -delta;
    Matrix3x3 R_plus  = matrix_multiply(rodrigues(omega_plus),  R);
    Matrix3x3 R_minus = matrix_multiply(rodrigues(omega_minus), R);
    double Ep = compute_objective_3param(base, ref, R_plus,  u_min, v_min, u_max, v_max);
    double Em = compute_objective_3param(base, ref, R_minus, u_min, v_min, u_max, v_max);
    return (Ep - Em) / (2.0 * delta);
}

Matrix3x3 rodrigues(double omega[3]) {
    double theta = sqrt(omega[0]*omega[0] + omega[1]*omega[1] + omega[2]*omega[2]);
    if (theta < 1e-12)
        return matrix_identity();

    double n1 = omega[0] / theta;
    double n2 = omega[1] / theta;
    double n3 = omega[2] / theta;
    double c  = cos(theta);
    double s  = sin(theta);
    double mc = 1.0 - c;

    Matrix3x3 R;
    R.m[0][0] = n1*n1*mc + c;      R.m[0][1] = n1*n2*mc - n3*s;  R.m[0][2] = n1*n3*mc + n2*s;
    R.m[1][0] = n2*n1*mc + n3*s;   R.m[1][1] = n2*n2*mc + c;     R.m[1][2] = n2*n3*mc - n1*s;
    R.m[2][0] = n3*n1*mc - n2*s;   R.m[2][1] = n3*n2*mc + n1*s;  R.m[2][2] = n3*n3*mc + c;
    return R;
}

int solve_linear_3x3(double A[3][3], double b[3], double x[3]) {
    double aug[3][4];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            aug[i][j] = A[i][j];
        aug[i][3] = b[i];
    }

    for (int col = 0; col < 3; col++) {
        int pivot = col;
        for (int row = col + 1; row < 3; row++)
            if (fabs(aug[row][col]) > fabs(aug[pivot][col]))
                pivot = row;

        if (pivot != col)
            for (int j = 0; j < 4; j++) {
                double tmp = aug[col][j];
                aug[col][j] = aug[pivot][j];
                aug[pivot][j] = tmp;
            }

        if (fabs(aug[col][col]) < 1e-12) {
            x[0] = x[1] = x[2] = 0.0;
            return -1;
        }

        for (int row = col + 1; row < 3; row++) {
            double factor = aug[row][col] / aug[col][col];
            for (int j = col; j < 4; j++)
                aug[row][j] -= factor * aug[col][j];
        }
    }

    for (int i = 2; i >= 0; i--) {
        x[i] = aug[i][3];
        for (int j = i + 1; j < 3; j++)
            x[i] -= aug[i][j] * x[j];
        x[i] /= aug[i][i];
    }
    return 0;
}
