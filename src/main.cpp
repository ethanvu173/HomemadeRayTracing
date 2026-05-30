#include "camera.h"
#include "sphere.h"
#include "hittable.h"
#include <iostream>

// Generates the colour of a ray emitted by the camera, based on collisions
// with objects in the world. 
colour ray_colour(const ray& r, world& w, int depth) {
    if (depth <= 0) return {0, 0, 0};

    hit_record rec;
    if (w.hit(r, 1e-8, 1e9, rec)) {
        ray scattered;
        colour attenuation;
        // Recursively call ray_colour for scattered rays
        if (rec.mat->scatter(r, rec, attenuation, scattered))
            return attenuation.hadamard_prod(ray_colour(scattered, w, depth-1));
        return {0, 0, 0};  // Ray absorbed by material
    }

    // Background gradient
    vec3 unit_direction = r.direction.unit_vector();
    auto a = 0.5*(unit_direction.y + 1.0);
    return (1.0-a)*colour(1.0, 1.0, 1.0) + a*colour(0.5, 0.7, 1.0);
}

int main() {
    const int samples_per_pixel = 100;
    const int max_depth = 50;
    double aspect_ratio = 16.0 /9.0;
    int image_width = 800;
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    camera cam = camera(image_width, image_height);

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
       
    // Generate the image
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

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
    std::clog << "\rDone.                    \n";
}