#pragma once
#include "vec3.h"
#include "material.h"
#include "hit_record.h"


struct triangle {
    vec3 v0, v1, v2;
    material* mat;

    // Calculate a hit from a ray with the Moller-Trumbore intersection algorithm
    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        vec3 e1 = v1 - v0, e2 = v2 - v0;
        vec3 h = r.direction.cross(e2);
        double a = e1.dot(h);

        if (std::abs(a) < 1e-8) return false; // triangle is parallel to ray

        double f = 1.0/a;
        vec3 s = r.origin - v0;
        double u = f*s.dot(h);
        if (u < 0 || u > 1) return false; // ray is outside e2's bounds

        vec3 q = s.cross(e1);
        double v = f * r.direction.dot(q);
        if (v < 0 || u+v > 1) return false; // ray is outside e1's bounds

        double t = f * e2.dot(q);
        if (t < t_min || t > t_max) return false; 

        // There is a hit, record it.
        rec.t = t;
        rec.point = r.at(t);
        vec3 outward_n = e1.cross(e2).unit_vector();
        rec.front_face = r.direction.dot(outward_n) < 0;
        rec.normal = rec.front_face ? outward_n : - outward_n;
        rec.mat = mat;
        return true;
    }
};