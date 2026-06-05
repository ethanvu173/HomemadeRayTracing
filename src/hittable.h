#pragma once
#include "vec3.h"
#include "sphere.h"
#include "triangle.h"
#include "material.h"
#include <vector>

struct world {
    std::vector<sphere> spheres;
    std::vector<triangle> triangles;

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        hit_record temp_rec;
        bool hit_anything = false;
        double closest = t_max;

        // Find if the ray hits any spheres
        for (const auto& s : spheres) {
            if (s.hit(r, t_min, closest, temp_rec)) {
                hit_anything = true;
                closest = temp_rec.t;
                rec = temp_rec;
            }
        }

        // Find if the ray hits any triangles
        for (const auto& t : triangles) {
            if (t.hit(r, t_min, closest, temp_rec)) {
                hit_anything = true;
                closest = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

// Generates the colour of a ray emitted by the camera, based on collisions
// with objects in the world. 
colour ray_colour(const ray& r, const world& w, int depth) {
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