//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getLinearColor(float u, float v)
    {
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
        float u_img = u * width;
        float v_img = (1 - v) * height;
        float iu = (int)u_img, iv = (int)v_img;
        float iu2, iv2;

        if ((u_img - iu) <= 0.5)
            iu2 = std::max(0.0f, iu - 1);
        else
            iu2 = iu + 1;

        if ((v_img - iv) <= 0.5)
            iv2 = std::max(0.0f, iv - 1);
        else
            iv2 = iv + 1;

        float u_lerp = 1.0f - std::abs(u_img - iu - 0.5);
        float v_lerp = 1.0f - std::abs(v_img - iv - 0.5);
        //std::cout << iu << '\t' << iv << '\n';
        auto c1 = image_data.at<cv::Vec3b>(iu, iv);
        auto c2 = image_data.at<cv::Vec3b>(iu2, iv);
        auto c3 = image_data.at<cv::Vec3b>(iu, iv2);
        auto c4 = image_data.at<cv::Vec3b>(iu2, iv2);
        Eigen::Vector3f c12 = {
            u_lerp * c1[0] + (1 - u_lerp) * c2[0],
            u_lerp * c1[1] + (1 - u_lerp) * c2[1],
            u_lerp * c1[2] + (1 - u_lerp) * c2[2]
        };

        Eigen::Vector3f c34 = {
            u_lerp * c3[0] + (1 - u_lerp) * c4[0],
            u_lerp * c3[1] + (1 - u_lerp) * c4[1],
            u_lerp * c3[2] + (1 - u_lerp) * c4[2]
        };

        Eigen::Vector3f color = {
            v_lerp * c12[0] + (1 - v_lerp) * c34[0],
            v_lerp * c12[1] + (1 - v_lerp) * c34[1],
            v_lerp * c12[2] + (1 - v_lerp) * c34[2]
        };

        return color;
    }

    Eigen::Vector3f getColor(float u, float v)
    {
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

};
#endif //RASTERIZER_TEXTURE_H
