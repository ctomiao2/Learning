#include <iostream>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
    float fov_rad = MY_PI * eye_fov / 180;
    float height = 2 * tan(0.5*fov_rad) * zNear;
    float width = height * aspect_ratio;
    projection << 2*zNear/width, 0, 0, 0,
        0, 2*zNear/height, 0, 0,
        0, 0, 2*(zNear + zFar)/(zNear - zFar), 2*zNear*zFar/(zNear - zFar),
        0, 0, -1, 0;
    
    return projection;
}

Eigen::Vector4f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.view * payload.model * payload.position;
}

Eigen::Vector3f frag_shader(const fragment_shader_payload& payload)
{
    return payload.color * 255.0f;
}

Eigen::Vector4f displacement_vertex_shader(const vertex_shader_payload& payload)
{
    float u = payload.tex_coords.x(), v = payload.tex_coords.y();
    float uv_h = payload.texture->getColor(u, v).norm()/(255.0 * std::sqrt(3.0));
    Eigen::Vector3f normal = payload.normal.normalized();
    Eigen::Matrix4f displacement;
    displacement << 1.0, 0.0, 0.0, 0.1*uv_h*normal[0],
                    0.0, 1.0, 0.0, 0.1*uv_h*normal[1],
                    0.0, 0.0, 1.0, 0.1*uv_h*normal[2],
                    0.0, 0.0, 0.0, 1.0;
    std::cout << 0.1*uv_h*normal[0] << ' ' << 0.1*uv_h*normal[1] << ' ' << 0.1*uv_h*normal[0] << ' ' << uv_h << '\n';
    std::cout << payload.position[0] << ' ' << payload.position[1] << ' ' << payload.position[2] << '\n';
    return displacement * payload.view * payload.model * payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        return_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{-4,4,20}, {500, 500, 500}};
    //auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;
    normal = normal.normalized();

    Eigen::Vector3f result_color = {0, 0, 0};

    Eigen::Vector3f ambient_color = Eigen::Vector3f(ka.x() * amb_light_intensity.x(), ka.y() * amb_light_intensity.y(), ka.z() * amb_light_intensity.z());
    Eigen::Vector3f diffuse_color = {0, 0, 0};
    Eigen::Vector3f specular_color = {0, 0, 0};
    Eigen::Vector3f vEye = eye_pos - payload.view_pos;
    float vEyeMagnitude = std::sqrt(vEye.x() * vEye.x() + vEye.y() * vEye.y() + vEye.z() * vEye.z());

    for (auto& light : lights)
    {
        Eigen::Vector3f r = light.position - payload.view_pos;
        float squre_r = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
        Eigen::Vector3f v_light = (1.0f/squre_r) * light.intensity;
        float normal_dot_r = std::max(0.0f, (normal.dot(r.normalized())));
        diffuse_color += normal_dot_r * Eigen::Vector3f(kd.x() * v_light.x(), kd.y() * v_light.y(), kd.z() * v_light.z());
        Eigen::Vector3f h = vEye + r;
        h = h.normalized();
        specular_color += std::pow(std::max(0.0f, h.dot(normal)), p) * Eigen::Vector3f(ks.x() * v_light.x(), ks.y() * v_light.y(), ks.z() * v_light.z());
    }

    result_color = ambient_color + diffuse_color + specular_color;
    result_color.x() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.x()));
    result_color.y() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.y()));
    result_color.z() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.z()));

    return result_color;
}

