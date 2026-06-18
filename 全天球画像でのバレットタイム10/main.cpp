#include <iostream>
#include <cmath>
#include <chrono>
#include <opencv2/opencv.hpp>

#include <Estimator.h>


using namespace spherical_bullet_time;

void expand_mp4(const std::string& filename){
    // mp4形式の全天球動画読み込み
    cv::VideoCapture cap("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename + ".mp4");
    if(!cap.isOpened()){
        std::cout << "動画ファイルが開けません" << std::endl;
    }

    std::cout << "width : " << (int)cap.get(cv::CAP_PROP_FRAME_WIDTH) << std::endl;
    std::cout << "height : " << (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    std::cout << "fps : " << cap.get(cv::CAP_PROP_FPS) << std::endl;

    cv::Mat frame;
    int count = 0;
    while(true){
        cap.read(frame);
        if(frame.empty()){
            std::cout << "動画末尾到達" << std::endl;
            return;
        }
        cv::imwrite("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename +"/" + std::to_string(count) + ".png", frame);
        count++;
    }
}


cv::Mat calc_R_by2p(const cv::Vec2d& gaze, const cv::Vec2d& ref){
    // 注視点と参照点の3次元座標を計算
    auto gaze_3d = Estimator::polar2Cartesian(gaze);
    auto ref_3d = Estimator::polar2Cartesian(ref);
//    std::cout << "gaze_3d" << gaze_3d << std::endl;
//    std::cout << "ref_3d" << ref_3d << std::endl;

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
//    std::cout << "gaze_polar" << gaze_polar << std::endl;
//    std::cout << "ref_polar" << ref_polar << std::endl;

//    gaze_polar = cv::Vec2d(0, M_PI_2);
//    ref_polar = cv::Vec2d(M_PI_4, M_PI_2);

    return calc_R_by2p(gaze_polar, ref_polar);
}

void reverse(const std::string& filename)
{
    int count = 0;
    while(true){
        // 入力画像を読み込む
        cv::Mat img_in = cv::imread("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename + "/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);
        if(count == 300){
            return;
        }

        // 後ろを向くような回転行列
        cv::Mat reverse_R = cv::Mat::zeros(3,3, CV_64F);
        reverse_R.ptr<double>(0)[0] = -1;
        reverse_R.ptr<double>(1)[1] = -1;
        reverse_R.ptr<double>(2)[2] = 1;

        cv::Mat reversed = Estimator::rotate_img(img_in, reverse_R, img_in.size());
        cv::imwrite("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename + std::to_string(count) + ".png", reversed);
        count+=30;
    }
}

void reverse2(const std::string& filename)
{
    int count = -10;
    while(true){
        // 入力画像を読み込む
        cv::Mat img_in = cv::imread("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename + "/0.png", cv::IMREAD_COLOR);
        if(count == 10){
            return;
        }

        // 真横を向く回転行列
       double angle_degrees = count; // 数度だけ右を向く
       double theta = angle_degrees * M_PI / 180.0; // ラジアンに変換

       // 回転行列を生成
       cv::Mat z_R = cv::Mat::eye(3, 3, CV_64F); // 単位行列で初期化

       //Z軸
/*       z_R.at<double>(0, 0) = cos(theta);
       z_R.at<double>(0, 1) = sin(theta);
       z_R.at<double>(1, 0) = -sin(theta);
       z_R.at<double>(1, 1) = cos(theta);*/
       
       //y軸
/*       z_R.at<double>(0, 0) = cos(theta);
       z_R.at<double>(0, 2) = sin(theta);
       z_R.at<double>(2, 0) = -sin(theta);
       z_R.at<double>(2, 2) = cos(theta);*/
       
       //x軸
       z_R.at<double>(1, 1) = cos(theta);
       z_R.at<double>(1, 2) = sin(theta);
       z_R.at<double>(2, 1) = -sin(theta);
       z_R.at<double>(2, 2) = cos(theta);

        cv::Mat reversed = Estimator::rotate_img(img_in, z_R, img_in.size());
        cv::imwrite("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/" + filename + "/roll/" + std::to_string(count) + ".png", reversed);
        std::cout << count << "枚目" << std::endl;   
        count+=1;
     
    }
}

void draw_gaze_line(){
    int count = 0;
    while(true){
        cv::Mat img_in_color = cv::imread("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/猫/test/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);
        if(img_in_color.empty()){
            std::cout << count << std::endl;
            std::cout << "おわり" << std::endl;
            return;
        }
        int rows = 380;
        int cols = 760;
//        int rows = 3040;
//        int cols = 6080;
        

        cv::line(img_in_color,cv::Point(0,rows/2), cv::Point(cols-1, rows/2), cv::Scalar(0,255,0),2);
        cv::line(img_in_color,cv::Point(cols/2,0), cv::Point(cols/2, rows-1), cv::Scalar(0,255,0),2);
        cv::imwrite("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/猫/test/line_"+std::to_string(count)+".png", img_in_color);
        count += 1;
    }
}

void create_GIF()
{
    cv::VideoWriter writer("時計_GIF.mp4", cv::VideoWriter::fourcc('M','P','4','V'), 30, cv::Size(720,360),1);
    int count = 0;

    while(true) {
        cv::Mat img_in_color = cv::imread("./時計/with_line/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);
        if (img_in_color.empty()) {
            std::cout << count << std::endl;
            std::cout << "おわり" << std::endl;
            return;
        }
        writer << img_in_color;
        count += 1;
    }
}


int main()
{
    // mp4ファイルを画像にばらす
//    expand_mp4("自動販売機3");
//    return 0;
   // 画像に注視点を示す線を引く
//    draw_gaze_line();
//    return 0;
//    create_GIF();
//    return 0;

    std::string filename = "猫";
    std::string homename = "/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/";
    int fps = 1;
    double focus = 1;
    double div = 8;
    double div_ = 2;
    int count_init = 0;
    int count = count_init;

    // 画像をひっくり返す
//    reverse(filename);
//    return 0;

    // 画像を回転させる
//    reverse2(filename);
//    return 0;

    // 処理時間計測開始
    std::chrono::system_clock::time_point  start = std::chrono::system_clock::now();

    cv::Size clip_size;
    cv::Size comparison_range;
    std::unique_ptr<Estimator> estimator;
    cv::Mat R;
    cv::Mat target_img;
    cv::Mat output_img;

    while(count<=10) {
        std::chrono::system_clock::time_point  rap_start = std::chrono::system_clock::now();

        // 入力画像を読み込む
        cv::Mat img_in = cv::imread(homename + filename + "/left/" + std::to_string(count) + ".png", cv::IMREAD_GRAYSCALE);
        cv::Mat img_in_color = cv::imread(homename + filename + "/left/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);
//        cv::Mat img_in = cv::imread("./rev_" + filename + "/" + std::to_string(count) + ".png", cv::IMREAD_GRAYSCALE);
//        cv::Mat img_in_color = cv::imread("./rev_" + filename + "/" + std::to_string(count) + ".png", cv::IMREAD_COLOR);
        if(img_in.empty()){
            // 処理時間計測終了
            std::chrono::system_clock::time_point  stop = std::chrono::system_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << "ms" << std::endl;
            return 0;
        }
//        img_in = cv::imread("bigimg.png", cv::IMREAD_GRAYSCALE);

        // 初回の初期化処理
        if(count == count_init) {
            // 推定器のインスタンス生成
            clip_size = cv::Size(img_in.cols / div, img_in.rows / div);  // 生成する透視投影画像のサイズ
            comparison_range = cv::Size(img_in.cols / div/div_, img_in.rows / div/div_);  // マッチング範囲
            estimator = std::make_unique<Estimator>(img_in.size(), comparison_range, clip_size, focus);

            // 1枚目の画像の中から注視点を選択し透視投影画像を生成
//            cv::Point2i gaze(2584, 1291);  // 研究室本棚
//            cv::Point2i ref(2878, 1282);  // 研究室本棚
//            cv::Point2i gaze(3704, 1609);  // 研究室内電子レンジ用
//            cv::Point2i ref(3792, 1610);  // 研究室内電子レンジ用
//            cv::Point2i gaze(140, 1356);  // object
//            cv::Point2i ref(152, 1355);  // object
//            cv::Point2i gaze(5537, 1474);  // object_100
//            cv::Point2i ref(5555, 1478);  // object_100
//            cv::Point2i gaze(5716, 1325);  // 時計
//            cv::Point2i ref(5726, 1324);  // 時計
//            cv::Point2i gaze(389, 1335);  // 時計_90
//            cv::Point2i ref(403, 1335);  // 時計_90
//            cv::Point2i gaze(25, 1580);  // 自販機
//            cv::Point2i ref(36, 1584);  // 自販機
//            cv::Point2i gaze(2924, 1486);  // 自販機_rev
//            cv::Point2i ref(2945, 1486);  // 自販機_rev
//            cv::Point2i gaze(473, 1490);  // 自販機_100フレーム目
//            cv::Point2i ref(507, 1490);  // 自販機_100フレーム目
//            cv::Point2i gaze(796, 1491);  // 自販機_130フレーム目
//            cv::Point2i ref(820, 1491);  // 自販機_130フレーム目
//            cv::Point2i gaze(5595, 1090);  // ポスト
//            cv::Point2i ref(5625, 1055);  // ポスト
//            cv::Point2i gaze(5734, 1519);  // 看板
//            cv::Point2i ref(5755, 1522);  // 看板
//            cv::Point2i gaze(2866, 1420);  // 自販機2_40
//            cv::Point2i ref(2992, 1420);  // 自販機2_40
//            cv::Point2i gaze(2774, 1364);  // 自販機3_100
//            cv::Point2i ref(2848, 1359);  // 自販機3_100
//            cv::Point2i gaze(3108, 1508);  // 掲示板
//            cv::Point2i ref(3208, 1508);  // 掲示板
//            cv::Point2i gaze(2817 ,1930);  // 猫(cat1左耳)
//           cv::Point2i ref(2999 ,1881);  // 猫(cat1右耳)
            cv::Point2i gaze(3040 ,1520);  // 猫(中心cat1左耳)
            cv::Point2i ref(3213 ,1520);  // 猫(中心cat1右耳)            
            R = calc_R_by2p(gaze, ref, img_in.size());  // 注視点と参照点から回転行列を計算
//            std::cout << "R_init" << R << std::endl;
            // 計算した回転行列で最初のフレームだけ計算
            target_img = estimator->rotate_img2(img_in, R, img_in.size());
            cv::imwrite(homename + filename + "/initial_clip.png", target_img);
            output_img = estimator->rotate_clip(img_in_color, R);
            cv::imwrite(homename + filename + "/test/" + std::to_string(count) + ".png", output_img);
            count+=fps;
            continue;
        }

//        estimator->test_can_calc_error(img_in, target_img, R);  // デバッグ用

        // 手動で初期値をずらす用
        // ヨー方向回転
//        double theta = 2/180.0*M_PI;
//        cv::Mat error_R = cv::Mat::zeros(3,3, CV_64F);
//        std::cout << "yaw" << std::endl;
//        error_R.ptr<double>(0)[0] = std::cos(theta);
//        error_R.ptr<double>(0)[1] = -std::sin(theta);
//        error_R.ptr<double>(1)[0] = std::sin(theta);
//        error_R.ptr<double>(1)[1] = std::cos(theta);
//        error_R.ptr<double>(2)[2] = 1;
//        R = error_R * R;

        // 回転行列を推定
        R = estimator->estimate_R(img_in, target_img, R);

        // 結果を出力する
//        target_img = estimator->rotate_img2(img_in, R, img_in.size());
        output_img = estimator->rotate_clip(img_in_color, R);
        cv::imwrite(homename + filename + "/test/" + std::to_string(count) + ".png", output_img);
        count += fps;

        // 画像一枚にかかる時間を計測
        std::chrono::system_clock::time_point  rap_end = std::chrono::system_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(rap_end - rap_start).count() << "ms" << std::endl;
    }
}
