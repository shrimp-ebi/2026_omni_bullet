//
// Created by suhara_sota on 2022/12/21.
//

#include "Estimator.h"

#include <cstdlib>
#include <filesystem>
#include <utility>

namespace {
namespace fs = std::filesystem;

// LM法の誤差推移を出力するCSVパス。
// main.cpp から環境変数で渡されない場合は、カレントディレクトリの data.csv に出す。
std::string error_csv_path()
{
    const char* path = std::getenv("SPHERICAL_BT_CSV_PATH");
    if(path != nullptr && path[0] != '\0') {
        return path;
    }
    return "data.csv";
}

// デバッグ画像・デバッグCSVの保存先を作る。
// 実行ごとに runs/.../debug を使うため、過去の結果と混ざりにくい。
std::string debug_output_path(const std::string& filename)
{
    const char* dir = std::getenv("SPHERICAL_BT_DEBUG_DIR");
    fs::path debug_dir = (dir != nullptr && dir[0] != '\0') ? fs::path(dir) : fs::path("debug");
    fs::create_directories(debug_dir);
    return (debug_dir / filename).string();
}
}

namespace spherical_bullet_time{
    Estimator::Estimator(cv::Size img_size, cv::Size comp_range, cv::Size clip_range, double focus):
        img_size_(std::move(img_size)){
        // ========================================
        // 初期化処理
        // ========================================
        // 光軸付近のマッチング範囲と出力画像範囲の3次元座標をあらかじめ設定
        comp_range_cartesian_ = cv::Mat_<cv::Vec3d>(std::move(comp_range));
        clip_range_cartesian_ = cv::Mat_<cv::Vec3d>(std::move(clip_range));
        Sb = cv::Mat_<cv::Vec3d>(img_size_);  // 全天球画像の3次元座標マップ

        // 透視投影カメラの向きを正面に向けるための回転行列。
        // このプログラムでは「全天球画像上の方向」を単位球面上の点として扱う。
        // R_wc_ は仮想カメラ座標と全天球側の座標軸の対応を合わせるための固定行列。
        R_wc_ = cv::Mat::zeros(3, 3, CV_64F);
        R_wc_.ptr<double>(0)[1] = 1;
        R_wc_.ptr<double>(1)[2] = 1;
        R_wc_.ptr<double>(2)[0] = 1;

        // 光軸位置（中心）
        int u0_comp = int(comp_range_cartesian_.cols/2);
        int v0_comp = int(comp_range_cartesian_.rows/2);
        int u0_clip = int(clip_range_cartesian_.cols/2);
        int v0_clip = int(clip_range_cartesian_.rows/2);

        // ========================================
        // カメラパラメータの計算
        // ========================================
        // 全天球画像のサイズ
        int We = img_size_.width;
        int He = img_size_.height;
        // マッチング範囲と出力範囲のサイズ
        int Wp_comp = int(comp_range_cartesian_.cols);
        int Hp_comp = int(comp_range_cartesian_.rows);
        int Wp_clip = int(clip_range_cartesian_.cols);
        int Hp_clip = int(clip_range_cartesian_.rows);

        // 画角の計算（ラジアン）。
        // 全天球画像の横幅が360度、縦幅が180度に相当するため、
        // 切り出しサイズから仮想カメラの水平・垂直画角を決める。
        double Theta_comp = 2*asin((M_PI*Wp_comp)/We);
        double Phi_comp = 2*asin((M_PI*Hp_comp)/(2*He));
        double Theta_clip = 2*asin((M_PI*Wp_clip)/We);
        double Phi_clip = 2*asin((M_PI*Hp_clip)/(2*He));

        // 画素間の長さの計算（仮想透視画像平面上での間隔）。
        // 出力クリップの1画素が、単位球面方向としてどれだけずれるかを決める。
        double Delta_x_comp = 2*tan(Theta_comp/2)/Wp_comp;
        double Delta_y_comp = 2*tan(Phi_comp/2)/Hp_comp;
        double Delta_x_clip = 2*tan(Theta_clip/2)/Wp_clip;
        double Delta_y_clip = 2*tan(Phi_clip/2)/Hp_clip;

        // ========================================
        // 全天球画像の3次元座標マップ（Sb）を生成
        // ========================================
        // 各ピクセルに対応する3次元座標を計算・格納
        cv::Vec2i uv;
        for(uv[1]=0; uv[1]<img_size_.height; uv[1]++){
            for(uv[0]=0; uv[0]<img_size_.width; uv[0]++){
                auto polar = sphericalImg2polar(uv, img_size_);
                auto cartesian = polar2Cartesian(polar);
                Sb.ptr<cv::Vec3d>(uv[1])[uv[0]] = cartesian;
            }
        }

        // ========================================
        // マッチング範囲のカメラ座標系3次元座標を生成
        // ========================================
        // カメラ座標系でのピクセル位置を、世界座標系での3次元方向ベクトルとして保存する。
        // ここで作った方向ベクトルをRで回すと、全天球画像上の参照先ピクセルが分かる。
        for (int v = 0; v < comp_range_cartesian_.rows; ++v) {
            for (int u = 0; u < comp_range_cartesian_.cols; ++u) {
                // カメラ座標系での3次元座標を計算
                cv::Vec3d cartesian_camera_comp = cv::Vec3d((u-u0_comp)*Delta_x_comp, (v-v0_comp)*Delta_y_comp, focus);
                cartesian_camera_comp = cartesian_camera_comp/cv::norm(cartesian_camera_comp);  // 正規化
                // ワールド座標系に変換
                comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(R_wc_.t()*cartesian_camera_comp);
            }
        }

        // ========================================
        // 出力クリップ範囲のカメラ座標系3次元座標を生成
        // ========================================
        for (int v = 0; v < clip_range_cartesian_.rows; ++v) {
            for (int u = 0; u < clip_range_cartesian_.cols; ++u) {
                // カメラ座標系での3次元座標を計算
                cv::Vec3d cartesian_camera_clip = cv::Vec3d((u-u0_clip)*Delta_x_clip, (v-v0_clip)*Delta_y_clip, focus);
                cartesian_camera_clip = cartesian_camera_clip/cv::norm(cartesian_camera_clip);  // 正規化
                // ワールド座標系に変換
                clip_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(R_wc_.t()*cartesian_camera_clip);
            }
        }

        // LM法で平均誤差・平均勾配を出すための画素数。
        // 変数名はclipだが、実際には推定用のcomparison rangeの画素数。
        num_of_clip_pixel_ = comp_range_cartesian_.rows*comp_range_cartesian_.cols;
    }