Eigen::Vector3f shadow_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        return_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;
    normal = normal.normalized();

    Eigen::Vector3f result_color = {0, 0, 0};

    Eigen::Vector3f ambient_color = Eigen::Vector3f(ka.x() * amb_light_intensity.x(), ka.y() * amb_light_intensity.y(), ka.z() * amb_light_intensity.z());
    Eigen::Vector3f diffuse_color = {0, 0, 0};
    Eigen::Vector3f specular_color = {0, 0, 0};
    Eigen::Vector3f vEye = eye_pos - payload.view_pos;
    float vEyeMagnitude = std::sqrt(vEye.x() * vEye.x() + vEye.y() * vEye.y() + vEye.z() * vEye.z());

    for (auto& light : lights)
    {
        Eigen::Vector3f r = light.position - payload.view_pos;
        float squre_r = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
        Eigen::Vector3f v_light = (1.0f/squre_r) * light.intensity;
        float normal_dot_r = std::max(0.0f, (normal.dot(r.normalized())));
        diffuse_color += normal_dot_r * Eigen::Vector3f(kd.x() * v_light.x(), kd.y() * v_light.y(), kd.z() * v_light.z());
        Eigen::Vector3f h = vEye + r;
        h = h.normalized();
        specular_color += std::pow(std::max(0.0f, h.dot(normal)), p) * Eigen::Vector3f(ks.x() * v_light.x(), ks.y() * v_light.y(), ks.z() * v_light.z());
    }

    result_color = ambient_color + diffuse_color + specular_color;
    result_color.x() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.x()));
    result_color.y() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.y()));
    result_color.z() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.z()));

    return result_color;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;
    normal = normal.normalized();

    Eigen::Vector3f result_color = {0, 0, 0};
    Eigen::Vector3f ambient_color = Eigen::Vector3f(ka.x() * amb_light_intensity.x(), ka.y() * amb_light_intensity.y(), ka.z() * amb_light_intensity.z());
    Eigen::Vector3f diffuse_color = {0, 0, 0};
    Eigen::Vector3f specular_color = {0, 0, 0};
    Eigen::Vector3f vEye = eye_pos - payload.view_pos;
    float vEyeMagnitude = std::sqrt(vEye.x() * vEye.x() + vEye.y() * vEye.y() + vEye.z() * vEye.z());

    for (auto& light : lights)
    {
        Eigen::Vector3f r = light.position - payload.view_pos;
        float squre_r = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
        Eigen::Vector3f v_light = (1.0f/squre_r) * light.intensity;
        float normal_dot_r = std::max(0.0f, (normal.dot(r.normalized())));
        diffuse_color += normal_dot_r * Eigen::Vector3f(kd.x() * v_light.x(), kd.y() * v_light.y(), kd.z() * v_light.z());
        Eigen::Vector3f h = vEye + r;
        h = h.normalized();
        specular_color += std::pow(std::max(0.0f, h.dot(normal)), p) * Eigen::Vector3f(ks.x() * v_light.x(), ks.y() * v_light.y(), ks.z() * v_light.z());
    }

    result_color = ambient_color + diffuse_color + specular_color;
    result_color.x() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.x()));
    result_color.y() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.y()));
    result_color.z() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.z()));

    return result_color;
}



Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    if (payload.texture2)
    {
        kd = payload.texture2->getColor(payload.tex_coords.x(), payload.tex_coords.y())/255.0f;
    }

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;
    normal.normalize();

    float kh = 0.2, kn = 0.1;
    
    float x = normal.x(), y = normal.y(), z = normal.z();
    Eigen::Vector3f t = Eigen::Vector3f(x*y/std::sqrt(x*x+z*z), -std::sqrt(x*x+z*z), z*y/std::sqrt(x*x+z*z));
    t = t.normalized();
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN << t.x(), b.x(), normal.x(),
           t.y(), b.y(), normal.y(),
           t.z(), b.z(), normal.z();

    if (payload.texture)
    {
        float u = payload.tex_coords.x(), v = payload.tex_coords.y();
        float delta_w = 1.0f/payload.texture->width;
        float delta_h = 1.0f/payload.texture->height;
        float uv_h = payload.texture->getColor(u, v).norm();
        //point += kn * normal * uv_h;
        float dU = kh * kn * (payload.texture->getColor(u + delta_w, v).norm() - uv_h);
        float dV = kh * kn * (payload.texture->getColor(u, v + delta_h).norm() - uv_h);
        Eigen::Vector3f tn = Eigen::Vector3f(-dU, -dV, 1);
        normal = TBN * tn;
        normal.normalize();
    }

    Eigen::Vector3f ambient_color = Eigen::Vector3f(ka.x() * amb_light_intensity.x(), ka.y() * amb_light_intensity.y(), ka.z() * amb_light_intensity.z());
    Eigen::Vector3f diffuse_color = {0, 0, 0};
    Eigen::Vector3f specular_color = {0, 0, 0};
    Eigen::Vector3f vEye = eye_pos - point;
    float vEyeMagnitude = vEye.norm();

    for (auto& light : lights)
    {
        Eigen::Vector3f r = light.position - point;
        float squre_r = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
        Eigen::Vector3f v_light = (1.0f/squre_r) * light.intensity;
        float normal_dot_r = std::max(0.0f, (normal.dot(r.normalized())));
        diffuse_color += normal_dot_r * Eigen::Vector3f(kd.x() * v_light.x(), kd.y() * v_light.y(), kd.z() * v_light.z());
        Eigen::Vector3f h = vEye + r;
        h = h.normalized();
        specular_color += std::pow(std::max(0.0f, h.dot(normal)), p) * Eigen::Vector3f(ks.x() * v_light.x(), ks.y() * v_light.y(), ks.z() * v_light.z());
    }

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = ambient_color + diffuse_color + specular_color;
    result_color.x() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.x()));
    result_color.y() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.y()));
    result_color.z() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.z()));

    return result_color;
}


Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    if (payload.texture2)
    {
        kd = payload.texture2->getLinearColor(payload.tex_coords.x(), payload.tex_coords.y())/255.0f;
    }

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal.normalized();

    float kh = 0.2, kn = 0.1;

    float x = normal.x(), y = normal.y(), z = normal.z();
    Eigen::Vector3f t = Eigen::Vector3f(x*y/std::sqrt(x*x+z*z), -std::sqrt(x*x+z*z), z*y/std::sqrt(x*x+z*z));
    t = t.normalized();
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN << t.x(), b.x(), normal.x(),
           t.y(), b.y(), normal.y(),
           t.z(), b.z(), normal.z();

    if (payload.texture)
    {
        float u = payload.tex_coords.x(), v = payload.tex_coords.y();
        float delta_w = 1.0f/payload.texture->width;
        float delta_h = 1.0f/payload.texture->height;
        float uv_h = payload.texture->getColor(u, v).norm();
        float dU = kh * kn * (payload.texture->getColor(u + delta_w, v).norm() - uv_h);
        float dV = kh * kn * (payload.texture->getColor(u, v + delta_h).norm() - uv_h);
        Eigen::Vector3f tn = Eigen::Vector3f(-dU, -dV, 1);
        normal = TBN * tn;
        normal.normalize();
    }

    Eigen::Vector3f ambient_color = Eigen::Vector3f(ka.x() * amb_light_intensity.x(), ka.y() * amb_light_intensity.y(), ka.z() * amb_light_intensity.z());
    Eigen::Vector3f diffuse_color = {0, 0, 0};
    Eigen::Vector3f specular_color = {0, 0, 0};
    Eigen::Vector3f vEye = eye_pos - payload.view_pos;
    float vEyeMagnitude = vEye.norm();

    for (auto& light : lights)
    {
        Eigen::Vector3f r = light.position - payload.view_pos;
        float squre_r = r.x() * r.x() + r.y() * r.y() + r.z() * r.z();
        Eigen::Vector3f v_light = (1.0f/squre_r) * light.intensity;
        float normal_dot_r = std::max(0.0f, (normal.dot(r.normalized())));
        diffuse_color += normal_dot_r * Eigen::Vector3f(kd.x() * v_light.x(), kd.y() * v_light.y(), kd.z() * v_light.z());
        Eigen::Vector3f h = vEye + r;
        h = h.normalized();
        specular_color += std::pow(std::max(0.0f, h.dot(normal)), p) * Eigen::Vector3f(ks.x() * v_light.x(), ks.y() * v_light.y(), ks.z() * v_light.z());
    }

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = ambient_color + diffuse_color + specular_color;
    result_color.x() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.x()));
    result_color.y() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.y()));
    result_color.z() = std::max(0.0f, std::min(255.0f, 255.0f*result_color.z()));

    return result_color;
}

