#pragma once
#include <cuda_runtime.h>
#include "scene_gpu.h"


// CUDA version of structs using CUDA supported float3 variables

struct ray_gpu {
    float3 origin, dir;

    __device__ float3 at(float t) const {
        return make_float3(origin.x + t*dir.x, origin.y + t*dir.y, origin.z + t*dir.z);
    }
};

struct hit_rec_gpu {
    float3 point, normal;
    float t;
    bool front_face;
    int mat_idx;
};

// Helper functions using CUDA supported float3

__device__ inline float dot_f(float3 a, float3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
__device__ inline float3 unit_vector_f(float3 v) {
    float squared = dot_f(v, v);
    float inv = 1/sqrtf(squared);
    return make_float3(v.x*inv, v.y*inv, v.z*inv);
}
__device__ inline float3 hadamard_prod_f(float3 a, float3 b) {
    return make_float3(a.x*b.x, a.y*b.y, a.z*b.z);
}
__device__ inline float3 rand_unit_vector_f(curandState* rng) {
    while (true) {
        float3 p = make_float3(curand_uniform(rng)*2 - 1,
                                curand_uniform(rng)*2 - 1,
                                curand_uniform(rng)*2 - 1);
        if (dot_f(p, p) < 1.f) return unit_vector_f(p);
    }
}

__device__ bool hit_sphere(const scene_gpu& scene, int i, ray_gpu r,
                            float t_min, float t_max, hit_rec_gpu& rec) {

    float3 centre = make_float3(scene.sphere_cx[i], scene.sphere_cy[i], scene.sphere_cz[i]);
    float3 oc = make_float3(r.origin.x-centre.x, r.origin.y-centre.y, r.origin.z-centre.z);
    float a = dot_f(r.dir, r.dir);
    float half_b = dot_f(oc, r.dir);
    float radius = scene.sphere_r[i];
    float c = dot_f(oc, oc) - radius*radius;
    float disc = half_b*half_b - a*c;
    if (disc < 0) return false;

    float sqrt_disc = sqrtf(disc);
    float root = (-half_b - sqrt_disc) / a;
    if (root <= t_min || root >= t_max) {
        root = (-half_b + sqrt_disc) / a;
        if (root <= t_min || root >= t_max)
            return false;
    }

    rec.t = root;
    rec.point = r.at(root);
    float3 outward_n = make_float3((rec.point.x-centre.x) / radius,
                                    (rec.point.y-centre.y) / radius,
                                    (rec.point.z-centre.z) / radius);
    rec.front_face = dot_f(r.dir, outward_n) < 0;
    rec.normal = rec.front_face ? outward_n : make_float3(-outward_n.x, -outward_n.y, -outward_n.z);
    rec.mat_idx = scene.sphere_mat[i];
    return true;
}

__device__ bool scene_hit(const scene_gpu& scene, ray_gpu r, float t_min,
                            float t_max, hit_rec_gpu& rec) {

    bool hit = false;
    float closest = t_max;
    hit_rec_gpu temp_rec;
    for (int i = 0; i < scene.num_spheres; ++i) {
        if (hit_sphere(scene, i, r, t_min, closest, temp_rec)) {
            hit = true;
            closest = temp_rec.t;
            rec = temp_rec;
        }
    }
    
    return hit;
}

__device__ float3 reflect_gpu(float3 v, float3 n) {
    return make_float3(v.x - 2*dot_f(v, n) * n.x,
                        v.y - 2*dot_f(v, n) * n.y,
                        v.z - 2*dot_f(v, n) * n.z);
}

__device__ float3 refract_gpu(float3 uv, float3 n, float eta_ratio) {
    float cos_theta = -dot_f(uv, n);
    float3 r_perp = make_float3(eta_ratio * (uv.x + cos_theta*n.x),
                                eta_ratio * (uv.y + cos_theta*n.y),
                                eta_ratio * (uv.z + cos_theta*n.z));
    
    float parallel_len = sqrtf(fabsf(1.f - dot_f(r_perp, r_perp)));
    
    return make_float3(r_perp.x - parallel_len*n.x,
                        r_perp.y - parallel_len*n.y,
                        r_perp.z - parallel_len*n.z);
}

__device__ float reflectance_gpu(float cos, float refrac_idx) {
    float r0 = (1-refrac_idx) / (1+refrac_idx);
    r0 *= r0;
    return r0 + (1-r0) * powf(1-cos, 5);
}
