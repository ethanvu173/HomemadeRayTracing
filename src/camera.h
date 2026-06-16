#pragma once
#include "vec3.h"
#include <cmath>
#include <numbers>
constexpr double PI = 3.14159265358979323846;

struct camera {
    vec3 origin, pixel00_loc, pixel_delta_u, pixel_delta_v;

    camera(int img_w, int img_h, vec3 lookfrom={0,0,0},
           vec3 lookat={0,0,-1}, vec3 vup={0,1,0},
           double vfov_deg=90.0) {

        double vfov = vfov_deg * PI / 180;
        double h = std::tan(vfov / 2);
        double focal_length = (lookfrom - lookat).length();
        // Set up viewport
        double vp_h = 2 * h * focal_length;
        double vp_w = vp_h * (double)img_w / img_h;

        origin = lookfrom;
        // Orthonormal basis vectors describing the camera's orientation
        vec3 w = (lookfrom - lookat).unit_vector();
        vec3 u = (vup.cross(w)).unit_vector();
        vec3 v = w.cross(u);
        // Vectors across the horizontal and vertical edges of the viewport
        vec3 vp_u = vp_w * u;
        vec3 vp_v = vp_h * -v;
        // Horizontal and vertical vectors in between pixels
        pixel_delta_u = vp_u / img_w;
        pixel_delta_v = vp_v / img_h;
        // Location of the upper left pixel
        vec3 vp_upper_left = origin - (w * focal_length) - vp_u/2 - vp_v/2;
        pixel00_loc = vp_upper_left + 0.5*(pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int x, int y) const {
        vec3 center = pixel00_loc + x*pixel_delta_u + y*pixel_delta_v;
        return {origin, (center-origin).unit_vector()};
    }
};