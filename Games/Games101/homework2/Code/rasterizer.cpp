// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    m_pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    m_ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    m_col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v)
{   
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    float AP_x = x - _v[0].x(), AP_y = y - _v[0].y();
    float BP_x = x - _v[1].x(), BP_y = y - _v[1].y();
    float CP_x = x - _v[2].x(), CP_y = y - _v[2].y();
    float AB_x = _v[1].x() - _v[0].x(), AB_y = _v[1].y() - _v[0].y();
    float BC_x = _v[2].x() - _v[1].x(), BC_y = _v[2].y() - _v[1].y();
    float CA_x = _v[0].x() - _v[2].x(), CA_y = _v[0].y() - _v[2].y();
    float AB_Cross_AP = AB_x * AP_y - AB_y * AP_x;
    float BC_Cross_BP = BC_x * BP_y - BC_y * BP_x;
    float CA_Cross_CP = CA_x * CP_y - CA_y * CP_x;
    return (AB_Cross_AP >= 0 && BC_Cross_BP >= 0 && CA_Cross_CP >= 0) || (AB_Cross_AP <= 0 && BC_Cross_BP <= 0 && CA_Cross_CP <= 0);
}

static Vector3f getColor(float alpha, float beta, const Vector3f* _v)
{
    float gamma = 1 - alpha - beta;
    float r = alpha * _v[0].x() + beta * _v[1].x() + gamma * _v[2].x();
    float g = alpha * _v[0].y() + beta * _v[1].y() + gamma * _v[2].y();
    float b = alpha * _v[0].z() + beta * _v[1].z() + gamma * _v[2].z();
    Eigen::Vector3f color = Eigen::Vector3f(
        255.0f * std::max(0.0f, std::min(1.0f, r)),
        255.0f * std::max(0.0f, std::min(1.0f, g)),
        255.0f * std::max(0.0f, std::min(1.0f, b))
    );

    return color;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = m_pos_buf[pos_buffer.pos_id];
    auto& ind = m_ind_buf[ind_buffer.ind_id];
    auto& col = m_col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = m_projection * m_view * m_model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*m_width*(vert.x()+1.0);
            vert.y() = 0.5*m_height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    // TODO : Find out the bounding box of current triangle.
    float min_x = std::min(t.v[2].x(), std::min(t.v[0].x(), t.v[1].x())) - 1;
    float max_x = std::max(t.v[2].x(), std::max(t.v[0].x(), t.v[1].x())) + 1;
    float min_y = std::min(t.v[2].y(), std::min(t.v[0].y(), t.v[1].y())) - 1;
    float max_y = std::max(t.v[2].y(), std::max(t.v[0].y(), t.v[1].y())) + 1;

    float super_sample_step = 0;
    int super_sample_times = 0;
    if (m_super_sample > 0) {
        super_sample_step = 1.0f/m_super_sample;
        super_sample_times = m_super_sample * m_super_sample;
    }

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            int ind = (m_height-1-y) * m_width + x;
            if (m_super_sample <= 0) {
                if (insideTriangle(x, y, t.v))
                    calc_interpolated_color_and_depth(t, x, y, x, y, -1);
            }
            else {
                std::vector<int> in_triangle_ss_indexes;
                bool in_triangle = false;
                bool all_in_triangle = true;
                float ss_start_x = x - 0.5 + 0.5 * super_sample_step;
                float ss_start_y = y - 0.5 + 0.5 * super_sample_step;
                // 遍历该像素的所有超采样点, 看是否有一个在三角形内
                for (int si = 0; si < m_super_sample; ++si) {
                    for (int sj = 0; sj < m_super_sample; ++sj) {
                        float sx = ss_start_x + super_sample_step * si;
                        float sy = ss_start_y + super_sample_step * sj;
                        bool cur_in_triangle = insideTriangle(sx, sy, t.v);
                        in_triangle_ss_indexes.push_back(cur_in_triangle);
                        if (cur_in_triangle)
                            in_triangle = true;
                        else
                            all_in_triangle = false;
                    }
                }

                if (!in_triangle)
                    continue;

                // 整个像素都在三角形内, 那么直接用一个点表示好了, 节省时空效率
                if (all_in_triangle) {
                    calc_interpolated_color_and_depth(t, x, y, x, y, -1);
                    continue;
                }

                // 只要有1个超采样点落入三角形内就要对该像素进行处理
                if (m_super_sample_depth_buf.find(ind) == m_super_sample_depth_buf.end()) {
                    // 初始化超采样深度\颜色 buffer
                    std::vector<float> ss_depth_list;
                    for (int ss_idx = 0; ss_idx < super_sample_times; ++ss_idx)
                        ss_depth_list.push_back(std::numeric_limits<float>::infinity());
                    m_super_sample_depth_buf.emplace(ind, ss_depth_list);

                    std::vector<Eigen::Vector3f> ss_color_list;
                    Eigen::Vector3f ZERO = {0.0f, 0.0f, 0.0f};
                    for (int ss_idx = 0; ss_idx < super_sample_times; ++ss_idx)
                        ss_color_list.push_back(ZERO);
                    m_super_sample_color_buf.emplace(ind, ss_color_list);
                }

                for (int si = 0; si < m_super_sample; ++si) {
                    for (int sj = 0; sj < m_super_sample; ++sj) {
                        float sx = ss_start_x + super_sample_step * si;
                        float sy = ss_start_y + super_sample_step * sj;
                        int ss_ind = si * m_super_sample + sj;
                        // 该超采样点在三角形内
                        if (in_triangle_ss_indexes[ss_ind]) {
                            calc_interpolated_color_and_depth(t, x, y, sx, sy, ss_ind);    
                        }
                    }
                }
            }

        }
    }
}

