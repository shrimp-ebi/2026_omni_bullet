//
// Created by suhara_sota on 2022/12/21.
//

#include "Estimator.h"

#include <utility>


namespace spherical_bullet_time{
    Estimator::Estimator(cv::Size img_size, cv::Size comp_range, cv::Size clip_range, double focus):
        img_size_(std::move(img_size)){
        // 光軸付近のマッチング範囲の3次元座標をあらかじめ設定
        comp_range_cartesian_ = cv::Mat_<cv::Vec3d>(std::move(comp_range));
        clip_range_cartesian_ = cv::Mat_<cv::Vec3d>(std::move(clip_range));
        Sb = cv::Mat_<cv::Vec3d>(img_size_);
        // 透視投影カメラの向きを正面に向けるための回転行列
        R_wc_ = cv::Mat::zeros(3, 3, CV_64F);
        R_wc_.ptr<double>(0)[1] = 1;
        R_wc_.ptr<double>(1)[2] = 1;
        R_wc_.ptr<double>(2)[0] = 1;
//        std::cout << R_wc_ << std::endl;
        
        // 光軸位置
        int u0_comp = int(comp_range_cartesian_.cols/2);
        int v0_comp = int(comp_range_cartesian_.rows/2);
        int u0_clip = int(clip_range_cartesian_.cols/2);
        int v0_clip = int(clip_range_cartesian_.rows/2);

        //画角の計算
        int We = img_size_.width;
        int He = img_size_.height;
        int Wp_comp = int(comp_range_cartesian_.cols);
        int Hp_comp = int(comp_range_cartesian_.rows);
        int Wp_clip = int(clip_range_cartesian_.cols);
        int Hp_clip = int(clip_range_cartesian_.rows);
        double Theta_comp = 2*asin((M_PI*Wp_comp)/We);
        double Phi_comp = 2*asin((M_PI*Hp_comp)/(2*He));
        double Theta_clip = 2*asin((M_PI*Wp_clip)/We);
        double Phi_clip = 2*asin((M_PI*Hp_clip)/(2*He));
        
        //画素間の長さの計算
        double Delta_x_comp = 2*tan(Theta_comp/2)/Wp_comp;
        double Delta_y_comp = 2*tan(Phi_comp/2)/Hp_comp;     
        double Delta_x_clip = 2*tan(Theta_clip/2)/Wp_clip;
        double Delta_y_clip = 2*tan(Phi_clip/2)/Hp_clip; 

        // Sbを計算
        cv::Vec2i uv;
        for(uv[1]=0; uv[1]<img_size_.height; uv[1]++){
            for(uv[0]=0; uv[0]<img_size_.width; uv[0]++){
                auto polar = sphericalImg2polar(uv, img_size_);
                auto cartesian = polar2Cartesian(polar);
                Sb.ptr<cv::Vec3d>(uv[1])[uv[0]] = cartesian;
//                comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(cartesian_camera_comp);
//                std::cout << Sb.ptr<cv::Vec3d>(uv[1])[uv[0]] << std::endl;
            }
        }

        // カメラ座標系でのピクセル位置を世界座標系での3次元座標の形で保存
        for (int v = 0; v < comp_range_cartesian_.rows; ++v) {
            for (int u = 0; u < comp_range_cartesian_.cols; ++u) {
                cv::Vec3d cartesian_camera_comp = cv::Vec3d((u-u0_comp)*Delta_x_comp, (v-v0_comp)*Delta_y_comp, focus);
                cartesian_camera_comp = cartesian_camera_comp/cv::norm(cartesian_camera_comp);
                comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(R_wc_.t()*cartesian_camera_comp);
//                comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(cartesian_camera_comp);
//                std::cout << comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] << std::endl;
            }
        }
        for (int v = 0; v < clip_range_cartesian_.rows; ++v) {
            for (int u = 0; u < clip_range_cartesian_.cols; ++u) {
                cv::Vec3d cartesian_camera_clip = cv::Vec3d((u-u0_clip)*Delta_x_clip, (v-v0_clip)*Delta_y_clip, focus);
                cartesian_camera_clip = cartesian_camera_clip/cv::norm(cartesian_camera_clip);
                clip_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(R_wc_.t()*cartesian_camera_clip);
//                comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] = cv::Mat(cartesian_camera_comp);
//                std::cout << comp_range_cartesian_.ptr<cv::Vec3d>(v)[u] << std::endl;
            }
        }

        num_of_clip_pixel_ = comp_range_cartesian_.rows*comp_range_cartesian_.cols;
        std::cout << "おわったお" << std::endl;
        
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
    
        //画像を回転する2
    cv::Mat Estimator::rotate_img2(const cv::Mat &img, const cv::Mat &R, const cv::Size& img_size){
        // 出力画像
        cv::Mat img_out;
        // 回転前後の座標を保存する変数
        cv::Vec2i uv_in;
        cv::Vec2i uv_out;
        if(img.type() == CV_8UC3){
        std::cout << "うえだお" << std::endl;
            img_out = cv::Mat(img_size, CV_8UC3);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(uv_out[1])[uv_out[0]];
                    auto rotated_polar = cartesian2Polar(rotated);
                    uv_in = polar2SphericalImg(rotated_polar, img_size);
                    img_out.ptr<cv::Vec3b>(uv_out[1])[uv_out[0]] = img.ptr<cv::Vec3b>(uv_in[1])[uv_in[0]];
//                std::cout << uv_out[1] << "," << uv_out[0] << " : " << uv_in[1] << "," << uv_in[0] << std::endl;
                }
            }
        }else{
        std::cout << "しただお" << std::endl;
            img_out = cv::Mat(img_size, CV_8U);
            for(uv_out[1]=0; uv_out[1]<img_size.height; uv_out[1]++){
                for(uv_out[0]=0; uv_out[0]<img_size.width; uv_out[0]++){
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(uv_out[1])[uv_out[0]];
                    auto rotated_polar = cartesian2Polar(rotated);
                    uv_in = polar2SphericalImg(rotated_polar, img_size);
                    img_out.ptr<uint8_t>(uv_out[1])[uv_out[0]] = img.ptr<cv::uint8_t>(uv_in[1])[uv_in[0]];
//                std::cout << uv_out[1] << "," << uv_out[0] << " : " << uv_in[1] << "," << uv_in[0] << std::endl;
                }
            }
        }
        std::cout << "かえしたお" << std::endl;
        return img_out;
    }


    //入力画像を出力画像へと変換するような回転行列を推定する
    cv::Mat Estimator::estimate_R(const cv::Mat &img_in, const cv::Mat &base_clip, const cv::Mat& R_ini)
    {
        // 初期値設定
        cv::Mat R = R_ini.clone();
//        std::cout << R << std::endl;

        // LM法の定数を設定
        double lambda = 0.0001;

//        cv::Mat kernel_x;  // x方向カーネル
//        cv::Mat kernel_y;  // y方向カーネル
//        create_smoothing_diff_kernel(kernel_x, kernel_y, 5, 2);
        // 微分画像を生成
        cv::Mat diff_image_x;
        cv::Mat diff_image_y;
        cv::Mat diff_image_z;
        make_differential_image(img_in, diff_image_x, diff_image_y, diff_image_z);

        int count=0;
        while(true) {
            // 基準画像Sbの点を回転させて、Srから似た見え方をする領域を探す。    
            
//            cv::Mat rotated_img = rotate_comparison_range(img_in, R);
//            cv::imwrite("./totyu/"+std::to_string(count)+".png", rotated_img);

            // 回転行列を微分した行列を生成
            cv::Mat w1, w2, w3;
            get_differentiation_of_R(w1, w2, w3, R);

//            std::cout << "R\n" << R << std::endl;
//            std::cout << "w1\n" << w1 << std::endl;
//            std::cout << "w2\n" << w2 << std::endl;
//            std::cout << "w3\n" << w3 << std::endl;

            cv::Vec3d grad(0, 0, 0);
            cv::Mat hesse = cv::Mat::zeros(3, 3, CV_64F);
            double sum_of_error = 0;
/*            for(int y=0; y<base_clip.rows; ++y){
                for(int x=0; x<base_clip.cols; ++x){*/
            cv::Vec2i xy_in;
            cv::Vec2i xy_out;    
            int comp_start_x = (img_in.cols-comp_range_cartesian_.cols)/2;
            int comp_start_y = (img_in.rows-comp_range_cartesian_.rows)/2;
//            std::cout << "comp_start_x,comp_start_y" << comp_start_x << "," << comp_start_y << std::endl;
            for(xy_in[1]=comp_start_y; xy_in[1]<comp_start_y+comp_range_cartesian_.rows; xy_in[1]++){//中心付近だけ比較したいな
                for(xy_in[0]=comp_start_x; xy_in[0]<comp_start_x+comp_range_cartesian_.cols; xy_in[0]++){
//                 std::cout << "xy_in: " << xy_in[0] << "," << xy_in[1] << std::endl;
                    // 回転後と回転前の画像座標で差分を計算
                    cv::Mat rotated = R * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    auto rotated_polar = cartesian2Polar(rotated);
                    xy_out = polar2SphericalImg(rotated_polar, img_in.size());
                    double error = img_in.ptr<uint8_t>(xy_out[1])[xy_out[0]] - base_clip.ptr<uint8_t>(xy_in[1])[xy_in[0]];
                    sum_of_error += error*error;
//                 std::cout << "xy_out: " << xy_out[0] << "," << xy_out[1] << std::endl;

                    // 回転前の三次元座標に各パラメータ微分の回転行列をかける
                    // つまり参照画像の座標系にR.tをかけたやつ
                    cv::Mat differential_1 = w1 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    cv::Mat differential_2 = w2 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];
                    cv::Mat differential_3 = w3 * Sb.ptr<cv::Vec3d>(xy_in[1])[xy_in[0]];

                    double sx = diff_image_x.ptr<double>(xy_out[1])[xy_out[0]];
                    double sy = diff_image_y.ptr<double>(xy_out[1])[xy_out[0]];
                    double sz = diff_image_z.ptr<double>(xy_out[1])[xy_out[0]];
//                    double sy = smoothing_diff_image_x.ptr<double>(y)[x];
//                    double sz = smoothing_diff_image_y.ptr<double>(y)[x];
//                    std::cout << "sx,sy,sz = " << double(sx) << ", " << double(sy) << ", " << double(sz) << std::endl;

                    double nabla_e1 = sx * differential_1.ptr<double>(0)[0] + sy * differential_1.ptr<double>(0)[1] + sz * differential_1.ptr<double>(0)[2];
                    double nabla_e2 = sx * differential_2.ptr<double>(0)[0] + sy * differential_2.ptr<double>(0)[1] + sz * differential_2.ptr<double>(0)[2];
                    double nabla_e3 = sx * differential_3.ptr<double>(0)[0] + sy * differential_3.ptr<double>(0)[1] + sz * differential_3.ptr<double>(0)[2];

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
//            std::cout << "grad : " << grad << std::endl;
//            std::cout << "hesse : " << hesse << std::endl;

            cv::Mat delta_w;
            cv::Mat delta_R;
            cv::Mat R_new;
            double sum_of_error_new;

            cv::Mat rotated_img_new = cv::Mat(comp_range_cartesian_.rows, comp_range_cartesian_.cols, CV_8U);
            while(true) {
                cv::Mat hesse_cp = hesse.clone();

                //LM法のlambda * Iを加算
                hesse_cp.ptr<double>(0)[0] += 1 + lambda;
                hesse_cp.ptr<double>(1)[1] += 1 + lambda;
                hesse_cp.ptr<double>(2)[2] += 1 + lambda;
//                std::cout << "hesse_cp\n" << hesse_cp << std::endl;

                //逆行列計算
                cv::Mat hesse_inv = hesse_cp.inv();
//                std::cout << "hesse_inv\n" << hesse_inv << std::endl;

                //更新値計算
                delta_w = -hesse_inv * grad;
//                std::cout << "delta_w\n" << delta_w << std::endl;

                // パラメータの変化量を回転行列化
                double norm_w = cv::norm(delta_w);
//                std::cout << "norm_delta_w: " <<cv::norm(delta_w) << std::endl;                
                // 変化が無い場合0除算が発生するため推定を終了する。
                if(norm_w < 1.0e-16){
//                    std::cout << "norm_delta_w: " <<cv::norm(delta_w) << std::endl;
                    std::cout << "こっちだよ" <<cv::norm(delta_w) << std::endl;
                    return R;
                }
                cv::Mat normalized_w = delta_w/norm_w;
                delta_R = calc_rodrigues_R(normalized_w, norm_w);
//                std::cout << "delta_R\n" << delta_R << std::endl;

                //更新したパラメータを用いた回転行列を計算
                R_new = delta_R * R;
//                std::cout << "R_new\n" << R_new << std::endl;

                //更新後の回転行列で誤差を再計算
                sum_of_error_new = 0;
                cv::Vec2i xy_in_2;
                for(xy_in_2[1]=comp_start_y; xy_in_2[1]<comp_start_y+comp_range_cartesian_.rows; xy_in_2[1]++){//中心付近だけ比較したいな
                    for(xy_in_2[0]=comp_start_x; xy_in_2[0]<comp_start_x+comp_range_cartesian_.cols; xy_in_2[0]++){
                        // 描画範囲の3次元点を回転
                        cv::Mat rotated = R_new * Sb.ptr<cv::Vec3d>(xy_in_2[1])[xy_in_2[0]];
//                        std::cout << comp_range_cartesian_.ptr<cv::Vec3d>(y)[x] << std::endl;
//                        std::cout << rotated << std::endl;
                        // 回転後の座標を極座標系に変換
                        auto rotated_polar = cartesian2Polar(rotated);
                        // 極座標系を画像座標系に変換
                        cv::Vec2i uv_out = polar2SphericalImg(rotated_polar);

                        // 誤差計算
                        double error = img_in.ptr<uint8_t>(uv_out[1])[uv_out[0]] - base_clip.ptr<uint8_t>(xy_in_2[1])[xy_in_2[0]];
                        sum_of_error_new += error*error;

                        // 回転後画像の画素値を保存
                        rotated_img_new.ptr<uint8_t>(uv_out[1])[uv_out[0]] = img_in.ptr<uint8_t>(uv_out[1])[uv_out[0]];
//                        std::cout << uv_out[1] << "," << uv_out[0] << std::endl;
                    }
                }
//                cv::imwrite("applied_new.png", rotated_img_new);
                sum_of_error_new /= num_of_clip_pixel_;

                //誤差値比較. 誤差が小さくなっていないか減少量が極端に少ない場合はやり直し
                std::cout << "error : " << sum_of_error << std::endl;
                std::cout << "new_error : " << sum_of_error_new << std::endl;
                double diff = sum_of_error - sum_of_error_new;
//                std::cout << "diff : " << abs(diff) << std::endl;
		appendToCSV("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/研究室2/data.csv", count, sum_of_error_new);
		count++;
                if (sum_of_error > sum_of_error_new || abs(diff) < 0.1) break;
                lambda *= 10;
            }

            std::cout << "更新!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            std::cout << "error : " << sum_of_error << std::endl;
            std::cout << "new error : " << sum_of_error_new << std::endl;
//            cv::imwrite("./totyu/"+std::to_string(count)+".png", rotated_img);
            appendToCSV("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/研究室2/data.csv", count, sum_of_error_new);
            count++;

            //パラメータ更新
            R = R_new;
//            std::cout << "R_new" << R << std::endl;

            //終了条件
            //それぞれのパラメータの更新値が十分小さいことを確認
            std::cout << cv::norm(delta_w) << std::endl;
            if(cv::norm(delta_w) < 1.0e-5){
                std::cout << "estimated R" << R << std::endl;
//                appendToCSV("/home/h233304/全天球画像でのバレットタイム10/cmake-build-debug/研究室2/data.csv", sum_of_error_new, cv::norm(delta_w));
                return R;
            }

            lambda /= 10;
        }
    }


    //微分画像を計算する
    void Estimator::make_differential_image(const cv::Mat& img_in, cv::Mat& diff_image_x, cv::Mat& diff_image_y, cv::Mat& diff_image_z){
        int ksize = 3;  // カーネルサイズ（5x5）
        double sigma = 2;  // ガウシアンカーネルの標準偏差
        cv::Mat blurred;
        cv::GaussianBlur(img_in, blurred, cv::Size(ksize, ksize), sigma);
        blurred.convertTo(blurred, CV_64F);
        diff_image_x = cv::Mat(img_in.rows, img_in.cols, CV_64F);
        diff_image_y = cv::Mat(img_in.rows, img_in.cols, CV_64F);
        diff_image_z = cv::Mat(img_in.rows, img_in.cols, CV_64F);
        
/*        cv::Vec2i uv_in(0,0);
        cv::Vec2i uv_in_theta(1,0);
        cv::Vec2i uv_in_phi(0,1);
        
        auto polar = sphericalImg2polar(uv_in, img_in.size());
        auto polar_theta = sphericalImg2polar(uv_in_theta, img_in.size());
        auto polar_phi = sphericalImg2polar(uv_in_phi, img_in.size());
        std::cout << "polar_theta: " << polar_theta << std::endl;
        std::cout << "polar_phi: " << polar_phi << std::endl;
                  
        double par_d_theta = polar_theta[0]-polar[0]; //∂θを計算
        double par_d_phi = polar_phi[1]-polar[1]; //∂φを計算
        std::cout << "par_d_theta, par_d_phi: " << par_d_theta << "," << par_d_phi << std::endl;*/
        
        cv::Vec2i uv_in;   
        
        for(uv_in[1]=0; uv_in[1]<img_in.rows; uv_in[1]++){
            for(uv_in[0]=0; uv_in[0]<img_in.cols; uv_in[0]++){
                auto polar = sphericalImg2polar(uv_in, img_in.size());
                double s_theta = std::sin(polar[0]);
                double s_phi = std::sin(polar[1]);
                double c_theta = std::cos(polar[0]);
                double c_phi = std::cos(polar[1]);
                
                double par_d_theta = sphericalImg2polar(cv::Vec2i((uv_in[0] + 1) % img_in.cols, uv_in[1]), img_in.size())[0] - sphericalImg2polar(cv::Vec2i((uv_in[0] - 1 + img_in.cols) % img_in.cols, uv_in[1]), img_in.size())[0];
                double par_d_phi = sphericalImg2polar(cv::Vec2i(uv_in[0], (uv_in[1] + 1) % img_in.rows), img_in.size())[1] - sphericalImg2polar(cv::Vec2i(uv_in[0], (uv_in[1] - 1 + img_in.rows) % img_in.rows), img_in.size())[1];
//                double par_d_theta = sphericalImg2polar(cv::Vec2i((uv_in[0] + 1) % img_in.cols, uv_in[1]), img_in.size())[0] - polar[0];
//                double par_d_phi = sphericalImg2polar(cv::Vec2i(uv_in[0], (uv_in[1] - 1 + img_in.rows) % img_in.rows), img_in.size())[1] - polar[1];

                double par_d_S_theta = blurred.ptr<double>(uv_in[1])[(uv_in[0] + 1) % img_in.cols]-blurred.ptr<double>(uv_in[1])[(uv_in[0] - 1 + img_in.cols) % img_in.cols]; //∂Sを計算
                double par_d_S_phi = blurred.ptr<double>((uv_in[1] + 1) % img_in.rows)[uv_in[0]]-blurred.ptr<double>((uv_in[1] - 1 + img_in.rows) % img_in.rows)[uv_in[0]];         
//                double par_d_S_theta = blurred.ptr<double>(uv_in[1])[(uv_in[0] + 1) % img_in.cols]-blurred.ptr<double>(uv_in[1])[uv_in[0]]; //∂Sを計算
//                double par_d_S_phi = blurred.ptr<double>((uv_in[1] - 1 + img_in.rows) % img_in.rows)[uv_in[0]]-blurred.ptr<double>(uv_in[1])[uv_in[0]];
//                std::cout << "par_d_S_theta, par_d_S_phi: " << par_d_S_theta << "," << par_d_S_phi << std::endl;        
                    
                double diffS_theta = par_d_S_theta/par_d_theta; //∂S/∂θを計算
                double diffS_phi = par_d_S_phi/par_d_phi; //∂S/∂φを計算
//                std::cout << "diffS_theta, diffS_phi: " << diffS_theta << "," << diffS_phi << std::endl;
                
                if(std::abs(s_phi)<=1.0e-10){
                    diff_image_x.ptr<double>(uv_in[1])[uv_in[0]] = 0;
                    diff_image_y.ptr<double>(uv_in[1])[uv_in[0]] = 0;
                }else{
                    diff_image_x.ptr<double>(uv_in[1])[uv_in[0]] = (-diffS_theta*s_theta/s_phi)+(diffS_phi*c_phi*c_theta);
                    diff_image_y.ptr<double>(uv_in[1])[uv_in[0]] = (diffS_theta*c_theta/s_phi)+(diffS_phi*c_phi*s_theta);
                }
                
                diff_image_z.ptr<double>(uv_in[1])[uv_in[0]] = (diffS_theta*0)+(-diffS_phi*s_phi);
            }
        }
        
        
        cv::Mat output_x, output_y, output_z;
        diff_image_x.convertTo(output_x, CV_8U, 255.0); // スケール変換
        diff_image_y.convertTo(output_y, CV_8U, 255.0);
        diff_image_z.convertTo(output_z, CV_8U, 255.0);
        
//        cv::imwrite("/home/h233304/全天球画像でのバレットタイム11/cmake-build-debug/猫/diff_image_x.png", output_x);
//        cv::imwrite("/home/h233304/全天球画像でのバレットタイム11/cmake-build-debug/猫/diff_image_y.png", output_y);
//        cv::imwrite("/home/h233304/全天球画像でのバレットタイム11/cmake-build-debug/猫/diff_image_z.png", output_z);
    }

    // 画像座標系から極座標系に変換する
    cv::Vec2d Estimator::sphericalImg2polar(const cv::Vec2i& uv) const{
        int u = uv[0];
        int v = uv[1];

        double theta = (u - img_size_.width * 0.5) / img_size_.width * 2 * M_PI;
//        double phi = -(v - img_size_.height * 0.5) / img_size_.height * M_PI;
        double phi = -double(v - img_size_.height) / img_size_.height * M_PI;

        return cv::Vec2d(theta, phi);
    }

    cv::Vec2d Estimator::sphericalImg2polar(const cv::Vec2i& uv, const cv::Size& size){
        int u = uv[0];
        int v = uv[1];

        double theta = (u - size.width * 0.5) / size.width * 2 * M_PI;
    //        double phi = -(v - size.height * 0.5) / size.height * M_PI;
        double phi = -double(v - size.height) / size.height * M_PI;

        return cv::Vec2d(theta, phi);
    }


    // 極座標系から画像座標系に変換する
    cv::Vec2i Estimator::polar2SphericalImg(const cv::Vec2d& polar) const{
        double theta = polar[0];
        double phi = polar[1];

        int u = int((theta + M_PI)*img_size_.width/(2*M_PI));
//        int v = int(-(phi - M_PI/2.0)*(img_size_.height)/M_PI);  // x軸が光軸版？
        int v = int(-(phi - M_PI)*img_size_.height/M_PI);

        return cv::Vec2i(u, v);
    }

    // 極座標系から画像座標系に変換する
    cv::Vec2i Estimator::polar2SphericalImg(const cv::Vec2d& polar, const cv::Size& size){
        double theta = polar[0];
        double phi = polar[1];

        int u = int((theta + M_PI)*size.width/(2*M_PI));
    //        int v = int(-(phi - M_PI/2.0)*(size.height)/M_PI);  // x軸が光軸版？
        int v = int(-(phi - M_PI)*size.height/M_PI);

        return cv::Vec2i(u, v);
    }

    // 極座標系から直交座標系に変換する
    cv::Vec3d Estimator::polar2Cartesian(const cv::Vec2d& polar)
    {
        double theta = polar[0];
        double phi = polar[1];

//        double x = std::cos(phi) * std::cos(theta);
//        double y = std::cos(phi) * std::sin(theta);
//        double z = std::sin(phi);  // zは本来負の符号だけど回転方向逆にしたいから反転してる←これやっぱまずかった
        double x = std::sin(phi) * std::cos(theta);
        double y = std::sin(phi) * std::sin(theta);
        double z = std::cos(phi);

        return cv::Vec3d(x, y, z);
    }

    // 直交座標系から極座標系に変換する
    cv::Vec2d Estimator::cartesian2Polar(const cv::Vec3d& cartesian){
        double x = cartesian[0];
        double y = cartesian[1];
        double z = cartesian[2];

//        double theta = std::atan2(y, x);
//        double phi = std::asin(z/cv::norm(cartesian));
        double theta = std::atan2(y, x);
        double phi = std::acos(z/cv::norm(cartesian));

        return cv::Vec2d(theta, phi);
    }

    // 画像座標点と回転行列を入力として回転後の画像座標を返す関数
    void Estimator::uv2uv_rotation(const cv::Vec2i& uv_in, cv::Vec2i& uv_out, const cv::Mat& R, const cv::Size& img_size){
        auto polar = sphericalImg2polar(uv_in, img_size);
        auto cartesian = polar2Cartesian(polar);
        cv::Mat rotated = R * (cartesian);
        auto rotated_polar = cartesian2Polar(rotated);
        uv_out = polar2SphericalImg(rotated_polar, img_size);
    }

    void Estimator::create_smoothing_diff_kernel(cv::Mat &kernel_x, cv::Mat &kernel_y, int kernel_size, int sigma)
    {
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
        // nは単位ベクトル, thetaは回転量
        cv::Mat delta_R = cv::Mat(3,3,CV_64F);

        double s = std::sin(theta);
        double c = std::cos(theta);

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
        // 回転後の画像を生成
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
        // 回転後の画像を生成
//        cv::Mat R_ = cv::Mat::eye(3, 3, CV_64F);
            cv::Mat rotated_img = cv::Mat(comp_range_cartesian_.size(), CV_8U);
            cv::Vec2i xy_in; 
            int comp_start_x = (img_in.cols-comp_range_cartesian_.cols)/2;
            int comp_start_y = (img_in.rows-comp_range_cartesian_.rows)/2;
//            std::cout << "comp_start_x,comp_start_y" << comp_start_x << "," << comp_start_y << std::endl;
            for(xy_in[1]=comp_start_y; xy_in[1]<comp_start_y+comp_range_cartesian_.rows; xy_in[1]++){//中心付近だけ比較したいな
                for(xy_in[0]=comp_start_x; xy_in[0]<comp_start_x+comp_range_cartesian_.cols; xy_in[0]++){
                // 画像の画素値を保存
                rotated_img.ptr<uint8_t>(xy_in[1]-comp_start_y)[xy_in[0]-comp_start_x] = img_in.ptr<uint8_t>(xy_in[1])[xy_in[0]];
            }
        }

        return rotated_img;
    }

    cv::Mat Estimator::rotate_clip(const cv::Mat& img_in, const cv::Mat& R) const{
        // 回転後の画像を生成
        cv::Mat rotated_img = cv::Mat(clip_range_cartesian_.size(), CV_8UC3);
        // 透視投影カメラの向きを正面に向けるための回転行列

        // カメラ座標系でのピクセル位置を世界座標系での3次元座標の形で保存
        for (int v = 0; v < rotated_img.rows; ++v) {
            for (int u = 0; u < rotated_img.cols; ++u) {
                // 世界座標系へ移してからカメラの向きを回転
                cv::Mat rotated = R * clip_range_cartesian_.ptr<cv::Vec3d>(v)[u];
//                std::cout << clip_range_cartesian_.ptr<cv::Vec3d>(v)[u] << std::endl;
//                std::cout << cartesian2Polar(clip_range_cartesian_.ptr<cv::Vec3d>(v)[u]) << std::endl;

                // 回転後の座標を極座標系に変換
                auto rotated_polar = cartesian2Polar(rotated);
//                std::cout << rotated_polar << std::endl;
                // 極座標系を画像座標系に変換
                cv::Vec2i uv_out = polar2SphericalImg(rotated_polar);

                // 回転後画像の画素値を保存
//                std::cout << uv_out[1] << "," << uv_out[0] << std::endl;
                rotated_img.ptr<cv::Vec3b>(v)[u] = img_in.ptr<cv::Vec3b>(uv_out[1])[uv_out[0]];
            }
        }

    return rotated_img;
}

    void Estimator::get_differentiation_of_R(cv::Mat& w1, cv::Mat& w2, cv::Mat& w3, const cv::Mat& R) {
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
        //csv出力
        std::ofstream ofs("./勾配test/error.csv");
        std::ofstream ofs_g("./勾配test/grad.csv");

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
            std::cout << "yaw" << std::endl;
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
                cv::imwrite("./勾配test/___"+std::to_string(i)+".png", rotated_clip);
            }else{
                cv::imwrite("./勾配test/"+std::to_string(i)+".png", rotated_clip);
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
        // 追記モードでCSVファイルを開く
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << value1 << "," << value2 << "\n";
            file.close();
        } else {
            std::cerr << "Error opening file!" << std::endl;
        }
    }
}
