#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <ctime>

#include <Estimator.h>

namespace fs = std::filesystem;

using namespace spherical_bullet_time;

// 出力ディレクトリ名を生成（タイムスタンプ付き）
std::string create_output_directory(const std::string& base_dir = "./output") {
    auto time_now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(time_now);

    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&time_t));

    std::string output_dir = base_dir + "_" + std::string(timestamp);

    try {
        if (!fs::exists(output_dir)) {
            fs::create_directories(output_dir);
            std::cout << "✓ 出力ディレクトリを作成しました: " << output_dir << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "✗ ディレクトリ作成エラー: " << e.what() << std::endl;
    }

    return output_dir;
}

int expand_mp4(const std::string& video_path, const std::string& output_dir){
    cv::VideoCapture cap(video_path);
    if(!cap.isOpened()){
        std::cerr << "✗ 動画ファイルが開けません: " << video_path << std::endl;
        return 0;
    }

    int total = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    std::cout << "動画展開: "
              << (int)cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT) << "px"
              << "  FPS=" << (int)cap.get(cv::CAP_PROP_FPS);
    if(total > 0) std::cout << "  全" << total << "フレーム";
    std::cout << "\n";

    cv::Mat frame;
    int count = 0;
    while(true){
        cap.read(frame);
        if(frame.empty()){
            std::cout << "\r✓ 展開完了: " << count << " フレーム            \n";
            return count;
        }
        cv::imwrite(output_dir + "/" + std::to_string(count) + ".png", frame);
        if(count % 10 == 0){
            std::cout << "\r  展開中... " << count;
            if(total > 0) std::cout << "/" << total;
            std::cout << " フレーム" << std::flush;
        }
        count++;
    }
}


cv::Mat calc_R_by2p(const cv::Vec2d& gaze, const cv::Vec2d& ref){
    // 注視点と参照点の3次元座標を計算
    auto gaze_3d = Estimator::polar2Cartesian(gaze);
    auto ref_3d = Estimator::polar2Cartesian(ref);

    // 回転後の座標軸を計算
    cv::Vec3d x = gaze_3d;
    cv::Vec3d z = gaze_3d.cross(ref_3d);
    z /= cv::norm(z);
    cv::Vec3d y = z.cross(x);

    //回転行列Rにまとめる
    cv::Mat R(3, 3, CV_64F);

    cv::Mat(3, 1, CV_64F, x.val).copyTo(R.col(0));
    cv::Mat(3, 1, CV_64F, y.val).copyTo(R.col(1));
    cv::Mat(3, 1, CV_64F, z.val).copyTo(R.col(2));

    return R;
}

cv::Mat calc_R_by2p(const cv::Point2i& gaze, const cv::Point2i& ref, cv::Size size){
    // 画像座標系を極座標系に変換
    cv::Vec2d gaze_polar = Estimator::sphericalImg2polar(gaze, size);
    cv::Vec2d ref_polar = Estimator::sphericalImg2polar(ref, size);

    return calc_R_by2p(gaze_polar, ref_polar);
}


