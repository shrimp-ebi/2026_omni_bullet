#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <opencv2/opencv.hpp>

#include <Estimator.h>

using namespace spherical_bullet_time;
namespace fs = std::filesystem;

namespace {
// このサンプルプログラムの基準ディレクトリ。
// 引数なしで起動した場合は、この下にある input.mp4 を読み込む。
const fs::path kProjectDir =
        "/home/y233324/ドキュメント/2026_注視画像生成/Code/2026_omni_bullet/全天球画像でのバレットタイム10";

// 実行ごとの出力フォルダ名に付ける時刻文字列を作る。
// 例: input_20260619_164351
std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&time, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

fs::path make_run_dir(const fs::path& input_video)
{
    // frames: 入力動画から取り出した元フレーム
    // processed_frames: バレットタイム視点へ変換した出力フレーム
    const fs::path run_dir = kProjectDir / "runs" /
            (input_video.stem().string() + "_" + timestamp());
    fs::create_directories(run_dir / "frames");
    fs::create_directories(run_dir / "processed_frames");
    return run_dir;
}

fs::path resolve_input_video(int argc, char** argv)
{
    // コマンドライン引数があればそれを入力動画として使う。
    // 相対パスの場合は kProjectDir からの相対パスとして扱う。
    if(argc >= 2) {
        fs::path input = argv[1];
        if(input.is_relative()) {
            input = kProjectDir / input;
        }
        return input;
    }
    return kProjectDir / "input.mp4";
}
}

// 注視点と参照点の極座標から回転行列を計算する。
// 注視点を新しいX軸、参照点を上下方向を決める手掛かりとして使い、
// 「見たい方向が正面に来る」カメラ姿勢を作る。
cv::Mat calc_R_by2p(const cv::Vec2d& gaze, const cv::Vec2d& ref)
{
    // 極座標系を直交座標系に変換
    auto gaze_3d = Estimator::polar2Cartesian(gaze);
    auto ref_3d = Estimator::polar2Cartesian(ref);

    // 直交座標系の構成：
    // X軸 = 注視方向
    // Z軸 = X軸と参照方向の外積（垂直方向）
    // Y軸 = Z軸とX軸の外積（右手系を構成）
    cv::Vec3d x = gaze_3d;
    cv::Vec3d z = gaze_3d.cross(ref_3d);
    z /= cv::norm(z);  // 正規化
    cv::Vec3d y = z.cross(x);

    // 回転行列を構成（各列が基底ベクトル）
    cv::Mat R(3, 3, CV_64F);
    cv::Mat(3, 1, CV_64F, x.val).copyTo(R.col(0));
    cv::Mat(3, 1, CV_64F, y.val).copyTo(R.col(1));
    cv::Mat(3, 1, CV_64F, z.val).copyTo(R.col(2));

    return R;
}

// オーバーロード版：画像座標を極座標に変換してから回転行列を計算
cv::Mat calc_R_by2p(const cv::Point2i& gaze, const cv::Point2i& ref, cv::Size size)
{
    // 画像座標を球面座標系の極座標に変換
    cv::Vec2d gaze_polar = Estimator::sphericalImg2polar(gaze, size);
    cv::Vec2d ref_polar = Estimator::sphericalImg2polar(ref, size);
    return calc_R_by2p(gaze_polar, ref_polar);
}

