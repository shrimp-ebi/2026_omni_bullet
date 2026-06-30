#ifndef OMNIGAZE_H
#define OMNIGAZE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ファイルパスベースの簡易API（まずはファイルI/O版を提供） */
int lm_estimate_frame_file(const char *base_path, const char *ref_path,
                          const double R_init[9], double sigma,
                          double R_out[9]);

int generate_gaze_frame_file(const char *in_path, const char *out_path,
                             int u_g, int v_g, const double R_cumulative[9]);

/* ワールド方向ベースの出力 (u,v ではなく世界ベクトルで指定)
 * Gx,Gy,Gz: 世界方向ベクトル (正規化していなくても可)
 */
int generate_gaze_frame_world_file(const char *in_path, const char *out_path,
                                   double Gx, double Gy, double Gz,
                                   const double R_cumulative[9]);

/* フルサイズ版: 切り出しなしで W×H 全画素を出力する。
 * 回転計算は generate_gaze_frame_world_file と完全に同一。
 * LM 推定の基準画像 Ib 生成用。
 */
int generate_gaze_full_world_file(const char *in_path, const char *out_path,
                                  double Gx, double Gy, double Gz,
                                  const double R_cumulative[9]);

/* ============================================================
 * 逐次追跡型 API（修正版と同等）
 * ============================================================ */

/* 注視点 G の世界座標から初期回転行列 R_0 を計算する。
 * R_0 は列に [ex, ey, ez] を格納（R_0 · e_z = G が成立）。
 * 逐次追跡の第 0 フレームで R_prev の初期値として使用する。
 * R_out[9]: row-major で返す。
 */
int get_initial_R_file(double Gx, double Gy, double Gz, double R_out[9]);

/* 逐次追跡用の注視画像生成（修正版 generate_gaze_frame と同等）。
 * 逆マッピング X = R · Xp でフルサイズ W×H を出力する。
 * R[9]: row-major。列に [ex, ey, ez] を格納 → 中心が注視点を参照。
 */
int generate_gaze_sequential_file(const char *in_path, const char *out_path,
                                   const double R[9]);

/* ============================================================
 * 2点方式 API
 * ============================================================ */

/* 注視点と補助点の画像座標から初期回転行列を生成する(論文式 2.4-2.7)。
 * ez=normalize(G), ey=normalize(G×Gs), ex=ey×ez, R=[ex|ey|ez] (列配置)。
 * R_out[9] は row-major で返す。img_path はサイズ取得のためのみ使用。
 */
int calc_R_2points_file(const char *img_path,
                        int u_gaze, int v_gaze,
                        int u_ref,  int v_ref,
                        double R_out[9]);

/* 2点方式用フルサイズ出力 (Ib 生成専用)。
 * R_0_param: calc_R_2points_file が返す初期回転行列(row-major 9要素)。
 * R_cumulative: LM 推定で得た累積回転行列(初回は単位行列)。
 */
int generate_gaze_full_R0_file(const char *in_path, const char *out_path,
                                const double R_0_param[9],
                                const double R_cumulative[9]);

/* 2点方式用クリップ出力 (各フレーム用)。W/4..3W/4 × H/4..3H/4 を切り出す。 */
int generate_gaze_frame_R0_file(const char *in_path, const char *out_path,
                                 const double R_0_param[9],
                                 const double R_cumulative[9]);

#ifdef __cplusplus
}
#endif

#endif // OMNIGAZE_H
