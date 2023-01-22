//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_normals(const std::vector<Eigen::Vector3f>& normals)
{
    auto id = get_next_id();
    nor_buf.emplace(id, normals);

    normal_id = id;

    return {id};
}


// Bresenham's line drawing algorithm
void rst::rasterizer::draw_line(Eigen::Vector3f begin, Eigen::Vector3f end)
{
    auto x1 = begin.x();
    auto y1 = begin.y();
    auto x2 = end.x();
    auto y2 = end.y();

    Eigen::Vector3f line_color = {255, 255, 255};

    int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;

    dx=x2-x1;
    dy=y2-y1;
    dx1=fabs(dx);
    dy1=fabs(dy);
    px=2*dy1-dx1;
    py=2*dx1-dy1;

    if(dy1<=dx1)
    {
        if(dx>=0)
        {
            x=x1;
            y=y1;
            xe=x2;
        }
        else
        {
            x=x2;
            y=y2;
            xe=x1;
        }
        Eigen::Vector2i point = Eigen::Vector2i(x, y);
        set_pixel(point,line_color);
        for(i=0;x<xe;i++)
        {
            x=x+1;
            if(px<0)
            {
                px=px+2*dy1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0))
                {
                    y=y+1;
                }
                else
                {
                    y=y-1;
                }
                px=px+2*(dy1-dx1);
            }
//            delay(0);
            Eigen::Vector2i point = Eigen::Vector2i(x, y);
            set_pixel(point,line_color);
        }
    }
    else
    {
        if(dy>=0)
        {
            x=x1;
            y=y1;
            ye=y2;
        }
        else
        {
            x=x2;
            y=y2;
            ye=y1;
        }
        Eigen::Vector2i point = Eigen::Vector2i(x, y);
        set_pixel(point,line_color);
        for(i=0;y<ye;i++)
        {
            y=y+1;
            if(py<=0)
            {
                py=py+2*dx1;
            }
            else
            {
                if((dx<0 && dy<0) || (dx>0 && dy>0))
                {
                    x=x+1;
                }
                else
                {
                    x=x-1;
                }
                py=py+2*(dx1-dy1);
            }
//            delay(0);
            Eigen::Vector2i point = Eigen::Vector2i(x, y);
            set_pixel(point,line_color);
        }
    }
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

static bool insideTriangle(int x, int y, const Vector4f* _v){
    Vector3f v[3];
    for(int i=0;i<3;i++)
        v[i] = {_v[i].x(),_v[i].y(), 1.0};
    Vector3f f0,f1,f2;
    f0 = v[1].cross(v[0]);
    f1 = v[2].cross(v[1]);
    f2 = v[0].cross(v[2]);
    Vector3f p(x,y,1.);
    if((p.dot(f0)*f0.dot(v[2])>0) && (p.dot(f1)*f1.dot(v[0])>0) && (p.dot(f2)*f2.dot(v[1])>0))
        return true;
    return false;
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector4f* v){
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(std::vector<Triangle *> &TriangleList) {

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    //Eigen::Matrix4f mvp = projection * view * model;
    for (const auto& t:TriangleList)
    {
        Triangle newtri = *t;

        Eigen::Matrix4f inv_trans = (view * model).inverse().transpose();
        Eigen::Vector4f n[] = {
                inv_trans * to_vec4(t->normal[0], 0.0f),
                inv_trans * to_vec4(t->normal[1], 0.0f),
                inv_trans * to_vec4(t->normal[2], 0.0f)
        };

        vertex_shader_payload payload_array[] = {
            vertex_shader_payload(t->v[0], n[0].head<3>(), t->tex_coords[0], model, view, texture ? &*texture : nullptr),
            vertex_shader_payload(t->v[1], n[1].head<3>(), t->tex_coords[1], model, view, texture ? &*texture : nullptr),
            vertex_shader_payload(t->v[2], n[2].head<3>(), t->tex_coords[2], model, view, texture ? &*texture : nullptr)
        };

        std::array<Eigen::Vector4f, 3> vertex_shader_output {
            vertex_shader(payload_array[0]),
            vertex_shader(payload_array[1]),
            vertex_shader(payload_array[2])
        };

        std::array<Eigen::Vector4f, 3> mm {
            vertex_shader_output[0],
            vertex_shader_output[1],
            vertex_shader_output[2]
        };

        std::array<Eigen::Vector3f, 3> viewspace_pos;

        std::transform(mm.begin(), mm.end(), viewspace_pos.begin(), [](auto& v) {
            return v.template head<3>();
        });

        Eigen::Vector4f v[] = {
            projection * mm[0],
            projection * mm[1],
            projection * mm[2]
        };

        //Homogeneous division
        for (auto& vec : v) {
            vec.x()/=vec.w();
            vec.y()/=vec.w();
            vec.z()/=vec.w();
        }
        /*
        std::cout << v[0].x() << ' ' << v[0].y() << ' ' << v[0].z() << ' ' << v[0].w() << '\n';
        std::cout << v[1].x() << ' ' << v[1].y() << ' ' << v[1].z() << ' ' << v[0].w() << '\n';
        std::cout << v[2].x() << ' ' << v[2].y() << ' ' << v[2].z() << ' ' << v[0].w() << '\n';
        */
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = std::max(0.0, std::min((double)width-1, 0.5*width*(vert.x()+1.0)));
            vert.y() = std::max(0.0, std::min((double)height-1, 0.5*height*(vert.y()+1.0)));
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            //screen space coordinates
            newtri.setVertex(i, v[i]);
        }

        for (int i = 0; i < 3; ++i)
        {
            //view space normal
            newtri.setNormal(i, n[i].head<3>());
        }

        newtri.setColor(0, 148,121.0,92.0);
        newtri.setColor(1, 148,121.0,92.0);
        newtri.setColor(2, 148,121.0,92.0);

        // Also pass view space vertice position
        rasterize_triangle(newtri, viewspace_pos, t);
    }
}

