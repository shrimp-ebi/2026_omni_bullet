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

if __name__ == '__main__':
    print('Load lib:', lib_path)
