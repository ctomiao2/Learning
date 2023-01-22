//
// Created by goksu on 2/25/20.
//
#include "Scene.hpp"
#include <atomic>

#pragma once
struct hit_payload
{
    float tNear;
    uint32_t index;
    Vector2f uv;
    Object* hit_obj;
};

class Renderer
{
public:
    void Render(const Scene& scene);

public:
    std::atomic<uint64_t> counter = {0};

private:
};
