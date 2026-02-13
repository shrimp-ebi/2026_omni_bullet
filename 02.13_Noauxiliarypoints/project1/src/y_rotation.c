/* y_rotation.c
 * Y軸回りの回転専用関数の実装
 */

#include "y_rotation.h"
#include "coord_transform.h"
#include "vector_math.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 度数法からラジアンへの変換 */
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)

static inline double gray_at(Image *img, double u, double v) {
  uint8_t rgb[3];
  get_pixel_bilinear(img, u, v, rgb);
  return (rgb[0] + rgb[1] + rgb[2]) / 3.0;
}

/* 参照画像上での ∂S/∂θ, ∂S/∂φ を計算（画像差分→角度差で割る） */
static void ref_image_derivative_theta_phi(Image *ref, double u, double v,
                                           double *dS_dtheta, double *dS_dphi) {
  int W = ref->width;
  int H = ref->height;

  const double dtheta = 2.0 * M_PI / (double)W;
  const double dphi = M_PI / (double)H;

  /* θ方向（u方向差分、横は周期） */
  double S_u_plus = gray_at(ref, u + 1.0, v);
  double S_u_minus = gray_at(ref, u - 1.0, v);
  *dS_dtheta = (S_u_plus - S_u_minus) / (2.0 * dtheta);

  /* φ方向（v方向差分）
     あなたのphi定義は phi = (H - v)*pi/H なので、dv/dphi = -H/pi = -1/dphi */
  double S_v_plus = gray_at(ref, u, v + 1.0);
  double S_v_minus = gray_at(ref, u, v - 1.0);
  double dS_dv = (S_v_plus - S_v_minus) / 2.0;
  *dS_dphi = dS_dv * (-1.0 / dphi);
}

/* ∂θ/∂X, ∂φ/∂X など（資料の式） */
static void dtheta_dphi_dXYZ(double theta, double phi, double *dtheta_dX,
                             double *dtheta_dY, double *dtheta_dZ,
                             double *dphi_dX, double *dphi_dY,
                             double *dphi_dZ) {
  double sinphi = sin(phi);
  double costh = cos(theta);
  double sinth = sin(theta);
  double cosphi = cos(phi);

  /* 極（sinphi≈0）で発散するので安全対策 */
  if (fabs(sinphi) < 1e-8)
    sinphi = (sinphi >= 0 ? 1e-8 : -1e-8);

  *dtheta_dX = costh / sinphi;
  *dtheta_dY = 0.0;
  *dtheta_dZ = -sinth / sinphi;

  *dphi_dX = cosphi * sinth;
  *dphi_dY = -sinphi;
  *dphi_dZ = cosphi * costh;
}

/* ===========================
 * Y軸回りの回転行列
 * =========================== */

Matrix3x3 create_y_rotation_matrix(double psi_deg) {
  double psi = DEG_TO_RAD(psi_deg);
  double cos_psi = cos(psi);
  double sin_psi = sin(psi);

  Matrix3x3 R;

  /* 式(9): Y軸回りの回転行列
   *
   * R(Y)(ψ) = [ cos(ψ)   0  -sin(ψ) ]
   *           [   0      1     0     ]
   *           [ sin(ψ)   0   cos(ψ) ]
   */
  R.m[0][0] = cos_psi;
  R.m[0][1] = 0.0;
  R.m[0][2] = -sin_psi;
  R.m[1][0] = 0.0;
  R.m[1][1] = 1.0;
  R.m[1][2] = 0.0;
  R.m[2][0] = sin_psi;
  R.m[2][1] = 0.0;
  R.m[2][2] = cos_psi;

  return R;
}

/* ===========================
 * 回転画像の生成
 * =========================== */

Image *rotate_image_y_axis(Image *input, double psi_deg) {
  if (!input) {
    fprintf(stderr, "エラー: 入力画像がNULL\n");
    return NULL;
  }

  printf("Y軸回りに%.2f度回転させた画像を生成中...\n", psi_deg);

  int W = input->width;
  int H = input->height;

  /* 回転行列を計算 */
  Matrix3x3 Rinv = create_y_rotation_matrix(-psi_deg);   // 左に5度戻した位置

  /* 出力画像を作成 */
  Image *output = image_create_like(input);
  if (!output) {
    fprintf(stderr, "エラー: 出力画像の作成失敗\n");
    return NULL;
  }

  /* 進捗表示用 */
  int progress_step = H / 10;
  printf("  処理中");
  fflush(stdout);

  /* 出力画像の全画素について処理 */
  for (int v_out = 0; v_out < H; v_out++) {
    if (v_out % progress_step == 0) {
      printf(".");
      fflush(stdout);
    }

    for (int u_out = 0; u_out < W; u_out++) {
      /* 1. 出力画素を世界座標に変換 */
      Vector3D X_prime = image_to_world(u_out, v_out, W, H);

      /* 2. 回転: X = R × X' */
      Vector3D X = matrix_vector_multiply(Rinv, X_prime);

      /* 3. 世界座標を画像座標に変換 */
      double u_in, v_in;
      world_to_image(X, W, H, &u_in, &v_in);

      /* 4. バイリニア補間で画素値を取得 */
      uint8_t rgb[3];
      get_pixel_bilinear(input, u_in, v_in, rgb);

      /* 5. 出力画像に設定 */
      set_pixel(output, u_out, v_out, rgb);
    }
  }

  printf(" 完了！\n");

  return output;
}

/* ===========================
 * 目的関数の計算
 * =========================== */

