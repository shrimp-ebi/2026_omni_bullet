#include <iostream>
#include <vector>
#include <cmath>

// OpenCVを使用する場合
#include <opencv2/opencv.hpp>

// 引数: 画像座標 (u,v) 、画像のサイズ (height, width)
// 戻り値: (theta, phi)
std::vector<double> sphericalImg2polar(int u, int v, std::vector<int> shape)
{
    double p_theta = (u - (shape[1] - 1) * 0.5) / (shape[1] - 1) * 2 * M_PI;
    double p_phi = -(v - (shape[0] - 1) * 0.5) / (shape[0] - 1) * M_PI;

    return { p_theta, p_phi };
}

// 引数: (theta, phi) 、画像のサイズ (height, width)
// 戻り値: 画像座標 (u,v)
std::vector<double> polar2SphericalImg(double theta, double phi, std::vector<int> shape)
{
    double u = (theta + M_PI) * (shape[1] - 1) / (2 * M_PI);
    double v = (phi - M_PI / 2) * (shape[0] - 1) / M_PI;

    return { u, v };
}

// 引数: (theta, phi)
// 戻り値: 直交座標系上の点 (x,y,z)
std::vector<double> polar2Cartesian_x(double theta, double phi)
{
    return {
        std::cos(phi) * std::cos(theta),
        std::cos(phi) * std::sin(theta),
        std::sin(phi)
    };
}

// 引数: 直交座標系上の点 (x,y,z)
// 戻り値: (theta, phi)
std::vector<double> cartesian2Polar_x(std::vector<double> vec)
{
    double theta = std::atan2(vec[1], vec[0]);
    double phi = std::acos(std::sqrt(vec[0] * vec[0] + vec[1] * vec[1]) / std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]));

    return { theta, phi };
}

int main()
{
    // 入力
    cv::Mat img = cv::imread("input.jpg");
    std::vector<double> gaze_ = { M_PI / 2, 0 }; // (theta, phi)
    std::vector<double> ref_ = { M_PI, 0 }; // (theta, phi)

    // 出力画像を定義
    cv::Mat out_img = cv::Mat::zeros(img.size(), img.type());

    // 入力点を直交座標系に変換
    std::vector<double> gaze = polar2Cartesian_x(gaze_[0], gaze_[1]);
    std::vector<double> ref = polar2Cartesian_x(ref_[0], ref_[1]);

    // 回転後の座標軸を計算
    std::vector<double> x = gaze;
    std::vector<double> z = {
        gaze[1] * ref[2] - gaze[2] * ref[1],
        gaze[2] * ref[0] - gaze[0] * ref[2],
        gaze[0] * ref[1] - gaze[1] * ref[0]
    };
    std::vector<double> y = {
        z[1] * x[2] - z[2] * x[1],
        z[2] * x[0] - z[0] * x[2],
        z[0] * x[1] - z[1] * x[0]
    };

    // 回転行列にする
    std::vector<std::vector<double>> R = { x, y, z };
    std::cout << R[0][0] << " " << R[0][1] << " " << R[0][2] << std::endl;
    std::cout << R[1][0] << " " << R[1][1] << " " << R[1][2] << std::endl;
    std::cout << R[2][0] << " " << R[2][1] << " " << R[2][2] << std::endl;

    // 全画素に対して極座標変換,直交行列変換,回転,極座標変換,画像表示を行う
    int width = out_img.cols;
    int height = out_img.rows;
    for (int v = 0; v < height; v++)
    {
        std::cout << "begin line " << v << std::endl;
        for (int u = 0; u < width; u++)
        {
            std::vector<double> polar = sphericalImg2polar(u, v, { height, width });
            std::vector<double> cartesian = polar2Cartesian_x(polar[0], polar[1]);
            std::vector<double> rotated = {
                R[0][0] * cartesian[0] + R[1][0] * cartesian[1] + R[2][0] * cartesian[2],
                R[0][1] * cartesian[0] + R[1][1] * cartesian[1] + R[2][1] * cartesian[2],
                R[0][2] * cartesian[0] + R[1][2] * cartesian[1] + R[2][2] * cartesian[2]
                };
            std::vector<double> rotated_polar = cartesian2Polar_x(rotated);
            std::vector<double> uv_input = polar2SphericalImg(rotated_polar[0], rotated_polar[1], { img.rows, img.cols });
            out_img.at<cv::Vec3b>(v, u) = img.at<cv::Vec3b>((int)uv_input[1], (int)uv_input[0]);
        }
    }

    cv::imwrite("output.png", out_img);

    return 0;
}
