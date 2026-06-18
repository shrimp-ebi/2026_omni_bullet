import os
import sys
import traceback

sys.path.append(os.path.dirname(__file__))
import omnigaze

def main():
    # repo root is two levels up from this script (.. -> 08_Acceleratingvideo, .. -> repo root)
    repo_root = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..'))
    base = os.path.normpath(os.path.join(repo_root, '03.01_3parameters', 'project2', 'images', 'base', 'base.jpg'))
    ref  = os.path.normpath(os.path.join(repo_root, '03.01_3parameters', 'project2', 'images', 'reference', 'reference_30deg.jpg'))
    out = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', 'build', 'test_gaze_out.jpg'))

    print('Base:', base)
    print('Ref :', ref)
    print('Out :', out)

    if not os.path.exists(base) or not os.path.exists(ref):
        print('ERROR: sample images not found. Adjust paths in the script.')
        return 2

    try:
        R_init = [1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0]
        print('Running lm_estimate_frame...')
        R_out = omnigaze.lm_estimate_frame(base, ref, R_init, sigma=3.0)
        print('lm_estimate_frame returned R_out =')
        print(R_out)

        print('Running generate_gaze_frame...')
        # choose center gaze
        u_g = 100
        v_g = 100
        omnigaze.generate_gaze_frame(base, out, u_g, v_g, R_out)
        print('generate_gaze_frame produced', out)
    except Exception as e:
        print('Test failed:')
        traceback.print_exc()
        return 1

    print('Test completed successfully.')
    return 0

if __name__ == '__main__':
    sys.exit(main())
