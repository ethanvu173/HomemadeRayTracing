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
