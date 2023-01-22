#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

int BVH_SAH_N = 0;
// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{   
    if (argc > 1)
        BVH_SAH_N = atoi(argv[1]);

    Scene scene(1280, 960);

    MeshTriangle bunny("../models/bunny/bunny.obj");

    scene.Add(&bunny);
    scene.Add(std::make_unique<Light>(Vector3f(-20, 70, 20), 1));
    scene.Add(std::make_unique<Light>(Vector3f(20, 70, 20), 1));
    scene.buildBVH();

    Renderer r;

    auto start = std::chrono::system_clock::now();
    r.Render(scene);
    auto stop = std::chrono::system_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()/1000000.0f;
    std::cout << "Render complete: \n";
    if (BVH_SAH_N > 0)
        std::cout << "SAH_" << BVH_SAH_N << " Time taken: " << elapsed_time << " seconds\n";
    else
        std::cout << "Time taken: " << elapsed_time << " seconds\n";

    return 0;
}