void draw_floor(rst::rasterizer &r)
{
    std::vector<Eigen::Vector3f> floor_verticles
    {
        {-2, -1, -1},
        {-2, -0.8, 8},
        {6, -1, -1},
        {6, -1, -1},
        {6, -0.8, 8},
        {-2, -0.8, 8}
    };

    std::vector<Triangle*> TriangleList;
    for(int i=0; i < floor_verticles.size(); i+=3)
    {
        Triangle* t = new Triangle();
        for(int j=0; j<3; j++)
        {
            t->setVertex(j, Vector4f(floor_verticles[i+j][0], floor_verticles[i+j][1], floor_verticles[i+j][2], 1.0));
            t->setNormal(j, Vector3f(0, 0, 1.0));
            t->setTexCoord(j, Vector2f(0, 0));
            t->setColor(j, 128, 128, 128);
        }
        TriangleList.push_back(t);
    }

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(frag_shader);
    r.draw(TriangleList);
}

void write_shadow_map(const std::string &filename, rst::rasterizer &r, std::vector<Triangle*> &TriangleList)
{
    r.clear(rst::Buffers::Color | rst::Buffers::Depth);
    //Eigen::Vector3f eye_pos = {-20, 20, 0};
    Eigen::Vector3f eye_pos = {-4,4,20};
    
    r.fill_frame_buf(Eigen::Vector3f(255.0f, 255.0f, 255.0f));

    float angle = 140.0;
    r.set_model(get_model_matrix(angle));
    r.set_view(get_view_matrix(eye_pos));
    r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));
    r.set_draw_depth_only(true);
    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(frag_shader);
    r.draw(TriangleList);
    // 画地板
    draw_floor(r);

    cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
    image.convertTo(image, CV_8UC3, 1.0f);
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

    cv::imwrite(filename, image);
}

