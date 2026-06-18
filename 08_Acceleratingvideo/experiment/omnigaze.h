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

#ifdef __cplusplus
}
#endif

#endif // OMNIGAZE_H
