//
// Created by goksu on 4/6/19.
//

#pragma once

#include <eigen3/Eigen/Eigen>
#include <algorithm>
#include "global.hpp"
#include "Triangle.hpp"
using namespace Eigen;

namespace rst
{
    enum class Buffers
    {
        Color = 1,
        Depth = 2
    };

    inline Buffers operator|(Buffers a, Buffers b)
    {
        return Buffers((int)a | (int)b);
    }

    inline Buffers operator&(Buffers a, Buffers b)
    {
        return Buffers((int)a & (int)b);
    }

    enum class Primitive
    {
        Line,
        Triangle
    };

    /*
     * For the curious : The draw function takes two buffer id's as its arguments. These two structs
     * make sure that if you mix up with their orders, the compiler won't compile it.
     * Aka : Type safety
     * */
    struct pos_buf_id
    {
        int pos_id = 0;
    };

    struct ind_buf_id
    {
        int ind_id = 0;
    };

    struct col_buf_id
    {
        int col_id = 0;
    };

    class rasterizer
    {
    public:
        rasterizer(int w, int h);
        pos_buf_id load_positions(const std::vector<Eigen::Vector3f>& positions);
        ind_buf_id load_indices(const std::vector<Eigen::Vector3i>& indices);
        col_buf_id load_colors(const std::vector<Eigen::Vector3f>& colors);

        void set_model(const Eigen::Matrix4f& m);
        void set_view(const Eigen::Matrix4f& v);
        void set_projection(const Eigen::Matrix4f& p);

        void set_pixel(int x, int y, const Eigen::Vector3f& color);

        void clear(Buffers buff);

        void draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type);

        std::vector<Eigen::Vector3f>& frame_buffer() { return m_frame_buf; }

        void set_multi_sample(int n) { m_super_sample = n; }

    private:
        void draw_line(Eigen::Vector3f begin, Eigen::Vector3f end);

        void rasterize_triangle(const Triangle& t);

        void calc_interpolated_color_and_depth(const Triangle& t, int ix, int iy, float x, float y, int ss_index=-1);

        // VERTEX SHADER -> MVP -> Clipping -> /.W -> VIEWPORT -> DRAWLINE/DRAWTRI -> FRAGSHADER

    private:
        Eigen::Matrix4f m_model;
        Eigen::Matrix4f m_view;
        Eigen::Matrix4f m_projection;

        std::map<int, std::vector<Eigen::Vector3f>> m_pos_buf;
        std::map<int, std::vector<Eigen::Vector3i>> m_ind_buf;
        std::map<int, std::vector<Eigen::Vector3f>> m_col_buf;

        // 插值后的最终pixel color
        std::vector<Eigen::Vector3f> m_frame_buf;
        // 完全在三角形内无需多重采用, 一个像素点用一个深度值表示即可节省内存
        std::vector<float> m_depth_buf;
        // 完全在三角形内无需多重采用, 一个像素点用一个颜色值表示即可节省内存
        std::vector<Eigen::Vector3f> m_pixel_buf;
        // 仅边缘少数像素点需要多重采样, 一个像素点划分多个区域保存多个深度值
        std::map<int, std::vector<float>> m_super_sample_depth_buf;
         // 仅边缘少数像素点需要多重采样, 一个像素点划分多个区域保存多个颜色值
        std::map<int, std::vector<Eigen::Vector3f>> m_super_sample_color_buf;
        
        int get_index(int x, int y);

        int m_width, m_height;
        int m_super_sample = 0; // m_super_sample * m_super_sample multisample
        int m_next_id = 0;
        int get_next_id() { return m_next_id++; }
    };
}