int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 140.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "../models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");

    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);

    auto texture_path = "hmap.jpg";
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;
    std::function<Eigen::Vector4f(vertex_shader_payload)> active_vertex_shader = vertex_shader;
    bool support_shadow = false;

    if (argc > 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc == 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            r.set_texture2(Texture(obj_path + "spot_texture.png"));
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the displacement shader\n";
            r.set_texture2(Texture(obj_path + "spot_texture.png"));
            active_shader = displacement_fragment_shader;
            active_vertex_shader = displacement_vertex_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "shadow")
        {
            std::cout << "Rasterizing using the shadow shader\n";
            active_shader = shadow_fragment_shader;
            r.set_texture(Texture(obj_path + "spot_texture.png"));
            r.set_texture2(Texture(obj_path + "shadowmap.png"));
            support_shadow = true;
        }
    }

    Eigen::Vector3f eye_pos = {0,0,10};

    r.set_vertex_shader(active_vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        if (argc == 3 && std::string(argv[2]) == "shadowmap")
        {
            write_shadow_map(filename, r, TriangleList);
        }
        else
        {
            r.clear(rst::Buffers::Color | rst::Buffers::Depth);
            r.set_model(get_model_matrix(angle));
            r.set_view(get_view_matrix(eye_pos));
            auto projection = get_projection_matrix(45.0, 1, 0.1, 50);
            r.set_projection(projection);
            r.draw(TriangleList);
            // 地板接受投影
            if (support_shadow)
            {
                r.set_recive_shadow(true);
                r.set_shadow_map_mvp(projection * get_view_matrix(Eigen::Vector3f(-4,4,20)) * get_model_matrix(140.0));
            }
            // 画地板
            draw_floor(r);

            cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
            image.convertTo(image, CV_8UC3, 1.0f);
            cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

            cv::imwrite(filename, image);
        }
        return 0;
    }

    while(key != 27)
    {
        if (argc == 3 && std::string(argv[1]) == "shadowmap")
        {
            write_shadow_map(filename, r, TriangleList);
            break;
        }
        else
        {
            r.clear(rst::Buffers::Color | rst::Buffers::Depth);

            r.set_model(get_model_matrix(angle));
            r.set_view(get_view_matrix(eye_pos));
            auto projection = get_projection_matrix(45.0, 1, 0.1, 50);
            r.set_projection(projection);

            //r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
            r.draw(TriangleList);

            if (support_shadow)
            {
                r.set_recive_shadow(true);
                r.set_shadow_map_mvp(projection * get_view_matrix(Eigen::Vector3f(-4,4,20)) * get_model_matrix(140.0));
            }

            // 画地板
            draw_floor(r);

            cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
            image.convertTo(image, CV_8UC3, 1.0f);
            cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

            cv::imshow("image", image);
            cv::imwrite(filename, image);
            key = cv::waitKey(10);

            if (key == 'a' )
            {
                angle -= 0.1;
            }
            else if (key == 'd')
            {
                angle += 0.1;
            }
        }

    }
    return 0;
}