void rst::rasterizer::calc_interpolated_color_and_depth(const Triangle& t, int ix, int iy, float x, float y, int ss_index)
{
    auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
    auto v = t.toVector4();
    float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    z_interpolated *= w_reciprocal;
    int ind = (m_height-1-iy) * m_width + ix;

    // 该像素任意一个采样点深度都不大于m_depth_buf[ind], 如果插值得到的深度比这个还要大那就不要再比了
    if (z_interpolated > m_depth_buf[ind])
        return;
    
    // 计算插值颜色
    Eigen::Vector3f interp_color = getColor(alpha, beta, t.color);

    // 没开multisample或这个像素完全在三角形内(即像素点各采超样点深度值都一样没必要保留多份)
    if (m_super_sample <= 0 || ss_index < 0) {
        m_depth_buf[ind] = z_interpolated;
        m_pixel_buf[ind] << interp_color.x(), interp_color.y(), interp_color.z();
    }
    else {
        // 超采样点深度比较
        if (z_interpolated > m_super_sample_depth_buf[ind][ss_index])
            return;
        m_super_sample_depth_buf[ind][ss_index] = z_interpolated;
        m_super_sample_color_buf[ind][ss_index] << interp_color.x(), interp_color.y(), interp_color.z();
    }

    Eigen::Vector3f pix_color = m_pixel_buf[ind];
    float pix_depth = m_depth_buf[ind];

    // 更新m_frame_buf
    int super_sample_times =  m_super_sample * m_super_sample;
    float w = 1.0f/super_sample_times;
    float r = 0, g = 0, b = 0;

    if (super_sample_times > 0 && m_super_sample_depth_buf.find(ind) != m_super_sample_depth_buf.end()) {
        auto &ss_depth_buf = m_super_sample_depth_buf[ind];
        auto &ss_color_buf = m_super_sample_color_buf[ind];
        for (int ss_idx = 0; ss_idx < super_sample_times; ++ss_idx) {
            Eigen::Vector3f choosed_color;
            if (ss_depth_buf[ss_idx] > pix_depth)
                choosed_color = pix_color;
            else
                choosed_color = ss_color_buf[ss_idx];

            r += w * choosed_color.x();
            g += w * choosed_color.y();
            b += w * choosed_color.z();
        }
    }
    else {
        r = pix_color.x();
        g = pix_color.y();
        b = pix_color.z();
    }
    Eigen::Vector3f color = {r, g, b};
    set_pixel(ix, iy, color);
}
    
void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    m_model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    m_view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    m_projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(m_frame_buf.begin(), m_frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        std::fill(m_pixel_buf.begin(), m_pixel_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(m_depth_buf.begin(), m_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : m_width(w), m_height(h)
{
    m_frame_buf.resize(w * h);
    m_pixel_buf.resize(w * h);
    m_depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (m_height-1-y)*m_width + x;
}

void rst::rasterizer::set_pixel(int x, int y, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (m_height-1-y) * m_width + x;
    m_frame_buf[ind] = color;
}

// clang-format on