static Eigen::Vector3f interpolate(float alpha, float beta, float gamma, const Eigen::Vector3f& vert1, const Eigen::Vector3f& vert2, const Eigen::Vector3f& vert3, float weight)
{
    return (alpha * vert1 + beta * vert2 + gamma * vert3) / weight;
}

static Eigen::Vector2f interpolate(float alpha, float beta, float gamma, const Eigen::Vector2f& vert1, const Eigen::Vector2f& vert2, const Eigen::Vector2f& vert3, float weight)
{
    auto u = (alpha * vert1[0] + beta * vert2[0] + gamma * vert3[0]);
    auto v = (alpha * vert1[1] + beta * vert2[1] + gamma * vert3[1]);

    u /= weight;
    v /= weight;

    return Eigen::Vector2f(u, v);
}

static Eigen::Vector3f shadow_map_depth_2_color(float depth)
{
    int d = depth * 1000;
    int r = d/(255*255);
    int g = (d - r*255*255)/255;
    int b = d % 255;
    return Eigen::Vector3f(r, g, b);
}

static float shadow_map_color_2_depth(const Eigen::Vector3f &color)
{
    int depth = 255*255*color[0] + 255*color[1] + color[2];
    return depth/1000.0f;
}

bool rst::rasterizer::is_shadow_pixel(const Triangle* model_triangle, float alpha, float beta, float gamma)
{
    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Vector4f v[] = {
        shadow_map_mvp * model_triangle->v[0],
        shadow_map_mvp * model_triangle->v[1],
        shadow_map_mvp * model_triangle->v[2]
    };

    //Homogeneous division
    for (auto& vec : v) {
        vec /= vec.w();
    }
    
    //Viewport transformation
    for (auto & vert : v)
    {
        vert.x() = std::max(0.0, std::min((double)width-1, 0.5*width*(vert.x()+1.0)));
        vert.y() = std::max(0.0, std::min((double)height-1, 0.5*height*(vert.y()+1.0)));
        vert.z() = vert.z() * f1 + f2;
    }

    float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    z_interpolated *= w_reciprocal;

    Eigen::Vector2f interp_tex_coord = interpolate(alpha, beta, gamma,
        model_triangle->tex_coords[0], model_triangle->tex_coords[1], model_triangle->tex_coords[2], 1.0);
    
    Eigen::Vector3f v0 = { v[0][0], v[0][1], v[0][2] };
    Eigen::Vector3f v1 = { v[1][0], v[1][1], v[1][2] };
    Eigen::Vector3f v2 = { v[2][0], v[2][1], v[2][2] };
    Eigen::Vector3f pos_interpolated = interpolate(alpha, beta, gamma, v0, v1, v2, 1.0);
    float u_tex = pos_interpolated.x()/(width-1), v_tex = pos_interpolated.y()/(height-1);

    Eigen::Vector3f shadow_depth_color = texture2->getColor(u_tex, v_tex);
    float shadow_depth = shadow_map_color_2_depth(shadow_depth_color);

    if (z_interpolated >= shadow_depth + 1e-2) {
        std::cout << model_triangle->v[0][0] << ' ' << model_triangle->v[0][1] << ' ' << model_triangle->v[0][2] << '\n';
        std::cout << model_triangle->v[1][0] << ' ' << model_triangle->v[1][1] << ' ' << model_triangle->v[1][2] << '\n';
        std::cout << model_triangle->v[2][0] << ' ' << model_triangle->v[2][1] << ' ' << model_triangle->v[2][2] << '\n';
        std::cout << u_tex << ' ' << v_tex << ' ' << z_interpolated << ' ' << shadow_depth << '\n';
    }
    return z_interpolated >= shadow_depth + 1e-2;
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t, const std::array<Eigen::Vector3f, 3>& view_pos, const Triangle* model_triangle) 
{
    auto v = t.toVector4();
    
    // TODO : Find out the bounding box of current triangle.
    float min_x = std::min(t.v[2].x(), std::min(t.v[0].x(), t.v[1].x()));
    float max_x = std::max(t.v[2].x(), std::max(t.v[0].x(), t.v[1].x()));
    float min_y = std::min(t.v[2].y(), std::min(t.v[0].y(), t.v[1].y()));
    float max_y = std::max(t.v[2].y(), std::max(t.v[0].y(), t.v[1].y()));
   
    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            if (!insideTriangle(x, y, t.v))
                continue;

            auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);

            float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            z_interpolated *= w_reciprocal;
            int ind = (height-1-y) * width + x;

            //std::cout << x << '\t' << y << '\t' << width << '\t' << height << '\n';
            // 该像素任意一个采样点深度都不大于depth_buf[ind], 如果插值得到的深度比这个还要大那就不要再比了
            if (z_interpolated > depth_buf[ind])
                continue;

            depth_buf[ind] = z_interpolated;

            // 是否在阴影
            if (recive_shadow && is_shadow_pixel(model_triangle, alpha, beta, gamma))
            {
                Vector2i point = { x, y };
                set_pixel(point, Eigen::Vector3f(32, 32, 32));
                continue;
            }

            Eigen::Vector3f pixel_color;

            if (draw_depth_only)
            {
                pixel_color = shadow_map_depth_2_color(z_interpolated);
            }
            else
            {
                Eigen::Vector3f interp_color = interpolate(alpha, beta, gamma, t.color[0], t.color[1], t.color[2], 1.0);
                Eigen::Vector3f interp_normal = interpolate(alpha, beta, gamma, t.normal[0], t.normal[1], t.normal[2], 1.0);
                interp_normal = interp_normal.normalized();
                Eigen::Vector2f interp_tex_coord = interpolate(alpha, beta, gamma, t.tex_coords[0], t.tex_coords[1], t.tex_coords[2], 1.0);
                Eigen::Vector3f interpolated_shadingcoords = interpolate(alpha, beta, gamma, view_pos[0], view_pos[1], view_pos[2], 1.0);
                fragment_shader_payload payload( interp_color, interp_normal, interp_tex_coord, texture ? &*texture : nullptr, texture2 ? &*texture2 : nullptr);
                payload.view_pos = interpolated_shadingcoords;
                pixel_color = fragment_shader(payload);
            }

            Vector2i point = { x, y };
            set_pixel(point, pixel_color);
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

void rst::rasterizer::fill_frame_buf(const Eigen::Vector3f &color)
{
    std::fill(frame_buf.begin(), frame_buf.end(), color);
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);

    texture = std::nullopt;
    texture2 = std::nullopt;
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-y)*width + x;
}

void rst::rasterizer::set_pixel(const Vector2i &point, const Eigen::Vector3f &color)
{
    //old index: auto ind = point.y() + point.x() * width;
    int ind = (height-point.y())*width + point.x();
    frame_buf[ind] = color;
}

void rst::rasterizer::set_vertex_shader(std::function<Eigen::Vector4f(vertex_shader_payload)> vert_shader)
{
    vertex_shader = vert_shader;
}

void rst::rasterizer::set_fragment_shader(std::function<Eigen::Vector3f(fragment_shader_payload)> frag_shader)
{
    fragment_shader = frag_shader;
}

