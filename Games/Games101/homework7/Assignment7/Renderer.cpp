//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <cstring>
#include "Scene.hpp"
#include "Renderer.hpp"
//#include <pthread.h>
#include "JobSystem.hpp"

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

struct ThreadArgs {
    ThreadArgs(Renderer* _render, Scene* _scene, Ray* _ray, int _pixel, Vector3f* _fb)
    : render(_render), scene(_scene), ray(_ray), pixel(_pixel), framebuffer(_fb) {}

    Scene *scene;
    Ray *ray;
    int pixel;
    Vector3f *framebuffer;
    Renderer *render;
};

void* threadCastRay(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs*)args;
    Vector3f *ret = new Vector3f(0);
    for (int k = 0; k < SPP; k++){
        *ret += threadArgs->scene->castRay(*threadArgs->ray, 0) / SPP;  
    }
    return ret;
}

bool flags[1000*1000];

void threadCastRayCallback(void* result, void* job) {
    Vector3f* ret = (Vector3f*)result;
    Job* pJob = (Job*)job;
    ThreadArgs* threadArgs = (ThreadArgs*)(pJob->data);
    Renderer *render = threadArgs->render;
    if (flags[threadArgs->pixel])
        printf("[Error]dumplicate pixel %d\n", threadArgs->pixel);
    flags[threadArgs->pixel] = true;
    threadArgs->framebuffer[threadArgs->pixel] = *ret;
    uint64_t cnt = threadArgs->render->counter.fetch_add(1);
    if (cnt % 10 == 0) {
        uint64_t total = threadArgs->scene->width;
        total *= threadArgs->scene->height;
        UpdateProgress(cnt * 1.0f/total);
    }

    delete threadArgs->ray;
    delete ret;
    delete threadArgs;
    delete pJob;
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    memset(flags, 0, 1000*1000);
    Vector3f *framebuffer = new Vector3f[scene.width * scene.height];

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    int m = 0;
    Vector3f eye_pos(278, 273, -800);

    std::cout << "SPP: " << SPP << "\n";

    JobSystem jobSystem;

    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                      imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

            Vector3f dir = normalize(Vector3f(-x, y, 1));

            if (!USE_MULTI_THREAD)
            {
                for (int k = 0; k < SPP; k++)
                    framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / SPP;
                ++m;
            }
            else
            {
                ThreadArgs *threadData = new ThreadArgs(this, (Scene*)&scene, new Ray(eye_pos, dir), m++, framebuffer);
                jobSystem.createJob(threadCastRay, threadCastRayCallback, (void*)threadData);
            }
        }
    }

    uint64_t total = scene.height;
    total *= scene.width;
    uint64_t curCounter = 0;
    while ((curCounter = counter.load(std::memory_order_acquire)) != total) {
        UpdateProgress(curCounter * 1.0f/total);
    }
    UpdateProgress(1.f);

    jobSystem.exit();

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }

    delete []framebuffer;

    fclose(fp);    
}