    //画像を回転する
    cv::Mat Estimator::rotate_img(const cv::Mat &img, const cv::Mat &R, const cv::Size& img_size){
        // 出力画像
        cv::Mat img_out;
        // 回転前後の座標を保存する変数
        cv::Vec2i uv_in;
        cv::Vec2i uv_out;

        if(img.type() == CV_8UC3){
            img_out = cv::Mat(img_size, CV_8UC3);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    uv2uv_rotation(uv_out, uv_in, R.t(), img_size);
                    img_out.ptr<cv::Vec3b>(uv_out[1])[uv_out[0]] = img.ptr<cv::Vec3b>(uv_in[1])[uv_in[0]];
//                std::cout << uv_out[1] << "," << uv_out[0] << " : " << uv_in[1] << "," << uv_in[0] << std::endl;
                }
            }
        }else{
            img_out = cv::Mat(img_size, CV_8U);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    uv2uv_rotation(uv_out, uv_in, R.t(), img_size);
                    img_out.ptr<uint8_t>(uv_out[1])[uv_out[0]] = img.ptr<cv::uint8_t>(uv_in[1])[uv_in[0]];
//                std::cout << uv_out[1] << "," << uv_out[0] << " : " << uv_in[1] << "," << uv_in[0] << std::endl;
                }
            }
        }

        return img_out;
    }
    
        //画像を回転する2（数値積分的な方法）
    cv::Mat Estimator::rotate_img2(const cv::Mat &img, const cv::Mat &R, const cv::Size& img_size){
        // ========================================
        // 全天球画像を回転させる
        // ========================================
        // 出力画像
        cv::Mat img_out;
        // 回転前後の座標を保存する変数
        cv::Vec2i uv_in;
        cv::Vec2i uv_out;

        if(img.type() == CV_8UC3){
            // カラー画像の場合
            img_out = cv::Mat(img_size, CV_8UC3);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    // 出力画像の座標系の3次元座標を計算
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(uv_out[1])[uv_out[0]];
                    // 3次元座標を極座標に変換
                    auto rotated_polar = cartesian2Polar(rotated);
                    // 極座標を入力画像の座標に変換
                    uv_in = polar2SphericalImg(rotated_polar, img_size);
                    // 入力画像からピクセル値をコピー
                    img_out.ptr<cv::Vec3b>(uv_out[1])[uv_out[0]] = img.ptr<cv::Vec3b>(uv_in[1])[uv_in[0]];
                }
            }
        }else{
            // グレースケール画像の場合
            img_out = cv::Mat(img_size, CV_8U);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    // 出力画像の座標系の3次元座標を計算
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(uv_out[1])[uv_out[0]];
                    // 3次元座標を極座標に変換
                    auto rotated_polar = cartesian2Polar(rotated);
                    // 極座標を入力画像の座標に変換
                    uv_in = polar2SphericalImg(rotated_polar, img_size);
                    // 入力画像からピクセル値をコピー
                    img_out.ptr<uint8_t>(uv_out[1])[uv_out[0]] = img.ptr<cv::uint8_t>(uv_in[1])[uv_in[0]];
                }
            }
        }

        return img_out;
    }


    // 入力画像を基準画像へ近づける回転行列を推定する。
    // 基本方針：
    // 1. 現在のRで入力画像を仮想的に回転したときの画素値を読む。
    // 2. 基準画像との差を誤差にする。
    // 3. 誤差が小さくなる微小回転 delta_w をLM法で求める。
    // 4. delta_wをロドリゲスの式で回転行列にし、Rへ左から掛ける。
    cv::Mat Estimator::estimate_R(const cv::Mat &img_in, const cv::Mat &base_clip, const cv::Mat& R_ini)
    {
        // ========================================
        // Levenberg-Marquardt法による回転行列推定
        // ========================================
        // 初期値設定
        cv::Mat R = R_ini.clone();

        // LM法の正則化パラメータ
        double lambda = 0.0001;

        // 微分画像を生成（画像勾配）。
        // 回転を少し変えたとき、画素値がどちらへ増減するかを調べるために使う。
        cv::Mat diff_image_x;
        cv::Mat diff_image_y;
        cv::Mat diff_image_z;
        make_differential_image(img_in, diff_image_x, diff_image_y, diff_image_z);

        const int max_outer_iter = 300;
        const int max_inner_iter = 50;
        int count=0;
        int outer_iter = 0;
        while(outer_iter++ < max_outer_iter) {
            // ========================================
            // 回転行列の微分行列を生成
            // ========================================
            cv::Mat w1, w2, w3;
            get_differentiation_of_R(w1, w2, w3, R);

            // ========================================
            // 勾配とヘッセ行列の計算
            // ========================================
            cv::Vec3d grad(0, 0, 0);
            cv::Mat hesse = cv::Mat::zeros(3, 3, CV_64F);
            double sum_of_error = 0;

            // マッチング範囲の中心座標
            cv::Vec2i xy_in;
            cv::Vec2i xy_out;
            int comp_start_x = (img_in.cols-comp_range_cartesian_.cols)/2;
            int comp_start_y = (img_in.rows-comp_range_cartesian_.rows)/2;

            // 全画素に対して誤差・勾配・ヘッセ行列を計算する。
            // ここで見ているのは画像全体ではなく、中央の小さな比較範囲だけ。
            for(xy_in[1]=comp_start_y; xy_in[1]<comp_start_y+comp_range_cartesian_.rows; xy_in[1]++){
                for(xy_in[0]=comp_start_x; xy_in[0]<comp_start_x+comp_range_cartesian_.cols; xy_in[0]++){
                    // 現在の回転行列で3次元座標を回転
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    auto rotated_polar = cartesian2Polar(rotated);
                    xy_out = polar2SphericalImg(rotated_polar, img_in.size());

                    // 回転前後の画像差分（誤差）を計算
                    double error = img_in.ptr<uint8_t>(xy_out[1])[xy_out[0]] - base_clip.ptr<uint8_t>(xy_in[1])[xy_in[0]];
                    sum_of_error += error*error;

                    // 回転パラメータに対する3次元座標の微分を計算する。
                    // w1,w2,w3 はそれぞれx,y,z軸まわりの微小回転に対するRの変化。
                    cv::Mat differential_1 = w1 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    cv::Mat differential_2 = w2 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    cv::Mat differential_3 = w3 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];

                    // 画像勾配を取得
                    double sx = diff_image_x.ptr<double>(xy_out[1])[xy_out[0]];
                    double sy = diff_image_y.ptr<double>(xy_out[1])[xy_out[0]];
                    double sz = diff_image_z.ptr<double>(xy_out[1])[xy_out[0]];

                    // 合成関数の微分則で、回転を少し変えたときの誤差変化を計算する。
                    // 画像勾配(sx,sy,sz)と、回転による3D方向の変化(differential_*)を内積する。
                    double nabla_e1 = sx * differential_1.ptr<double>(0)[0] + sy * differential_1.ptr<double>(0)[1] + sz * differential_1.ptr<double>(0)[2];
                    double nabla_e2 = sx * differential_2.ptr<double>(0)[0] + sy * differential_2.ptr<double>(0)[1] + sz * differential_2.ptr<double>(0)[2];
                    double nabla_e3 = sx * differential_3.ptr<double>(0)[0] + sy * differential_3.ptr<double>(0)[1] + sz * differential_3.ptr<double>(0)[2];

                    // 勾配を蓄積
                    grad[0] += error * nabla_e1;
                    grad[1] += error * nabla_e2;
                    grad[2] += error * nabla_e3;

                    // ヘッセ行列を蓄積
                    hesse.ptr<double>(0)[0] += nabla_e1 * nabla_e1;
                    hesse.ptr<double>(0)[1] += nabla_e1 * nabla_e2;
                    hesse.ptr<double>(0)[2] += nabla_e1 * nabla_e3;
                    hesse.ptr<double>(1)[0] += nabla_e2 * nabla_e1;
                    hesse.ptr<double>(1)[1] += nabla_e2 * nabla_e2;
                    hesse.ptr<double>(1)[2] += nabla_e2 * nabla_e3;
                    hesse.ptr<double>(2)[0] += nabla_e3 * nabla_e1;
                    hesse.ptr<double>(2)[1] += nabla_e3 * nabla_e2;
                    hesse.ptr<double>(2)[2] += nabla_e3 * nabla_e3;
                }
            }

            // 平均値を計算
            grad /= num_of_clip_pixel_;
            hesse /= num_of_clip_pixel_;
            sum_of_error /= num_of_clip_pixel_;

            // ========================================
            // Levenberg-Marquardt法のパラメータ更新
            // ========================================
            cv::Mat delta_w;
            cv::Mat delta_R;
            cv::Mat R_new;
            double sum_of_error_new;

            // lambda値を調整しながら適切な更新を探す
            int inner_iter = 0;
            while(inner_iter++ < max_inner_iter) {
                cv::Mat hesse_cp = hesse.clone();

                // LM法：ヘッセ行列の対角成分にlambda*Iを加算（正則化）。
                // lambdaが大きいほど慎重に、小さいほどニュートン法に近い大きな更新になる。
                hesse_cp.ptr<double>(0)[0] += 1 + lambda;
                hesse_cp.ptr<double>(1)[1] += 1 + lambda;
                hesse_cp.ptr<double>(2)[2] += 1 + lambda;

                // 逆行列計算
                cv::Mat hesse_inv = hesse_cp.inv();

                // 更新値計算：delta_w = -H^-1 * g
                delta_w = -hesse_inv * grad;

                // パラメータの変化量を回転行列化（ロドリゲスの回転公式）
                double norm_w = cv::norm(delta_w);

                // 変化が無い場合は推定を終了
                if(norm_w < 1.0e-16){
                    return R;
                }

                cv::Mat normalized_w = delta_w/norm_w;
                delta_R = calc_rodrigues_R(normalized_w, norm_w);

                // 更新したパラメータを用いた新しい回転行列を計算。
                // delta_Rを左から掛けて、現在の姿勢Rを微小回転ぶんだけ動かす。
                R_new = delta_R * R;

                // ========================================
                // 更新後の回転行列で誤差を再計算
                // ========================================
                sum_of_error_new = 0;
                cv::Vec2i xy_in_2;
                for(xy_in_2[1]=comp_start_y; xy_in_2[1]<comp_start_y+comp_range_cartesian_.rows; xy_in_2[1]++){
                    for(xy_in_2[0]=comp_start_x; xy_in_2[0]<comp_start_x+comp_range_cartesian_.cols; xy_in_2[0]++){
                        // 新しい回転行列で3次元座標を回転
                        cv::Mat rotated = R_new * Sb.ptr<cv::Vec3d>(xy_in_2[1])[xy_in_2[0]];
                        auto rotated_polar = cartesian2Polar(rotated);
                        cv::Vec2i uv_out = polar2SphericalImg(rotated_polar);

                        // 誤差計算
                        double error = img_in.ptr<uint8_t>(uv_out[1])[uv_out[0]] - base_clip.ptr<uint8_t>(xy_in_2[1])[xy_in_2[0]];
                        sum_of_error_new += error*error;
                    }
                }
                sum_of_error_new /= num_of_clip_pixel_;

                // ========================================
                // 誤差値の比較と収束判定
                // ========================================
                double diff = sum_of_error - sum_of_error_new;

                // 誤差が改善した場合、または改善量が十分小さい場合は、この更新を採用する。
                // 誤差が悪化する場合はlambdaを大きくし、より小さな一歩で再試行する。
                appendToCSV(error_csv_path(), count, sum_of_error_new);
                count++;

                if (sum_of_error > sum_of_error_new || abs(diff) < 0.1) break;

                // 誤差が改善していない場合はlambdaを増加（より小さい更新を試す）
                lambda *= 10;
            }

            appendToCSV(error_csv_path(), count, sum_of_error_new);
            count++;

            // パラメータ更新
            R = R_new;

            // ========================================
            // 終了条件判定
            // ========================================
            // パラメータの更新値が十分小さいことを確認
            if(cv::norm(delta_w) < 1.0e-5){
                return R;
            }

            // 次のイテレーションではlambdaを減少（より大きい更新を試す）
            lambda /= 10;
        }
        // 最大反復回数に達した場合は現在のRを返す
        return R;
    }


    // 微分画像を計算する（画像勾配をカルテシアン座標系で計算）。
    // 通常の画像微分はu,v方向だが、回転推定では3D方向ベクトルが動く。
    // そのため θ,φ 方向の微分を、単位球面上の x,y,z 方向の微分へ変換している。
    void Estimator::make_differential_image(const cv::Mat& img_in, cv::Mat& diff_image_x, cv::Mat& diff_image_y, cv::Mat& diff_image_z){
        // ========================================
        // 入力画像をぼかして微分計算
        // ========================================
        int ksize = 3;  // ガウシアンカーネルサイズ
        double sigma = 2;  // ガウシアンカーネルの標準偏差
        cv::Mat blurred;
        cv::GaussianBlur(img_in, blurred, cv::Size(ksize, ksize), sigma);
        blurred.convertTo(blurred, CV_64F);

        // 微分画像の初期化
        diff_image_x = cv::Mat(img_in.rows, img_in.cols, CV_64F);
        diff_image_y = cv::Mat(img_in.rows, img_in.cols, CV_64F);
        diff_image_z = cv::Mat(img_in.rows, img_in.cols, CV_64F);

        cv::Vec2i uv_in;

        // ========================================
        // 各ピクセルの勾配を計算
        // ========================================
        for(uv_in[1]=0; uv_in[1]<img_in.rows; uv_in[1]++){
            for(uv_in[0]=0; uv_in[0]<img_in.cols; uv_in[0]++){
                // 現在のピクセルを極座標に変換
                auto polar = sphericalImg2polar(uv_in, img_in.size());
                double s_theta = std::sin(polar[0]);
                double s_phi = std::sin(polar[1]);
                double c_theta = std::cos(polar[0]);
                double c_phi = std::cos(polar[1]);

                // θ方向の極座標差分を計算
                double par_d_theta = sphericalImg2polar(cv::Vec2i((uv_in[0] + 1) % img_in.cols, uv_in[1]), img_in.size())[0]
                                   - sphericalImg2polar(cv::Vec2i((uv_in[0] - 1 + img_in.cols) % img_in.cols, uv_in[1]), img_in.size())[0];
                // φ方向の極座標差分を計算
                double par_d_phi = sphericalImg2polar(cv::Vec2i(uv_in[0], (uv_in[1] + 1) % img_in.rows), img_in.size())[1]
                                 - sphericalImg2polar(cv::Vec2i(uv_in[0], (uv_in[1] - 1 + img_in.rows) % img_in.rows), img_in.size())[1];

                // θ方向の画像勾配を計算
                double par_d_S_theta = blurred.ptr<double>(uv_in[1])[(uv_in[0] + 1) % img_in.cols]
                                     - blurred.ptr<double>(uv_in[1])[(uv_in[0] - 1 + img_in.cols) % img_in.cols];
                // φ方向の画像勾配を計算
                double par_d_S_phi = blurred.ptr<double>((uv_in[1] + 1) % img_in.rows)[uv_in[0]]
                                   - blurred.ptr<double>((uv_in[1] - 1 + img_in.rows) % img_in.rows)[uv_in[0]];

                // 合成関数の微分則を適用：∂S/∂θ, ∂S/∂φ を計算
                double diffS_theta = par_d_S_theta/par_d_theta;
                double diffS_phi = par_d_S_phi/par_d_phi;

                // カルテシアン座標系での微分を計算
                // x = sin(φ)cos(θ), y = sin(φ)sin(θ), z = cos(φ) からのヤコビアン
                if(std::abs(s_phi)<=1.0e-10){
                    // 極点付近では勾配を0とする
                    diff_image_x.ptr<double>(uv_in[1])[uv_in[0]] = 0;
                    diff_image_y.ptr<double>(uv_in[1])[uv_in[0]] = 0;
                }else{
                    diff_image_x.ptr<double>(uv_in[1])[uv_in[0]] = (-diffS_theta*s_theta/s_phi)+(diffS_phi*c_phi*c_theta);
                    diff_image_y.ptr<double>(uv_in[1])[uv_in[0]] = (diffS_theta*c_theta/s_phi)+(diffS_phi*c_phi*s_theta);
                }

                diff_image_z.ptr<double>(uv_in[1])[uv_in[0]] = (diffS_theta*0)+(-diffS_phi*s_phi);
            }
        }

        // デバッグ用：微分画像の可視化（オプション）
        cv::Mat output_x, output_y, output_z;
        diff_image_x.convertTo(output_x, CV_8U, 255.0);
        diff_image_y.convertTo(output_y, CV_8U, 255.0);
        diff_image_z.convertTo(output_z, CV_8U, 255.0);
    }

    // ========================================
    // 座標系変換関数群
    // ========================================

    // 画像座標系（ピクセル座標）から球面座標系の極座標（θ, φ）に変換。
    // u方向は -π ～ π の方位角、v方向は 0 ～ π の天頂角として扱う。
    cv::Vec2d Estimator::sphericalImg2polar(const cv::Vec2i& uv) const{
        int u = uv[0];
        int v = uv[1];

        // θ：水平方向の角度（-π ～ π）
        double theta = (u - img_size_.width * 0.5) / img_size_.width * 2 * M_PI;
        // φ：垂直方向の角度（0 ～ π）
        double phi = -double(v - img_size_.height) / img_size_.height * M_PI;

        return cv::Vec2d(theta, phi);
    }

    // 静的版：画像座標系から球面座標系の極座標に変換
    cv::Vec2d Estimator::sphericalImg2polar(const cv::Vec2i& uv, const cv::Size& size){
        int u = uv[0];
        int v = uv[1];

        double theta = (u - size.width * 0.5) / size.width * 2 * M_PI;
        double phi = -double(v - size.height) / size.height * M_PI;

        return cv::Vec2d(theta, phi);
    }


    // 極座標（θ, φ）から画像座標系（ピクセル座標）に変換。
    // sphericalImg2polar の逆変換として、全天球画像上の参照画素を求める。
    cv::Vec2i Estimator::polar2SphericalImg(const cv::Vec2d& polar) const{
        double theta = polar[0];
        double phi = polar[1];

        // ピクセル座標に変換
        int u = int((theta + M_PI)*img_size_.width/(2*M_PI));
        int v = int(-(phi - M_PI)*img_size_.height/M_PI);

        return cv::Vec2i(u, v);
    }

    // 静的版：極座標から画像座標系に変換
    cv::Vec2i Estimator::polar2SphericalImg(const cv::Vec2d& polar, const cv::Size& size){
        double theta = polar[0];
        double phi = polar[1];

        int u = int((theta + M_PI)*size.width/(2*M_PI));
        int v = int(-(phi - M_PI)*size.height/M_PI);

        return cv::Vec2i(u, v);
    }


    // 極座標（θ, φ）から直交座標系（x, y, z）に変換。
    // 画像上の1点を「カメラ中心から見た方向」として単位球面上に置く。
    cv::Vec3d Estimator::polar2Cartesian(const cv::Vec2d& polar)
    {
        double theta = polar[0];
        double phi = polar[1];

        // 単位球面上の座標
        double x = std::sin(phi) * std::cos(theta);
        double y = std::sin(phi) * std::sin(theta);
        double z = std::cos(phi);

        return cv::Vec3d(x, y, z);
    }

    // 直交座標系（x, y, z）から極座標（θ, φ）に変換。
    // 回転後の3D方向を、全天球画像のピクセル参照へ戻す前段階。
    cv::Vec2d Estimator::cartesian2Polar(const cv::Vec3d& cartesian){
        double x = cartesian[0];
        double y = cartesian[1];
        double z = cartesian[2];

        // θ：x-y平面での角度
        double theta = std::atan2(y, x);
        // φ：z軸からの角度
        double phi = std::acos(z/cv::norm(cartesian));

        return cv::Vec2d(theta, phi);
    }

    // 画像座標点と回転行列を入力として回転後の画像座標を返す関数。
    // ピクセル -> 極座標 -> 3D方向 -> 回転 -> 極座標 -> ピクセル、という変換を1点に対して行う。
    void Estimator::uv2uv_rotation(const cv::Vec2i& uv_in, cv::Vec2i& uv_out, const cv::Mat& R, const cv::Size& img_size){
        auto polar = sphericalImg2polar(uv_in, img_size);
        auto cartesian = polar2Cartesian(polar);
        cv::Mat rotated = R * (cartesian);
        auto rotated_polar = cartesian2Polar(rotated);
        uv_out = polar2SphericalImg(rotated_polar, img_size);
    }

    void Estimator::create_smoothing_diff_kernel(cv::Mat &kernel_x, cv::Mat &kernel_y, int kernel_size, int sigma)
    {
        // ガウシアンで平滑化しながら微分するためのカーネルを自前で作る。
        // 現在の本処理では make_differential_image が使われており、
        // この関数は主に test_can_calc_error の確認用。
        kernel_x = cv::Mat::zeros(kernel_size, kernel_size, CV_64F);
        kernel_y = cv::Mat::zeros(kernel_size, kernel_size, CV_64F);
        double C=0;
        for(int y = -kernel_size/2; y <= kernel_size/2; y++)
        {
            for(int x = -kernel_size/2; x <= kernel_size/2; x++)
            {
//                std::cout << kernel_x << std::endl;
//                std::cout << kernel_y << std::endl;
//                std::cout << "exp:" << -(x*x + y*y)/(2.0*sigma*sigma) << std::endl;
                kernel_x.ptr<double>(y+kernel_size/2)[x+kernel_size/2] = x * exp(-(x*x + y*y)/(2.0*sigma*sigma));
                kernel_y.ptr<double>(y+kernel_size/2)[x+kernel_size/2] = y * exp(-(x*x + y*y)/(2.0*sigma*sigma));
                C += x*x*exp(-(x*x + y*y)/(2.0*sigma*sigma));
            }
        }
//        std::cout << kernel_x << std::endl;
//        std::cout << kernel_y << std::endl;
//        std::cout << C << std::endl;

        kernel_x = kernel_x / C;
        kernel_y = kernel_y / C;
    }


    cv::Mat Estimator::calc_rodrigues_R(const cv::Vec3d &n, double theta)
    {
        // ========================================
        // ロドリゲスの回転公式：単位ベクトルnを軸にθだけ回転
        // ========================================
        // R = I + sin(θ)[n]_× + (1-cos(θ))[n]²_×
        // ここで [n]_× は外積を表す行列
        // ========================================

        cv::Mat delta_R = cv::Mat(3,3,CV_64F);

        double s = std::sin(theta);
        double c = std::cos(theta);

        // ロドリゲスの回転公式を直接展開した形
        delta_R.ptr<double>(0)[0] = n[0]*n[0] * (1-c) + c;
        delta_R.ptr<double>(0)[1] = n[0]*n[1] * (1-c) - n[2]*s;
        delta_R.ptr<double>(0)[2] = n[0]*n[2] * (1-c) + n[1]*s;

        delta_R.ptr<double>(1)[0] = n[1]*n[0] * (1-c) + n[2]*s;
        delta_R.ptr<double>(1)[1] = n[1]*n[1] * (1-c) + c;
        delta_R.ptr<double>(1)[2] = n[1]*n[2] * (1-c) - n[0]*s;

        delta_R.ptr<double>(2)[0] = n[2]*n[0] * (1-c) - n[1]*s;
        delta_R.ptr<double>(2)[1] = n[2]*n[1] * (1-c) + n[0]*s;
        delta_R.ptr<double>(2)[2] = n[2]*n[2] * (1-c) + c;

        return delta_R;
    }

    cv::Mat Estimator::rotate_comparison_range(const cv::Mat& img_in, const cv::Mat& R) const{
        // 回転後の画像を生成する確認用関数。
        // comp_range_cartesian_ の各方向をRで回し、全天球画像から小窓をサンプリングする。
//        cv::Mat R_ = cv::Mat::eye(3, 3, CV_64F);
        cv::Mat rotated_img = cv::Mat(comp_range_cartesian_.size(), CV_8U);
        for (int i = 0; i < rotated_img.rows; ++i) {
            for (int j = 0; j < rotated_img.cols; ++j) {
                // カメラの向きを回転
                cv::Mat rotated = R * comp_range_cartesian_.ptr<cv::Vec3d>(i)[j];
//                cv::Mat rotated = R_ * comp_range_cartesian_.ptr<cv::Vec3d>(i)[j];
//                std::cout << comp_range_cartesian_.ptr<cv::Vec3d>(i)[j] << std::endl;
//                std::cout << cartesian2Polar(comp_range_cartesian_.ptr<cv::Vec3d>(i)[j]) << std::endl;

                // 回転後の座標を極座標系に変換
                auto rotated_polar = cartesian2Polar(rotated);
//                std::cout << rotated_polar << std::endl;
                // 極座標系を画像座標系に変換
                cv::Vec2i uv_out = polar2SphericalImg(rotated_polar);

                // 回転後画像の画素値を保存
//                std::cout << uv_out[1] << "," << uv_out[0] << std::endl;
                rotated_img.ptr<uint8_t>(i)[j] = img_in.ptr<uint8_t>(uv_out[1])[uv_out[0]];
            }
        }

        return rotated_img;
    }
    
    cv::Mat Estimator::rotate_comparison_range2(const cv::Mat& img_in) const{
        // ========================================
        // 全天球画像の中心部分のみを切り出す
        // ========================================
        // 回転ではなく単に中心領域を抽出（マッチング用基準クリップ生成）
        cv::Mat rotated_img = cv::Mat(comp_range_cartesian_.size(), CV_8U);
        cv::Vec2i xy_in;

        // 中心領域の開始座標
        int comp_start_x = (img_in.cols-comp_range_cartesian_.cols)/2;
        int comp_start_y = (img_in.rows-comp_range_cartesian_.rows)/2;

        // 中心領域をコピー
        for(xy_in[1]=comp_start_y; xy_in[1]<comp_start_y+comp_range_cartesian_.rows; xy_in[1]++){
            for(xy_in[0]=comp_start_x; xy_in[0]<comp_start_x+comp_range_cartesian_.cols; xy_in[0]++){
                // 画像の画素値を保存
                rotated_img.ptr<uint8_t>(xy_in[1]-comp_start_y)[xy_in[0]-comp_start_x] = img_in.ptr<uint8_t>(xy_in[1])[xy_in[0]];
            }
        }

        return rotated_img;
    }

    cv::Mat Estimator::rotate_clip(const cv::Mat& img_in, const cv::Mat& R) const{
        // ========================================
        // 全天球画像を回転させてバレットタイム用クリップを生成
        // ========================================
        // 出力画像（透視投影カメラから見た画像）
        cv::Mat rotated_img = cv::Mat(clip_range_cartesian_.size(), CV_8UC3);

        // 出力画像の各ピクセルに対応する3次元座標をRで回転し、
        // 全天球画像から対応するピクセルを取得する。
        // ここで作られる画像が最終動画の1フレームになる。
        for (int v = 0; v < rotated_img.rows; ++v) {
            for (int u = 0; u < rotated_img.cols; ++u) {
                // 世界座標系へ移してからカメラの向きを回転
                cv::Mat rotated = R * clip_range_cartesian_.ptr<cv::Vec3d>(v)[u];

                // 回転後の座標を極座標系に変換
                auto rotated_polar = cartesian2Polar(rotated);
                // 極座標系を画像座標系に変換
                cv::Vec2i uv_out = polar2SphericalImg(rotated_polar);

                // 回転後画像の画素値を保存
                rotated_img.ptr<cv::Vec3b>(v)[u] = img_in.ptr<cv::Vec3b>(uv_out[1])[uv_out[0]];
            }
        }

        return rotated_img;
    }

    void Estimator::get_differentiation_of_R(cv::Mat& w1, cv::Mat& w2, cv::Mat& w3, const cv::Mat& R) {
        // Rをx,y,z各軸の微小回転で少し動かしたときの微分行列を作る。
        // estimate_R のLM法で「どの向きに回せば誤差が減るか」を計算するための材料。
        w1 = cv::Mat::zeros(3, 3, CV_64F);
        w1.ptr<double>(1)[0] = -R.ptr<double>(2)[0];
        w1.ptr<double>(1)[1] = -R.ptr<double>(2)[1];
        w1.ptr<double>(1)[2] = -R.ptr<double>(2)[2];
        w1.ptr<double>(2)[0] = R.ptr<double>(1)[0];
        w1.ptr<double>(2)[1] = R.ptr<double>(1)[1];
        w1.ptr<double>(2)[2] = R.ptr<double>(1)[2];
        w2 = cv::Mat::zeros(3, 3, CV_64F);
        w2.ptr<double>(0)[0] = R.ptr<double>(2)[0];
        w2.ptr<double>(0)[1] = R.ptr<double>(2)[1];
        w2.ptr<double>(0)[2] = R.ptr<double>(2)[2];
        w2.ptr<double>(2)[0] = -R.ptr<double>(0)[0];
        w2.ptr<double>(2)[1] = -R.ptr<double>(0)[1];
        w2.ptr<double>(2)[2] = -R.ptr<double>(0)[2];
        w3 = cv::Mat::zeros(3, 3, CV_64F);
        w3.ptr<double>(0)[0] = -R.ptr<double>(1)[0];
        w3.ptr<double>(0)[1] = -R.ptr<double>(1)[1];
        w3.ptr<double>(0)[2] = -R.ptr<double>(1)[2];
        w3.ptr<double>(1)[0] = R.ptr<double>(0)[0];
        w3.ptr<double>(1)[1] = R.ptr<double>(0)[1];
        w3.ptr<double>(1)[2] = R.ptr<double>(0)[2];
    }


    void Estimator::test_can_calc_error(const cv::Mat& img, const cv::Mat& base_clip, const cv::Mat& R_ini) const{
        // 誤差関数の形を確認するためのデバッグ関数。
        // R_iniに角度誤差を加え、角度ごとの誤差と勾配をCSVへ出力する。
        // main.cpp からは通常呼ばれない。
        //csv出力
        std::ofstream ofs(debug_output_path("error.csv"));
        std::ofstream ofs_g(debug_output_path("grad.csv"));

        // 微分カーネル
        cv::Mat kernel_x;  // x方向カーネル
        cv::Mat kernel_y;  // y方向カーネル
        create_smoothing_diff_kernel(kernel_x, kernel_y, 5, 2);


        for(int i=-10;i<=10; i+=1) {
            // パラメータを設定
            double theta = i / 180.0 * M_PI;
            cv::Mat error_R = cv::Mat::zeros(3, 3, CV_64F);
            // ピッチ方向回転
//            std::cout << "pitch" << std::endl;
//            error_R.ptr<double>(0)[0] = std::cos(theta);
//            error_R.ptr<double>(0)[2] = std::sin(theta);
//            error_R.ptr<double>(2)[0] = -std::sin(theta);
//            error_R.ptr<double>(2)[2] = std::cos(theta);
//            error_R.ptr<double>(1)[1] = 1;
            // ロール方向回転
//            std::cout << "roll" << std::endl;
//            error_R.ptr<double>(0)[0] = 1;
//            error_R.ptr<double>(1)[1] = std::cos(theta);
//            error_R.ptr<double>(1)[2] = -std::sin(theta);
//            error_R.ptr<double>(2)[1] = std::sin(theta);
//            error_R.ptr<double>(2)[2] = std::cos(theta);
            // ヨー方向回転
            error_R.ptr<double>(0)[0] = std::cos(theta);
            error_R.ptr<double>(0)[1] = -std::sin(theta);
            error_R.ptr<double>(1)[0] = std::sin(theta);
            error_R.ptr<double>(1)[1] = std::cos(theta);
            error_R.ptr<double>(2)[2] = 1;

            // 追加でロール回転
//            cv::Mat error_R_sub = cv::Mat::zeros(3, 3, CV_64F);
//            std::cout << "roll" << std::endl;
//            error_R_sub.ptr<double>(0)[0] = 1;
//            error_R_sub.ptr<double>(1)[1] = std::cos(theta);
//            error_R_sub.ptr<double>(1)[2] = -std::sin(theta);
//            error_R_sub.ptr<double>(2)[1] = std::sin(theta);
//            error_R_sub.ptr<double>(2)[2] = std::cos(theta);
//            error_R = error_R * error_R_sub;

            // 初期値に追加で誤差用の回転を加える
//            cv::Mat R = R_ini;
            cv::Mat R = error_R * R_ini;

            // 回転行列適用後の画像を生成
            cv::Mat rotated_clip = rotate_comparison_range(img, R);
            if(i<0){
                cv::imwrite(debug_output_path("___" + std::to_string(i) + ".png"), rotated_clip);
            }else{
                cv::imwrite(debug_output_path(std::to_string(i) + ".png"), rotated_clip);
            }

            // 平滑微分画像を生成
            cv::Mat smoothing_diff_image_x;
            cv::Mat smoothing_diff_image_y;
            cv::filter2D(rotated_clip, smoothing_diff_image_x, CV_64F, kernel_x);
            cv::filter2D(rotated_clip, smoothing_diff_image_y, CV_64F, kernel_y);

            // 回転行列を微分した行列を生成
            cv::Mat w1, w2, w3;
            get_differentiation_of_R(w1, w2, w3, R);

//            std::cout << "R\n" << R << std::endl;
//            std::cout << "w1\n" << w1 << std::endl;
//            std::cout << "w2\n" << w2 << std::endl;
//            std::cout << "w3\n" << w3 << std::endl;
//            std::cout << "w1*R.t\n" << w1 * R.t() << std::endl;
//            std::cout << "w2*R.t\n" << w2 * R.t() << std::endl;
//            std::cout << "w3*R.t\n" << w3 * R.t() << std::endl;

            cv::Vec3d grad(0, 0, 0);
            cv::Mat hesse = cv::Mat::zeros(3, 3, CV_64F);
            double sum_of_error = 0;
            for(int y=0; y<base_clip.rows; ++y){
                for(int x=0; x<base_clip.cols; ++x){
                    // 回転後と回転前の画像座標で差分を計算
                    double error = rotated_clip.ptr<uint8_t>(y)[x] - base_clip.ptr<uint8_t>(y)[x];
                    sum_of_error += error*error;

                    // 回転前の三次元座標に各パラメータ微分の回転行列をかける
                    // つまり参照画像の座標系にR.tをかけたやつ
//                    cv::Mat differential_1 = w1 * R.t() * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];
//                    cv::Mat differential_2 = w2 * R.t() * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];
//                    cv::Mat differential_3 = w3 * R.t() * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];
                    cv::Mat differential_1 = w1 * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];
                    cv::Mat differential_2 = w2 * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];
                    cv::Mat differential_3 = w3 * comp_range_cartesian_.ptr<cv::Vec3d>(y)[x];

//                    std::cout << "xyz\n" << comp_range_cartesian_.ptr<cv::Vec3d>(y)[x] << std::endl;
//                    std::cout << "diff_1\n" << differential_1 << std::endl;

//                    std::cout << differential_1.ptr<double>(0)[1] << "," << differential_1.ptr<double>(0)[2]<< ",";
//                    std::cout << differential_2.ptr<double>(0)[1] << "," << differential_2.ptr<double>(0)[2]<< ",";
//                    std::cout << differential_3.ptr<double>(0)[1] << "," << differential_3.ptr<double>(0)[2] << std::endl;

                    // 合成関数の微分による回転後画像のx,y,z方向微分。xは奥行方向なので省略
                    double sy = smoothing_diff_image_x.ptr<double>(y)[x];
                    double sz = smoothing_diff_image_y.ptr<double>(y)[x];
                    double nabla_e1 = sy * differential_1.ptr<double>(0)[1] + sz * differential_1.ptr<double>(0)[2];
                    double nabla_e2 = sy * differential_2.ptr<double>(0)[1] + sz * differential_2.ptr<double>(0)[2];
                    double nabla_e3 = sy * differential_3.ptr<double>(0)[1] + sz * differential_3.ptr<double>(0)[2];

                    grad[0] += error * nabla_e1;
                    grad[1] += error * nabla_e2;
                    grad[2] += error * nabla_e3;

                    hesse.ptr<double>(0)[0] += nabla_e1 * nabla_e1;
                    hesse.ptr<double>(0)[1] += nabla_e1 * nabla_e2;
                    hesse.ptr<double>(0)[2] += nabla_e1 * nabla_e3;
                    hesse.ptr<double>(1)[0] += nabla_e2 * nabla_e1;
                    hesse.ptr<double>(1)[1] += nabla_e2 * nabla_e2;
                    hesse.ptr<double>(1)[2] += nabla_e2 * nabla_e3;
                    hesse.ptr<double>(2)[0] += nabla_e3 * nabla_e1;
                    hesse.ptr<double>(2)[1] += nabla_e3 * nabla_e2;
                    hesse.ptr<double>(2)[2] += nabla_e3 * nabla_e3;
                }
            }
            grad /= num_of_clip_pixel_;
            hesse /= num_of_clip_pixel_;
            sum_of_error /= num_of_clip_pixel_;

            // 誤差と勾配を出力
            ofs << sum_of_error << '\n';
            ofs_g << grad[0] << "," << grad[1] << "," << grad[2] << "," << '\n';
        }
    }
    
    void Estimator::appendToCSV(const std::string& filename, double value1, double value2) {
        // 追記モードでCSVファイルを開く。
        // estimate_R の反復回数と誤差を記録し、収束の様子をあとから確認する。
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << value1 << "," << value2 << "\n";
            file.close();
        } else {
            std::cerr << "Error opening file!" << std::endl;
        }
    }
}
