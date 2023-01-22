//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    auto split_method = BVHAccel::SplitMethod::NAIVE;
    if (BVH_SAH_N > 0)
        split_method = BVHAccel::SplitMethod::SAH;
    this->bvh = new BVHAccel(objects, 1, split_method);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    Intersection intersection = bvh->Intersect(ray);
    if (!intersection.happened){
        return this->backgroundColor;
    }
    return shade(ray, intersection, depth);
}

Vector3f Scene::shade(const Ray &ray, const Intersection& hitIntersection, int depth) const
{
    // 是光源
    if (hitIntersection.m->hasEmission())
        return hitIntersection.m->Kd;

    Vector3f hitPoint = hitIntersection.coords;

    // 直接光的贡献
    Vector3f L_dir(0);

    // 采样一根光线
    Intersection lightIntersection;
    float pdfLight = 1.0f;
    sampleLight(lightIntersection, pdfLight);
    lightIntersection.coords += lightIntersection.normal * EPSILON;
    Vector3f wi = hitPoint - lightIntersection.coords;
    Vector3f f_r = hitIntersection.m->eval(wi, -ray.direction, hitIntersection.normal);
    // 检查光线与hitPoint之间有无遮挡
    Ray sampleLightRay(lightIntersection.coords, wi.normalized());
    Intersection sampleLightHitInter = bvh->Intersect(sampleLightRay);
    Vector3f delta = sampleLightHitInter.coords - hitPoint;

    float wiDotPosNormal = dotProduct(wi, lightIntersection.normal);
    float wiDotHitNormal = dotProduct(wi, hitIntersection.normal);

    // 光线与点之间无遮挡, 且在点的正半球
    if (wiDotPosNormal > 0 && wiDotHitNormal < 0 && sampleLightHitInter.happened && abs(delta.x) < 1e-2 && abs(delta.y) < 1e-2 && abs(delta.z) < 1e-2)
    {
        float dist = wi.norm();
        float cos_theta = -wiDotHitNormal/dist;
        float cos_theta2 = wiDotPosNormal/dist;
        L_dir = f_r * (lightIntersection.emit * cos_theta * cos_theta2 / (dist * dist * pdfLight));
        //printf("f_r: (%.2f, %.2f, %.2f), emit: (%.2f, %.2f, %.2f), cos_theta: %.2f, cos_theta2: %.2f, L_dir:(%.5f, %.5f, %.5f), dist: %.1f, depth: %d\n",
        //    f_r.x, f_r.y, f_r.z, lightIntersection.emit.x, lightIntersection.emit.y, lightIntersection.emit.z,
        //    cos_theta, cos_theta2, L_dir.x, L_dir.y, L_dir.z, dist, depth);
    }

    Vector3f L_indir(0);

    if (depth >= 2 || get_random_float() > PRR)
        return L_dir;
    
    Vector3f w_indirect;
    w_indirect = hitIntersection.m->sample(w_indirect, hitIntersection.normal);
    Ray indirectRay(hitPoint + hitIntersection.normal * EPSILON, w_indirect);
    Intersection indirectHitInter = bvh->Intersect(indirectRay);

    // 射线打空或者打到了光源
    if (!indirectHitInter.happened || indirectHitInter.m->hasEmission())
        return L_dir;

    auto indirectLightColor = shade(indirectRay, indirectHitInter, depth+1);
    float cos_theta3 = dotProduct(w_indirect, hitIntersection.normal);

    if (!indirectHitInter.happened || cos_theta3 <= 0)
        return L_dir;
    
    L_indir =  (indirectLightColor * cos_theta3 / (2 * M_PI * PRR));

    //printf("f_r: (%.2f, %.2f, %.2f), cos_theta3: %.2f, L_indir:(%.5f, %.5f, %.5f), depth: %d\n",
    //        f_r.x, f_r.y, f_r.z, cos_theta3, L_indir.x, L_indir.y, L_indir.z, depth);

    return L_dir + L_indir;
}