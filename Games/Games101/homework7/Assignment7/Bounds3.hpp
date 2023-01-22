//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_BOUNDS3_H
#define RAYTRACING_BOUNDS3_H
#include "Ray.hpp"
#include "Vector.hpp"
#include <limits>
#include <array>

class Bounds3
{
  public:
    Vector3f pMin, pMax; // two points to specify the bounding box
    Bounds3()
    {
        double minNum = std::numeric_limits<double>::lowest();
        double maxNum = std::numeric_limits<double>::max();
        pMax = Vector3f(minNum, minNum, minNum);
        pMin = Vector3f(maxNum, maxNum, maxNum);
    }
    Bounds3(const Vector3f p) : pMin(p), pMax(p) {}
    Bounds3(const Vector3f p1, const Vector3f p2)
    {
        pMin = Vector3f(fmin(p1.x, p2.x), fmin(p1.y, p2.y), fmin(p1.z, p2.z));
        pMax = Vector3f(fmax(p1.x, p2.x), fmax(p1.y, p2.y), fmax(p1.z, p2.z));
    }

    Vector3f Diagonal() const { return pMax - pMin; }
    int maxExtent() const
    {
        Vector3f d = Diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        else if (d.y > d.z)
            return 1;
        else
            return 2;
    }

    double SurfaceArea() const
    {
        Vector3f d = Diagonal();
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

    Vector3f Centroid() { return 0.5 * pMin + 0.5 * pMax; }
    Bounds3 Intersect(const Bounds3& b)
    {
        return Bounds3(Vector3f(fmax(pMin.x, b.pMin.x), fmax(pMin.y, b.pMin.y),
                                fmax(pMin.z, b.pMin.z)),
                       Vector3f(fmin(pMax.x, b.pMax.x), fmin(pMax.y, b.pMax.y),
                                fmin(pMax.z, b.pMax.z)));
    }

    Vector3f Offset(const Vector3f& p) const
    {
        Vector3f o = p - pMin;
        if (pMax.x > pMin.x)
            o.x /= pMax.x - pMin.x;
        if (pMax.y > pMin.y)
            o.y /= pMax.y - pMin.y;
        if (pMax.z > pMin.z)
            o.z /= pMax.z - pMin.z;
        return o;
    }

    bool Overlaps(const Bounds3& b1, const Bounds3& b2)
    {
        bool x = (b1.pMax.x >= b2.pMin.x) && (b1.pMin.x <= b2.pMax.x);
        bool y = (b1.pMax.y >= b2.pMin.y) && (b1.pMin.y <= b2.pMax.y);
        bool z = (b1.pMax.z >= b2.pMin.z) && (b1.pMin.z <= b2.pMax.z);
        return (x && y && z);
    }

    bool Inside(const Vector3f& p, const Bounds3& b)
    {
        return (p.x >= b.pMin.x && p.x <= b.pMax.x && p.y >= b.pMin.y &&
                p.y <= b.pMax.y && p.z >= b.pMin.z && p.z <= b.pMax.z);
    }
    inline const Vector3f& operator[](int i) const
    {
        return (i == 0) ? pMin : pMax;
    }

    inline bool IntersectP(const Ray& ray) const;
};



inline bool Bounds3::IntersectP(const Ray& ray) const
{
    // invDir: ray direction(x,y,z), invDir=(1.0/x,1.0/y,1.0/z), use this because Multiply is faster that Division
    // dirIsNeg: ray direction(x,y,z), dirIsNeg=[int(x>0),int(y>0),int(z>0)], use this to simplify your logic
    // TODO test if ray bound intersects
    // x-slab: t1 = (-O.z + zmin)/R.z   t2 = (-O.z + zmax)/R.z
    float t1 = std::numeric_limits<float>::min(), t2 = std::numeric_limits<float>::max();
    if (ray.direction.z == 0 && (ray.origin.z < pMin.z || ray.origin.z > pMax.z))
        return false;
    // y-slab: t3 = (-O.x + xmin)/R.x   t4 = (-O.x + xmax)/R.x
    float t3 = std::numeric_limits<float>::min(), t4 = std::numeric_limits<float>::max();
    if (ray.direction.x == 0 && (ray.origin.x < pMin.x || ray.origin.x > pMax.x))
        return false;
    // z-slab: t5 = (-O.y + ymin)/R.y   t6 = (-O.y + ymax)/R.y
    float t5 = std::numeric_limits<float>::min(), t6 = std::numeric_limits<float>::max();
    if (ray.direction.y == 0 && (ray.origin.y < pMin.y || ray.origin.y > pMax.y))
        return false;

    t1 = (-ray.origin.z + pMin.z)/ray.direction.z;
    t2 = (-ray.origin.z + pMax.z)/ray.direction.z;
    if (t1 < 0 && t2 < 0)
        return false;
    t3 = (-ray.origin.x + pMin.x)/ray.direction.x;
    t4 = (-ray.origin.x + pMax.x)/ray.direction.x;
    if (t3 < 0 && t4 < 0)
        return false;
    t5 = (-ray.origin.y + pMin.y)/ray.direction.y;
    t6 = (-ray.origin.y + pMax.y)/ray.direction.y;
    if (t5 < 0 && t6 < 0)
        return false;
    
    float min_t12 = t1 <= t2 ? t1 : t2, max_t12 = t1 >= t2 ? t1 : t2;
    float min_t34 = t3 <= t4 ? t3 : t4, max_t34 = t3 >= t4 ? t3 : t4;
    float min_t56 = t5 <= t6 ? t5 : t6, max_t56 = t5 >= t6 ? t5 : t6;
    float min_max_t = (max_t12 <= max_t34 && max_t12 <= max_t56) ? max_t12 : (max_t34 <= max_t56 ? max_t34 : max_t56);
    float max_min_t = (min_t12 >= min_t34 && min_t12 >= min_t56) ? min_t12 : (min_t34 >= min_t56 ? min_t34 : min_t56);

    return min_max_t >= max_min_t;
}

inline Bounds3 Union(const Bounds3& b1, const Bounds3& b2)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b1.pMin, b2.pMin);
    ret.pMax = Vector3f::Max(b1.pMax, b2.pMax);
    return ret;
}

inline Bounds3 Union(const Bounds3& b, const Vector3f& p)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b.pMin, p);
    ret.pMax = Vector3f::Max(b.pMax, p);
    return ret;
}

#endif // RAYTRACING_BOUNDS3_H
