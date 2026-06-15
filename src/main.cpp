#include "camera.h"
#include "sphere.h"
#include "hittable.h"
#include "cpu_renderer.h"
#include "scene_gpu.h"
#include "gpu_renderer.cuh"
#include <chrono>
#include <iostream>


enum mode {
    SIMPLE,
    MULTITHREAD,
    CUDA
};

mode render_mode = CUDA;

void simple_render(camera cam, int image_width, int image_height, int samples_per_pixel,
                    world w, int max_depth) {
    // Generate the image
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int j = 0; j < image_height; j++) {
        std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {   
            colour pixel_colour{0, 0, 0};

            // Random sampling for anti-aliasing
            for (int sample = 0; sample < samples_per_pixel; sample++) {
                double r_u = rand_double() - 0.5;
                double r_v = rand_double() - 0.5;
                vec3 centre = cam.pixel00_loc + (i+r_u)*cam.pixel_delta_u + (j+r_v)*cam.pixel_delta_v;
                ray r{cam.origin, (centre-cam.origin).unit_vector()};
                pixel_colour += ray_colour(r, w, max_depth);
            }
            // Get the average pixel colour from all samples taken
            pixel_colour = pixel_colour / samples_per_pixel;
            write_colour(std::cout, pixel_colour);
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    std::clog << "\rRendered in " << (int) ms << " ms\n";
}

int main() {
    const int samples_per_pixel = 100;
    const int max_depth = 50;
    double aspect_ratio = 16.0 /9.0;
    int image_width = 800;
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    camera cam = camera(image_width, image_height0);

    // SCENE CONFIGURATION
    // Material types
    lambertian mat_ground{colour(0.45, 0.45, 0.45)};
    lambertian mat_diffuse{colour(0.8, 0.2, 0.2)};
    lambertian mat_blue{colour(0.2, 0.3, 0.9)};
    metal mat_mirror{colour(0.9, 0.9, 0.9), 0.0};
    metal mat_brushed{colour(0.8, 0.5, 0.1), 0.4};
    dielectric mat_glass{1.5};
    dielectric mat_diamond{2.4};

    lambertian mat_tri1{colour(0.2, 0.8, 0.3)};
    lambertian mat_tri2{colour(0.9, 0.7, 0.1)};

    // Array of materials for CUDA rendering
    std::vector<material*> mat_list = {&mat_ground, &mat_diffuse, &mat_blue, 
                                        &mat_mirror, &mat_brushed, &mat_glass,
                                        &mat_diamond, &mat_tri1, &mat_tri2};

    // Placing objects into the world
    world w;
    w.spheres.push_back({{0, -100.5, -1}, 100 ,&mat_ground});
    w.spheres.push_back({{0, 0, -1.2}, 0.5, &mat_diffuse});
    w.spheres.push_back({{-1.1, 0, -1}, 0.5, &mat_glass});
    w.spheres.push_back({{-1.1, 0, -1}, -0.45, &mat_glass}); // hollow interior for glass sphere
    w.spheres.push_back({{1.1, 0, -1}, 0.5, &mat_brushed});
    w.spheres.push_back({{0.0, -0.3, -0.6}, 0.2, &mat_mirror});
    w.spheres.push_back({{0.0, 0.25, -0.5}, 0.1, &mat_diamond});
    w.spheres.push_back({{0.0, 0.1, 0.5}, 0.4, &mat_blue});
    w.triangles.push_back({vec3(-2.5, -0.5, -2.5),
                           vec3(-0.5, -0.5, -2.5),
                           vec3(-1.5, 1.0, -2.0),
                           &mat_tri1});
    w.triangles.push_back({vec3(0.75, -0.5, -3.0),
                           vec3(1.75, -0.5, -3.0),
                           vec3(1.25, 1.2, -3.0),
                           &mat_tri2});
    
    if (render_mode == SIMPLE)
        simple_render(cam, image_width, image_height, samples_per_pixel, w, max_depth);
    else if (render_mode == MULTITHREAD) {
        // Number of threads (by default, the max threads on the device)
        int threads = std::thread::hardware_concurrency();
        std::vector<colour> frame_buffer(image_height*image_width);

        auto t1 = std::chrono::high_resolution_clock::now();

        render_sections(frame_buffer.data(), image_width, image_height, 
                        cam, w, samples_per_pixel, max_depth,
                        64, threads);

        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::clog << "Rendered in " << (int) ms << " ms\n";

        // Write the colours to the ppm from the colours contained in frame_buffer
        std::cout << "P3\n" << image_width << " " << image_height << "\n255\n";
        for (const auto& pixel_colour : frame_buffer)
            write_colour(std::cout, pixel_colour); 
    }
    else if (render_mode == CUDA) {
        int block_size = 16;
        scene_gpu scene = upload_scene(w, mat_list, cam);
        uchar3* d_frame_buffer = nullptr;

        auto t1 = std::chrono::high_resolution_clock::now();
        cuda_render(&d_frame_buffer, image_width, image_height, scene,
                    samples_per_pixel, max_depth, block_size);
        auto t2 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::clog << "Rendered in " << (int) ms << " ms\n";
        
        std::vector<uchar3> h_frame_buffer(image_width * image_height);
        cuda_check(cudaMemcpy(h_frame_buffer.data(), d_frame_buffer, 
                    image_width * image_height * sizeof(uchar3),
                    cudaMemcpyDeviceToHost));
        
        cuda_check(cudaFree(d_frame_buffer));
        free_scene_gpu(scene);
        
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (const auto& pixel_colour : h_frame_buffer)
            std::cout << (int) pixel_colour.x << ' ' << (int) pixel_colour.y << ' ' << (int) pixel_colour.z << '\n';
    }
}