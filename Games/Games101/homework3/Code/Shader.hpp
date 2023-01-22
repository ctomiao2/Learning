//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_SHADER_H
#define RASTERIZER_SHADER_H
#include <eigen3/Eigen/Eigen>
#include "Texture.hpp"


struct fragment_shader_payload
{
    fragment_shader_payload()
    {
        texture = nullptr;
        texture2 = nullptr;
    }

    fragment_shader_payload(const Eigen::Vector3f& col, const Eigen::Vector3f& nor,const Eigen::Vector2f& tc, Texture* tex, Texture* tex2) :
         color(col), normal(nor), tex_coords(tc), texture(tex), texture2(tex2) {}


    Eigen::Vector3f view_pos;
    Eigen::Vector3f color;
    Eigen::Vector3f normal;
    Eigen::Vector2f tex_coords;
    Texture* texture;
    Texture* texture2;
};

struct vertex_shader_payload
{
    vertex_shader_payload(const Eigen::Vector4f& pos, const Eigen::Vector3f& nor,const Eigen::Vector2f& tc,
        const Eigen::Matrix4f& m, const Eigen::Matrix4f& v, Texture* tex) : 
        position(pos), normal(nor), tex_coords(tc), model(m), view(v), texture(tex) {}
    Eigen::Vector4f position;
    Eigen::Vector2f tex_coords;
    Eigen::Vector3f normal;
    Eigen::Matrix4f model;
    Eigen::Matrix4f view;
    Texture* texture;
};

#endif //RASTERIZER_SHADER_H