int main(int argc, char** argv)
{
    // ============================================
    // 入力動画の設定・初期化
    // ============================================
    const fs::path input_video = resolve_input_video(argc, argv);
    cv::VideoCapture cap(input_video.string());
    if(!cap.isOpened()) {
        std::cerr << "動画ファイルが開けません: " << input_video << std::endl;
        return 1;
    }

    // 出力ディレクトリの作成
    const fs::path run_dir = make_run_dir(input_video);
    const fs::path frames_dir = run_dir / "frames";
    const fs::path processed_dir = run_dir / "processed_frames";
    const fs::path debug_dir = run_dir / "debug";
    const fs::path csv_path = run_dir / "data.csv";
    const fs::path movie_path = run_dir / "processed.mp4";
    fs::create_directories(debug_dir);

    // 環境変数の設定（Estimatorクラスで使用）。
    // Estimator 側は出力先を直接知らないため、CSVとデバッグ画像の保存先だけここから渡す。
    setenv("SPHERICAL_BT_CSV_PATH", csv_path.string().c_str(), 1);
    setenv("SPHERICAL_BT_DEBUG_DIR", debug_dir.string().c_str(), 1);

    // 出力FPSの設定
    const double input_fps = cap.get(cv::CAP_PROP_FPS);
    const double output_fps = input_fps > 0.0 ? input_fps : 30.0;

    // 入力動画の情報を表示
    std::cout << "input: " << input_video << std::endl;
    std::cout << "run_dir: " << run_dir << std::endl;
    std::cout << "width: " << static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH)) << std::endl;
    std::cout << "height: " << static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)) << std::endl;
    std::cout << "fps: " << output_fps << std::endl;

    // ============================================
    // バレットタイム処理のパラメータ設定
    // ============================================
    const double focus = 1;              // 仮想透視カメラの焦点距離。値が大きいほど画角が狭くなる。
    const double div = 8;                // 出力クリップの大きさ。元画像の幅・高さをこの値で割る。
    const double div_ = 2;               // 推定に使う比較範囲。出力クリップよりさらに小さくする。
    const int count_init = 0;            // 初期姿勢を手で与えるフレーム番号。

    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    // 処理に使用する変数の初期化
    cv::Size clip_size;
    cv::Size comparison_range;
    std::unique_ptr<Estimator> estimator;
    cv::Mat R;                           // 現在フレームで推定された「仮想カメラの向き」。
    cv::Mat target_img;                  // 現在のRで全天球画像全体を回転したグレースケール画像。
    cv::Mat match_img;                   // 初期フレーム中央から切り出した、追跡対象の小窓。
    cv::VideoWriter writer;

    // ============================================
    // フレーム処理のメインループ
    // ============================================
    int count = 0;
    while(true) {
        // フレームの読み込み
        cv::Mat frame_color;
        cap.read(frame_color);
        if(frame_color.empty()) {
            break;  // 動画終了
        }

        // フレームの保存
        const auto frame_name = std::to_string(count) + ".png";
        cv::imwrite((frames_dir / frame_name).string(), frame_color);

        // グレースケールに変換
        cv::Mat frame_gray;
        cv::cvtColor(frame_color, frame_gray, cv::COLOR_BGR2GRAY);

        std::chrono::system_clock::time_point lap_start = std::chrono::system_clock::now();

        if(count == count_init) {
            // ========================================
            // フレーム0: 初期化処理
            // ========================================
            // クリップサイズと比較範囲の計算
            clip_size = cv::Size(frame_gray.cols / div, frame_gray.rows / div);
            comparison_range = cv::Size(frame_gray.cols / div / div_, frame_gray.rows / div / div_);
            estimator = std::make_unique<Estimator>(frame_gray.size(), comparison_range, clip_size, focus);

            // 注視点と参照点の指定（画像座標）。
            // gaze がバレットタイム映像の中心に来る点、ref は回転の傾き（ロール）を決める補助点。
            cv::Point2i gaze(2477, 1504);    // 注視点の座標
            cv::Point2i ref(2781, 1491);     // 参照点の座標

            // 注視点と参照点から回転行列を計算
            R = calc_R_by2p(gaze, ref, frame_gray.size());
            target_img = estimator->rotate_img2(frame_gray, R, frame_gray.size());
            // 初期フレームをRで正面化したあと、中央の小窓を基準画像として保存する。
            // 以降のフレームは、この小窓と一致するようにRを更新していく。
            match_img = estimator->rotate_comparison_range2(target_img);
            cv::imwrite((run_dir / "initial_clip.png").string(), match_img);
        } else {
            // ========================================
            // フレーム1以降: 回転推定と画像回転
            // ========================================
            // Levenberg-Marquardt法を使用して回転行列を推定する。
            // 前フレームのRを初期値にすることで、動画内の連続した動きを追跡する。
            R = estimator->estimate_R(frame_gray, target_img, R);
            // グレースケール画像を回転
            target_img = estimator->rotate_img2(frame_gray, R, frame_gray.size());
        }

        // カラー画像を回転させてバレットタイム画像を生成
        cv::Mat output_img = estimator->rotate_clip(frame_color, R);
        cv::imwrite((processed_dir / frame_name).string(), output_img);

        // 出力動画ライターの初期化（フレーム0で実行）
        if(!writer.isOpened()) {
            writer.open(
                    movie_path.string(),
                    cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                    output_fps,
                    output_img.size(),
                    true);
            if(!writer.isOpened()) {
                std::cerr << "動画出力を開けません: " << movie_path << std::endl;
                return 1;
            }
        }

        // 処理済みフレームを出力動画に書き込む
        writer << output_img;

        // 処理時間の表示（フレーム0と30フレームごと）
        std::chrono::system_clock::time_point lap_end = std::chrono::system_clock::now();
        if(count == 0 || count % 30 == 0) {
            std::cout << "frame " << count << ": "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(lap_end - lap_start).count()
                      << "ms" << std::endl;
        }
        count++;
    }

    // ============================================
    // 処理完了・結果表示
    // ============================================
    std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
    std::cout << "processed frames: " << count << std::endl;
    std::cout << "movie: " << movie_path << std::endl;
    std::cout << "elapsed: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()
              << "ms" << std::endl;

    return 0;
}
