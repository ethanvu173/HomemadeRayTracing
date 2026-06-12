#pragma once
#include <vector>
#include <algorithm>
#include <cuda_runtime.h>
#include <source_location>
#include <cstdio>
#include <cstdlib>
#include "hittable.h"
#include "camera.h"


enum mat_type {
    LAMBERTIAN,
    METAL,
    DIELECTRIC
};

// Store all data in arrays to make accessing across threads possible.
struct scene_gpu {
    float* sphere_cx, *sphere_cy, *sphere_cz; // Centre coordinates of sphere
    float* sphere_r;
    int* sphere_mat;
    int num_spheres;

    int* mat_type;
    float* mat_r, *mat_g, *mat_b;
    float* fuzz, *refrac_idx;
    int num_mats;

    float3 cam_origin, pixel00_loc, pixel_delta_u, pixel_delta_v;
};

// Convert data about the world, object materials, and camera into
// CUDA-compatible formats. Return as a scene_gpu object.
inline scene_gpu upload_scene(const world& w, const std::vector<material*> mats,
                                const camera& cam) {

    int num_spheres = (int) w.spheres.size();
    int num_mats = (int) mats.size();

    std::vector<float> cx(num_spheres), cy(num_spheres), cz(num_spheres), r(num_spheres);
    std::vector<int> s_mats(num_spheres);

    for (int i = 0; i < num_spheres; ++i) {
        cx[i] = (float) w.spheres[i].centre.x;
        cy[i] = (float) w.spheres[i].centre.y;
        cz[i] = (float) w.spheres[i].centre.z;
        r[i] = (float) w.spheres[i].radius;
        // Search for the sphere's material from the array of materials
        s_mats[i] = (int) (std::find(mats.begin(), mats.end(),
                        w.spheres[i].mat) - mats.begin());
    }

    // Arrays of material types, colours, fuzz for metals, and index of refraction for dielectrics. 
    std::vector<int> m_types(num_mats);
    std::vector<float> m_r(num_mats), m_g(num_mats), m_b(num_mats), m_fuzz(num_mats), m_refrac_idx(num_mats);

    // Determine the material type of the material at mat[i]
    for (int i = 0; i < num_mats; ++i) {
        if (auto* lamb = dynamic_cast<lambertian*>(mats[i])) {  // returns nullptr if cast is unsuccessful
            m_types[i] = LAMBERTIAN;
            m_r[i] = (float) lamb->albedo.x;
            m_g[i] = (float) lamb->albedo.y;
            m_b[i] = (float) lamb->albedo.z;
        }

        else if (auto* met = dynamic_cast<metal*>(mats[i])) {
            m_types[i] = METAL;
            m_r[i] = (float) met->albedo.x;
            m_g[i] = (float) met->albedo.y;
            m_b[i] = (float) met->albedo.z;
            m_fuzz[i] = (float) met->fuzz;
        }

        else if (auto* dielec = dynamic_cast<dielectric*>(mats[i])) {
            m_types[i] = DIELECTRIC;
            m_refrac_idx[i] = (float) dielec->refrac_idx;
        }
    }

    scene_gpu scene{};
    scene.num_spheres = num_spheres;
    scene.num_mats = num_mats;

    // Helpers to allocate arrays and copy from the CPU
    auto cuda_alloc_float = [&](float** dst, std::vector<float>& src) {
        cudaMalloc(dst, src.size()*sizeof(float));
        cudaMemcpy(*dst, src.data(), src.size()*sizeof(float),
                    cudaMemcpyHostToDevice);
    };
    auto cuda_alloc_int = [&](int** dst, std::vector<int>& src) {
        cudaMalloc(dst, src.size()*sizeof(int));
        cudaMemcpy(*dst, src.data(), src.size()*sizeof(int),
                    cudaMemcpyHostToDevice);
    };

    // Allocate all data from the scene
    cuda_alloc_float(&scene.sphere_cx, cx);
    cuda_alloc_float(&scene.sphere_cy, cy);
    cuda_alloc_float(&scene.sphere_cz, cz);
    cuda_alloc_float(&scene.sphere_r, r);
    cuda_alloc_int(&scene.sphere_mat, s_mats);
    cuda_alloc_int(&scene.mat_type, m_types);
    cuda_alloc_float(&scene.mat_r, m_r);
    cuda_alloc_float(&scene.mat_g, m_g);
    cuda_alloc_float(&scene.mat_b, m_b);
    cuda_alloc_float(&scene.fuzz, m_fuzz);
    cuda_alloc_float(&scene.refrac_idx, m_refrac_idx);

    scene.cam_origin = make_float3((float) cam.origin.x, (float) cam.origin.y, (float) cam.origin.z);
    scene.pixel00_loc = make_float3((float) cam.pixel00_loc.x, (float) cam.pixel00_loc.y, (float) cam.pixel00_loc.z);
    scene.pixel_delta_u = make_float3((float) cam.pixel_delta_u.x, (float) cam.pixel_delta_u.y, (float) cam.pixel_delta_u.z);
    scene.pixel_delta_v = make_float3((float) cam.pixel_delta_v.x, (float) cam.pixel_delta_v.y, (float) cam.pixel_delta_v.z);

    return scene;
}

inline void free_scene_gpu(scene_gpu& scene) {
    cudaFree(scene.sphere_cx);
    cudaFree(scene.sphere_cy);
    cudaFree(scene.sphere_cz);
    cudaFree(scene.sphere_r);
    cudaFree(scene.sphere_mat);
    cudaFree(scene.mat_type);
    cudaFree(scene.mat_r);
    cudaFree(scene.mat_g);
    cudaFree(scene.mat_b);
    cudaFree(scene.fuzz);
    cudaFree(scene.refrac_idx);
}