int main(int argc, char* argv[])
{
    // コマンドライン引数のデフォルト値
    int gaze_u = 3040, gaze_v = 1520;
    int ref_u  = 3213, ref_v  = 1520;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--gaze-u" && i+1 < argc) { gaze_u = std::stoi(argv[++i]); }
        else if (arg == "--gaze-v" && i+1 < argc) { gaze_v = std::stoi(argv[++i]); }
        else if (arg == "--ref-u"  && i+1 < argc) { ref_u  = std::stoi(argv[++i]); }
        else if (arg == "--ref-v"  && i+1 < argc) { ref_v  = std::stoi(argv[++i]); }
        else {
            std::cerr << "不明な引数: " << arg << std::endl;
            std::cerr << "使用法: ./spherical_bullettime [--gaze-u U] [--gaze-v V] [--ref-u U] [--ref-v V]" << std::endl;
            return 1;
        }
    }

    std::cout << "=====================================" << std::endl;
    std::cout << "  全天球バレットタイム処理プログラム" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;

    // パラメータ設定
    std::string video_file = "../input.mp4";
    int fps = 1;
    double focus = 1;
    double div = 8;
    double div_ = 2;
    int count_init = 0;
    int count = count_init;

    // 出力ディレクトリ作成
    std::string output_base_dir = create_output_directory("../output");
    std::string frames_dir    = output_base_dir + "/frames";
    std::string processed_dir = output_base_dir + "/processed";
    fs::create_directories(frames_dir);
    fs::create_directories(processed_dir);
    std::cout << "   frames/     展開フレーム保存先\n";
    std::cout << "   processed/  処理済み画像保存先\n\n";

    // 処理時間計測開始
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    // パラメータ表示
    std::cout << "処理パラメータ:\n";
    std::cout << "  入力動画      : " << video_file << "\n";
    std::cout << "  注視点        : (" << gaze_u << ", " << gaze_v << ")\n";
    std::cout << "  参照点        : (" << ref_u  << ", " << ref_v  << ")\n";
    std::cout << "  フォーカス距離: " << focus << "\n";
    std::cout << "  クリップ分割  : 1/" << (int)div
              << "   比較範囲: 1/" << (int)(div*div_) << "\n\n";

    cv::Size clip_size;
    cv::Size comparison_range;
    std::unique_ptr<Estimator> estimator;
    cv::Mat R;
    cv::Mat target_img;
    cv::Mat output_img;

    int total_frames = expand_mp4(video_file, frames_dir);
    std::cout << "\n";

    while(true) {
        std::chrono::system_clock::time_point rap_start = std::chrono::system_clock::now();

        // 入力画像を読み込む（frames/ から）
        cv::Mat img_in = cv::imread(frames_dir + "/" + std::to_string(count) + ".png", cv::IMREAD_GRAYSCALE);
        cv::Mat img_in_color = cv::imread(frames_dir + "/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);

        if(img_in.empty()){
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start).count();
            std::cout << "✓ 全処理完了  総時間: " << elapsed_ms << "ms\n";
            return 0;
        }

        // 初回：Estimator 初期化 + 基準フレーム生成
        if(count == count_init) {
            std::cout << "[初期化]\n";
            clip_size       = cv::Size(img_in.cols / div,        img_in.rows / div);
            comparison_range = cv::Size(img_in.cols / div / div_, img_in.rows / div / div_);
            estimator = std::make_unique<Estimator>(img_in.size(), comparison_range, clip_size, focus);

            std::cout << "  入力: " << img_in.cols << "x" << img_in.rows << "px"
                      << "  クリップ: " << clip_size.width << "x" << clip_size.height << "px"
                      << "  比較範囲: " << comparison_range.width << "x" << comparison_range.height << "px\n";

            cv::Point2i gaze(gaze_u, gaze_v);
            cv::Point2i ref(ref_u, ref_v);
            R = calc_R_by2p(gaze, ref, img_in.size());

            target_img = estimator->rotate_img2(img_in, R, img_in.size());
            cv::imwrite(processed_dir + "/00_initial_reference.png", target_img);

            output_img = estimator->rotate_clip(img_in_color, R);
            cv::imwrite(processed_dir + "/frame_0.png", output_img);
            std::cout << "✓ frame_0.png → processed/\n\n";

            count += fps;
            continue;
        }

        // フレーム処理
        std::cout << "[フレーム " << std::setw(2) << count << "/" << total_frames - 1 << "] 最適化中\n";
        R = estimator->estimate_R(img_in, target_img, R);

        output_img = estimator->rotate_clip(img_in_color, R);
        std::string out_name = "frame_" + std::to_string(count) + ".png";
        cv::imwrite(processed_dir + "/" + out_name, output_img);

        auto frame_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - rap_start).count();
        std::cout << "✓ " << out_name << "  [" << frame_ms << "ms]\n\n";

        count += fps;
    }

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start).count();
    std::cout << "✓ 全処理完了  総時間: " << elapsed_ms << "ms\n";
    return 0;
}
