import ctypes
import os
from ctypes import c_char_p, c_int, c_double, POINTER

lib_path = os.path.join(os.path.dirname(__file__), '..', 'build', 'libomnigaze.so')
lib_path = os.path.normpath(lib_path)
lib = ctypes.CDLL(lib_path)

# int lm_estimate_frame_file(const char *base_path, const char *ref_path,
#                           const double R_init[9], double sigma,
#                           double R_out[9]);
lib.lm_estimate_frame_file.argtypes = [c_char_p, c_char_p, POINTER(c_double), c_double, POINTER(c_double)]
lib.lm_estimate_frame_file.restype = c_int

# int generate_gaze_frame_file(const char *in_path, const char *out_path,
#                              int u_g, int v_g, const double R_cumulative[9]);
lib.generate_gaze_frame_file.argtypes = [c_char_p, c_char_p, c_int, c_int, POINTER(c_double)]
lib.generate_gaze_frame_file.restype = c_int

# int generate_gaze_frame_world_file(const char *in_path, const char *out_path,
#                                    double Gx, double Gy, double Gz,
#                                    const double R_cumulative[9]);
lib.generate_gaze_frame_world_file.argtypes = [c_char_p, c_char_p, c_double, c_double, c_double, POINTER(c_double)]
lib.generate_gaze_frame_world_file.restype = c_int

# int generate_gaze_full_world_file(const char *in_path, const char *out_path,
#                                   double Gx, double Gy, double Gz,
#                                   const double R_cumulative[9]);
lib.generate_gaze_full_world_file.argtypes = [c_char_p, c_char_p, c_double, c_double, c_double, POINTER(c_double)]
lib.generate_gaze_full_world_file.restype = c_int

def lm_estimate_frame(base_path, ref_path, R_init, sigma=3.0):
    R_init_arr = (c_double * 9)(*R_init)
    R_out = (c_double * 9)()
    ret = lib.lm_estimate_frame_file(base_path.encode(), ref_path.encode(), R_init_arr, float(sigma), R_out)
    if ret != 0:
        raise RuntimeError('lm_estimate_frame_file failed')
    return list(R_out)

def generate_gaze_frame(in_path, out_path, u_g, v_g, R_cumulative):
    R_arr = (c_double * 9)(*R_cumulative)
    ret = lib.generate_gaze_frame_file(in_path.encode(), out_path.encode(), int(u_g), int(v_g), R_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_frame_file failed')
    return True

def generate_gaze_frame_world(in_path, out_path, Gx, Gy, Gz, R_cumulative):
    R_arr = (c_double * 9)(*R_cumulative)
    ret = lib.generate_gaze_frame_world_file(in_path.encode(), out_path.encode(), float(Gx), float(Gy), float(Gz), R_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_frame_world_file failed')
    return True

def generate_gaze_full_world(in_path, out_path, Gx, Gy, Gz, R_cumulative):
    R_arr = (c_double * 9)(*R_cumulative)
    ret = lib.generate_gaze_full_world_file(in_path.encode(), out_path.encode(), float(Gx), float(Gy), float(Gz), R_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_full_world_file failed')
    return True

# ============================================================
# 逐次追跡型 API（修正版と同等）
# ============================================================

# int get_initial_R_file(double Gx, double Gy, double Gz, double R_out[9]);
lib.get_initial_R_file.argtypes = [c_double, c_double, c_double, POINTER(c_double)]
lib.get_initial_R_file.restype = c_int

# int generate_gaze_sequential_file(const char *in_path, const char *out_path,
#                                    const double R[9]);
lib.generate_gaze_sequential_file.argtypes = [c_char_p, c_char_p, POINTER(c_double)]
lib.generate_gaze_sequential_file.restype = c_int

def get_initial_R(Gx, Gy, Gz):
    """注視点 G の世界座標から初期回転行列 R_0 を計算する (9要素リスト, row-major)。
    R_0 は列に [ex, ey, ez] を格納 → R_0 · e_z = G が成立。"""
    R_out = (c_double * 9)()
    ret = lib.get_initial_R_file(float(Gx), float(Gy), float(Gz), R_out)
    if ret != 0:
        raise RuntimeError('get_initial_R_file failed')
    return list(R_out)

def generate_gaze_sequential(in_path, out_path, R):
    """逐次追跡用の注視画像生成（修正版 generate_gaze_frame と同等）。
    逆マッピング X = R · Xp でフルサイズ W×H を出力する。"""
    R_arr = (c_double * 9)(*R)
    ret = lib.generate_gaze_sequential_file(in_path.encode(), out_path.encode(), R_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_sequential_file failed')
    return True

# ============================================================
# 2点方式 API
# ============================================================

# int calc_R_2points_file(const char *img_path,
#                         int u_gaze, int v_gaze, int u_ref, int v_ref,
#                         double R_out[9]);
lib.calc_R_2points_file.argtypes = [c_char_p, c_int, c_int, c_int, c_int, POINTER(c_double)]
lib.calc_R_2points_file.restype = c_int

# int generate_gaze_full_R0_file(const char *in_path, const char *out_path,
#                                 const double R_0_param[9], const double R_cumulative[9]);
lib.generate_gaze_full_R0_file.argtypes = [c_char_p, c_char_p, POINTER(c_double), POINTER(c_double)]
lib.generate_gaze_full_R0_file.restype = c_int

# int generate_gaze_frame_R0_file(const char *in_path, const char *out_path,
#                                  const double R_0_param[9], const double R_cumulative[9]);
lib.generate_gaze_frame_R0_file.argtypes = [c_char_p, c_char_p, POINTER(c_double), POINTER(c_double)]
lib.generate_gaze_frame_R0_file.restype = c_int

def calc_R_2points(img_path, u_gaze, v_gaze, u_ref, v_ref):
    """注視点と補助点の画像座標から初期回転行列 R_0 を計算する(9要素リスト)。"""
    R_out = (c_double * 9)()
    ret = lib.calc_R_2points_file(img_path.encode(), int(u_gaze), int(v_gaze),
                                   int(u_ref), int(v_ref), R_out)
    if ret != 0:
        raise RuntimeError('calc_R_2points_file failed')
    return list(R_out)

def generate_gaze_full_R0(in_path, out_path, R_0, R_cumulative):
    """2点方式: フルサイズ Ib 生成。R_0 は calc_R_2points の戻り値。"""
    R_0_arr  = (c_double * 9)(*R_0)
    R_cum_arr = (c_double * 9)(*R_cumulative)
    ret = lib.generate_gaze_full_R0_file(in_path.encode(), out_path.encode(), R_0_arr, R_cum_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_full_R0_file failed')
    return True

def generate_gaze_frame_R0(in_path, out_path, R_0, R_cumulative):
    """2点方式: クリップ注視画像生成。R_0 は calc_R_2points の戻り値。"""
    R_0_arr  = (c_double * 9)(*R_0)
    R_cum_arr = (c_double * 9)(*R_cumulative)
    ret = lib.generate_gaze_frame_R0_file(in_path.encode(), out_path.encode(), R_0_arr, R_cum_arr)
    if ret != 0:
        raise RuntimeError('generate_gaze_frame_R0_file failed')
    return True

if __name__ == '__main__':
    print('Load lib:', lib_path)