double compute_objective_function(Image *base, Image *ref, double psi_deg,
                                  int u_min, int v_min, int u_max, int v_max) {
  int W = base->width;
  int H = base->height;

  /* 回転行列を計算 */
  Matrix3x3 R = create_y_rotation_matrix(psi_deg);

  double sum = 0.0;
  int count = 0;

  /* 比較領域内の全画素について */
  for (int v = v_min; v <= v_max; v++) {
    for (int u = u_min; u <= u_max; u++) {
      /* 基準画像の点を世界座標に変換 */
      Vector3D X = image_to_world(u, v, W, H);

      /* Y軸回りに回転 */
      Vector3D X_prime = matrix_vector_multiply(R, X);

      /* 参照画像の座標に変換 */
      double u_ref, v_ref;
      world_to_image(X_prime, W, H, &u_ref, &v_ref);

      /* 両画像の画素値を取得 */
      uint8_t rgb_base[3], rgb_ref[3];
      get_pixel(base, u, v, rgb_base);
      get_pixel_bilinear(ref, u_ref, v_ref, rgb_ref);

      /* グレースケール変換（簡易版） */
      double gray_base = (rgb_base[0] + rgb_base[1] + rgb_base[2]) / 3.0;
      double gray_ref = (rgb_ref[0] + rgb_ref[1] + rgb_ref[2]) / 3.0;

      /* 差の2乗を加算 */
      double diff = gray_ref - gray_base;
      sum += diff * diff;
      count++;
    }
  }

  /* 式(14): E(ψ) = (1/2N) Σ (Sr - Sb)² */
  return sum / (2.0 * count);
}

/* ===========================
 * 微分の計算
 * =========================== */

double compute_analytical_derivative(Image *base, Image *ref, double psi_deg,
                                     int u_min, int v_min, int u_max,
                                     int v_max) {
  int W = base->width;
  int H = base->height;

  /* 回転角度と三角関数 */
  double psi = DEG_TO_RAD(psi_deg);
  double cos_psi = cos(psi);
  double sin_psi = sin(psi);

  /* 回転行列 */
  Matrix3x3 R = create_y_rotation_matrix(psi_deg);

  double sum = 0.0;
  int count = 0;

  for (int v = v_min; v <= v_max; v++) {
    for (int u = u_min; u <= u_max; u++) {

      /* 基準画像の点（球面上） */
      Vector3D X = image_to_world(u, v, W, H);

      /* 回転後の点 */
      Vector3D X_prime = matrix_vector_multiply(R, X);

      /* 回転後点を参照画像の(u,v)へ */
      double u_ref, v_ref;
      world_to_image(X_prime, W, H, &u_ref, &v_ref);

      /* 画素値 */
      uint8_t rgb_base[3], rgb_ref[3];
      get_pixel(base, u, v, rgb_base);
      get_pixel_bilinear(ref, u_ref, v_ref, rgb_ref);

      double gray_base = (rgb_base[0] + rgb_base[1] + rgb_base[2]) / 3.0;
      double gray_ref = (rgb_ref[0] + rgb_ref[1] + rgb_ref[2]) / 3.0;

      /* diff = Sr - Sb */
      double diff = gray_ref - gray_base;

      /* ===== dX'/dψ, dY'/dψ, dZ'/dψ （式11, 12, 13） =====
         X' =  X cosψ - Z sinψ
         Y' =  Y
         Z' =  X sinψ + Z cosψ
      */
      double dX_dpsi = -X.x * sin_psi - X.z * cos_psi;
      double dY_dpsi = 0.0;
      double dZ_dpsi = X.x * cos_psi - X.z * sin_psi;

      /* ===== 参照画像の微分：∂S/∂θ, ∂S/∂φ ===== */
      double dS_dtheta, dS_dphi;
      ref_image_derivative_theta_phi(ref, u_ref, v_ref, &dS_dtheta, &dS_dphi);

      /* ===== ∂θ/∂X', ∂φ/∂X' など ===== */
      double theta_p, phi_p;
      world_to_angle(X_prime, &theta_p, &phi_p);

      double dth_dX, dth_dY, dth_dZ;
      double dph_dX, dph_dY, dph_dZ;
      dtheta_dphi_dXYZ(theta_p, phi_p, &dth_dX, &dth_dY, &dth_dZ, &dph_dX,
                       &dph_dY, &dph_dZ);

      /* ===== 連鎖律で ∂S/∂X', ∂S/∂Y', ∂S/∂Z' ===== */
      double dSr_dX = dS_dtheta * dth_dX + dS_dphi * dph_dX;
      double dSr_dY = dS_dtheta * dth_dY + dS_dphi * dph_dY;
      double dSr_dZ = dS_dtheta * dth_dZ + dS_dphi * dph_dZ;

      /* ===== 式: dE/dψ = (1/N) Σ (Sr-Sb)*(...) ===== */
      double chain_rule =
          dSr_dX * dX_dpsi + dSr_dY * dY_dpsi + dSr_dZ * dZ_dpsi;

      sum += diff * chain_rule;
      count++;
    }
  }

  return sum / (double)count;
}

double compute_numerical_derivative(Image *base, Image *ref, double psi_deg,
                                    double delta_psi, int u_min, int v_min,
                                    int u_max, int v_max) {
  /* 数値微分: dE/dψ ≈ (E(ψ + Δψ) - E(ψ)) / Δψ
   * 
   * 注意: 理論微分との単位を合わせるため、ラジアンで微分する
   * delta_psi は度で与えられるが、微分値は [エネルギー/ラジアン] で返す
   */

  /* delta_psiを度からラジアンに変換 */
  double delta_psi_rad = delta_psi * M_PI / 180.0;

  double E_psi = compute_objective_function(base, ref, psi_deg, u_min, v_min,
                                            u_max, v_max);

  double E_psi_delta = compute_objective_function(
      base, ref, psi_deg + delta_psi, u_min, v_min, u_max, v_max);

  /* ラジアンで割る */
  return (E_psi_delta - E_psi) / delta_psi_rad;
}