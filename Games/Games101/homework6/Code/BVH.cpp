#include <algorithm>
#include <cassert>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    if (splitMethod == SplitMethod::SAH)
        root = recursiveSAHBuild(primitives);
    else
        root = recursiveBuild(primitives);

    time(&stop);
    double diff = difftime(stop, start);

    printf("\rBVH Generation complete: \nTime Taken: %f secs\n\n", diff);
}

BVHBuildNode* BVHAccel::recursiveSAHBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();
    if (objects.size() == 1) {
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveSAHBuild(std::vector{objects[0]});
        node->right = recursiveSAHBuild(std::vector{objects[1]});
        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds = Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        int divisions = std::min(BVH_SAH_N, (int)objects.size());
        double min_cost = std::numeric_limits<double>::max();
        size_t min_i = 1;
        std::vector<Bounds3> forward_bounds;
        std::vector<Bounds3> backward_bounds;

        float bucket_len = objects.size()/(float)divisions + 1e-5;
        size_t forward_max_end = bucket_len * (divisions - 1);

        for (size_t i = 1; i < divisions; ++i)
        {
            Bounds3 last_forward_bounds, last_backward_bounds;
            if (!forward_bounds.empty())
                last_forward_bounds = forward_bounds.back();
            if (!backward_bounds.empty())
                last_backward_bounds = backward_bounds.back();

            size_t forward_start = (i-1) * bucket_len;
            size_t forward_end = std::min((size_t)(i * bucket_len), forward_max_end);
            size_t backward_start = std::min((size_t)((divisions - i) * bucket_len), forward_max_end);
            size_t backward_end = std::min((size_t)((divisions - i + 1) * bucket_len), objects.size());

            Bounds3 cur_forward_bounds = last_forward_bounds;
            for (size_t j = forward_start; j < forward_end; ++j)
                cur_forward_bounds = Union(cur_forward_bounds, objects[j]->getBounds());
            
            Bounds3 cur_backward_bounds = last_backward_bounds;
            for (size_t j = backward_start; j < backward_end; ++j)
                cur_backward_bounds = Union(cur_backward_bounds, objects[j]->getBounds());

            forward_bounds.push_back(cur_forward_bounds);
            backward_bounds.push_back(cur_backward_bounds);
        }

        Bounds3 union_bounds = Union(forward_bounds[0], backward_bounds.back());
        double union_area = union_bounds.SurfaceArea();

        for (size_t i = 1; i < divisions; ++i) {
            size_t left_n = std::min((size_t)(i * bucket_len), forward_max_end);
            size_t right_n = objects.size() - left_n;
            double cost = left_n * forward_bounds[i-1].SurfaceArea()/union_area + right_n * backward_bounds[divisions-i-1].SurfaceArea()/union_area;
            if (cost < min_cost)
            {
                min_cost = cost;
                min_i = i;
            }
        }

        auto beginning = objects.begin();
        int offset = std::min((size_t)(min_i * bucket_len), forward_max_end);
        auto middling = objects.begin() + offset;
        auto ending = objects.end();
        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);
        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        BVHBuildNode* left = recursiveSAHBuild(leftshapes);
        BVHBuildNode* right = recursiveSAHBuild(rightshapes);
        node->left = left;
        node->right = right;
        node->bounds = Union(left->bounds, right->bounds);

        return node;
    }

}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    //Bounds3 bounds;
    //for (int i = 0; i < objects.size(); ++i)
    //    bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        node->left = recursiveBuild(leftshapes);
        node->right = recursiveBuild(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    // TODO Traverse the BVH to find intersection
    Intersection ret;
    if (!node->bounds.IntersectP(ray))
        return ret;
    
    // is leaf node, stop recursive
    if (node->object)
        return node->object->getIntersection(ray);

    Intersection left, right;

    if (node->left)
        left = getIntersection(node->left, ray);
    
    if (node->right)
        right = getIntersection(node->right, ray);

    return left.distance <= right.distance ? left : right;
}