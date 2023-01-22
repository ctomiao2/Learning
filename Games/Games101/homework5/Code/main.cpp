#include "Scene.hpp"
#include "Sphere.hpp"
#include "Triangle.hpp"
#include "Light.hpp"
#include "Renderer.hpp"

// In the main function of the program, we create the scene (create objects and lights)
// as well as set the options for the render (image width and height, maximum recursion
// depth, field-of-view, etc.). We then call the render function().
int main()
{
    Scene scene(1280, 960);

    auto sph1 = std::make_unique<Sphere>(Vector3f(-1, 0, -12), 2);
    sph1->materialType = DIFFUSE_AND_GLOSSY;
    sph1->diffuseColor = Vector3f(1.0, 0, 0);
    //sph1->materialType = REFLECTION;

    auto sph2 = std::make_unique<Sphere>(Vector3f(0.5, -0.5, -8), 1.5);
    sph2->ior = 1.5;
    sph2->id = 1;
    sph2->materialType = REFLECTION_AND_REFRACTION;
    sph2->diffuseColor = Vector3f(0, 1, 0);
    //sph2->materialType = REFLECTION;

    scene.Add(std::move(sph1));
    scene.Add(std::move(sph2));

    Vector3f verts[4] = {{-5,-3,-6}, {5,-3,-6}, {5,-3,-16}, {-5,-3,-16}};
    uint32_t vertIndex[6] = {0, 1, 3, 1, 2, 3};
    Vector2f st[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    auto mesh = std::make_unique<MeshTriangle>(verts, vertIndex, 2, st, 5);
    mesh->id = 2;
    mesh->materialType = DIFFUSE_AND_GLOSSY;

    scene.Add(std::move(mesh));
    
    Vector3f ceilVerts[4] = {{-5,7,-6}, {5,7,-6}, {5,7,-16}, {-5,7,-16}};
    uint32_t ceilVertIndex[6] = {0, 3, 1, 1, 3, 2};
    Vector2f ceilSt[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    auto ceilMesh = std::make_unique<MeshTriangle>(ceilVerts, ceilVertIndex, 2, ceilSt, 5);
    ceilMesh->id = 3;
    ceilMesh->materialType = DIFFUSE_AND_GLOSSY;
    //scene.Add(std::move(ceilMesh));
    
    scene.Add(std::make_unique<Light>(Vector3f(-20, 70, 20), 0.5));
    scene.Add(std::make_unique<Light>(Vector3f(30, 50, -12), 0.5)); 
    scene.Add(std::make_unique<Light>(Vector3f(0, 5, -12), 1));  

    Renderer r;
    r.Render(scene);

    return 0;
}