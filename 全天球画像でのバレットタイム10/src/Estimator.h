//
// Created by suhara_sota on 2022/12/21.
//

#ifndef ESTIMATOR_H
#define ESTIMATOR_H

#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <fstream>

#include <opencv2/opencv.hpp>

namespace spherical_bullet_time{
    class Estimator{
    public:
        explicit Estimator(cv::Size img_size, cv::Size comp_range, cv::Size clip_range, double focus);

        //画像を回転する
        static cv::Mat rotate_img(const cv::Mat& img, const cv::Mat& R, const cv::Size& img_size);

        //画像を回転する2
        cv::Mat rotate_img2(const cv::Mat& img, const cv::Mat& R, const cv::Size& img_size);

        //入力画像を出力画像へと変換するような回転行列を推定する
        cv::Mat estimate_R(const cv::Mat& img_in, const cv::Mat& base_clip, const cv::Mat& R_ini);

        // 誤差と勾配をcsvで吐く
        void test_can_calc_error(const cv::Mat& img, const cv::Mat& base_clip, const cv::Mat& R_ini) const;

        // 画像座標系から極座標系に変換する
        [[nodiscard]] cv::Vec2d sphericalImg2polar(const cv::Vec2i& uv) const;
        static cv::Vec2d sphericalImg2polar(const cv::Vec2i& uv, const cv::Size& size);

        // 極座標系から画像座標系に変換する
        [[nodiscard]] cv::Vec2i polar2SphericalImg(const cv::Vec2d& polar) const;
        static cv::Vec2i polar2SphericalImg(const cv::Vec2d& polar, const cv::Size& size);

        // 極座標系から直交座標系に変換する
        static cv::Vec3d polar2Cartesian(const cv::Vec2d& polar);

        // 直交座標系から極座標系に変換する
        static cv::Vec2d cartesian2Polar(const cv::Vec3d& cartesian);

        // 画像座標点と回転行列を入力として回転後の画像座標を返す関数
        static void uv2uv_rotation(const cv::Vec2i& uv_in, cv::Vec2i& uv_out, const cv::Mat& R, const cv::Size& img_size);

        // 3次元座標wを通る直線を回転軸として||w||radだけ回転させる回転行列を計算する(ロドリゲスの回転公式)
        static cv::Mat calc_rodrigues_R(const cv::Vec3d &n, double theta);

        // 全天球画像を回転させて中心部分のみを切り出す
        [[nodiscard]] cv::Mat rotate_comparison_range(const cv::Mat& img_in, const cv::Mat& R) const;

        // 全天球画像の中心部分のみを切り出す
        [[nodiscard]] cv::Mat rotate_comparison_range2(const cv::Mat& img_in) const;

        // 全天球画像を回転させて指定されたサイズのバレットタイム画像を切り出す
        [[nodiscard]] cv::Mat rotate_clip(const cv::Mat& img_in, const cv::Mat& R) const;


    private:
        //微分画像を生成
        static void make_differential_image(const cv::Mat& img_in, cv::Mat& diff_image_x, cv::Mat& diff_image_y, cv::Mat& diff_image_z) ;
        // 二次元の微分カーネルを生成
        static void create_smoothing_diff_kernel(cv::Mat& kernel_x, cv::Mat& kernel_y, int kernel_size, int sigma) ;
        // 微小回転を用いて回転行列を微分した行列を返す
        static void get_differentiation_of_R(cv::Mat& w1, cv::Mat& w2, cv::Mat& w3, const cv::Mat& R) ;
        //目的関数の値と勾配を記録するグラフ
        static void appendToCSV(const std::string& filename, double value1, double value2) ;

        cv::Mat R_wc_;
        cv::Size img_size_;
        cv::Mat comp_range_cartesian_;
        cv::Mat clip_range_cartesian_;
        cv::Mat Sb;
        int num_of_clip_pixel_;
    };
}

#endif //ESTIMATOR_H
