#pragma once
#include "vec3.h"
#include "material.h"
#include "hit_record.h"


struct sphere {
    vec3 centre;
    double radius;
    material* mat;

    sphere(vec3 c, double r, material *m) {
        centre = c;
        radius = r;
        mat = m;
    }

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
        // Use quadratic formula to determine a hit
        vec3 oc = r.origin - centre;
        double a = r.direction.length_squared();
        double half_b = oc.dot(r.direction);
        double c = oc.length_squared() - radius*radius;
        double disc = half_b*half_b - a*c;
        if (disc < 0) 
            return false;

        // Ensure the hit is within the allowed time frame
        double sqrt_disc = std::sqrt(disc);
        double root = (-half_b - sqrt_disc) / a;
        if (root <= t_min || root >= t_max) {
            root = (-half_b + sqrt_disc) / a;
            if (root <= t_min || root >= t_max) 
                return false;
        }

        // There is a hit. Set the hit record variables.
        rec.t = root;
        rec.point = r.at(root);
        vec3 outward_n = (rec.point - centre) / radius;
        rec.front_face = r.direction.dot(outward_n) < 0;
        rec.normal = rec.front_face ? outward_n : -outward_n;
        rec.mat = mat;

        return true;
    }
};

#include "material_impl.h"