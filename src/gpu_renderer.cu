#include "gpu_renderer.cuh"
#include <curand_kernel.h>
#include "scene_gpu.h"
#include "gpu_helper.h"


// Non-recursive version of ray_colour for CUDA optimization 
__device__ float3 ray_colour_gpu(ray_gpu r, const scene_gpu& scene, 
                                    curandState* rng, int max_depth) {

    float3 attenuation = make_float3(1, 1, 1);  // white colour

    for (int i = 0; i < max_depth; ++i) {
        hit_rec_gpu rec;
        // If there is no hit, i.e. if the ray hits the sky
        if (!scene_hit(scene, r, 1e-4f, 1e9f, rec)) {
            float3 unit_direction = unit_vector_f(r.dir);
            float a = 0.5f * (unit_direction.y + 1.f);
            float3 sky = make_float3(1-0.5f*a, 1-0.3f*a, 1.f);
            // Return sky gradient
            return make_float3(attenuation.x*sky.x, attenuation.y*sky.y, attenuation.z*sky.z);
        }

        // Ray hits an object. Initialize new material and ray information.
        int mat_i = rec.mat_idx;
        int type = scene.mat_type[mat_i];
        float3 new_attenuation;
        ray_gpu scattered;

        // Handle ray scattering based on material type
        if (type == LAMBERTIAN) {
            float3 rand_vector = rand_unit_vector_f(rng);
            float3 scattered_dir = make_float3(rec.normal.x + rand_vector.x,
                                                rec.normal.y + rand_vector.y,
                                                rec.normal.z + rand_vector.z);
            scattered = {rec.point, unit_vector_f(scattered_dir)};
            new_attenuation = make_float3(scene.mat_r[mat_i], scene.mat_g[mat_i], scene.mat_b[mat_i]);
        }
        else if (type == METAL) {
            // massively bloated implementation but make_float3 is like that
            float3 reflection = make_float3(r.dir.x - 2*dot_f(r.dir, rec.normal) * rec.normal.x,
                                            r.dir.y - 2*dot_f(r.dir, rec.normal) * rec.normal.y,
                                            r.dir.z - 2*dot_f(r.dir, rec.normal) * rec.normal.z);
            float3 rand_vector = rand_unit_vector_f(rng);
            float fuzz = scene.fuzz[mat_i];
            
            // Metallic reflections with fuzz
            reflection = make_float3(reflection.x + fuzz*rand_vector.x,
                                        reflection.y + fuzz*rand_vector.y,
                                        reflection.z + fuzz*rand_vector.z);

            scattered = {rec.point, unit_vector_f(reflection)};
            new_attenuation = make_float3(scene.mat_r[mat_i], scene.mat_g[mat_i], scene.mat_b[mat_i]);

            if (dot_f(scattered.dir, rec.normal) <= 0) return make_float3(0, 0, 0);
        }
        else if (type == DIELECTRIC) {
            float refrac_idx = scene.refrac_idx[mat_i];
            float ratio = rec.front_face ? (1.f/refrac_idx) : refrac_idx;

            float3 unit_dir = unit_vector_f(r.dir);

            float cos_theta = -dot_f(unit_dir, rec.normal);
            float sin_theta = sqrtf(1 - cos_theta*cos_theta);

            bool cannot_reflect = ratio * sin_theta > 1.f;
            bool rand_chance = reflectance_gpu(cos_theta, ratio) > curand_uniform(rng);
            float3 out_dir;

            // Randomly chose if a ray should reflect or refract based on
            // the materials reflectance
            if (cannot_reflect || rand_chance) {
                out_dir = reflect_gpu(unit_dir, rec.normal);
            } else {
                out_dir = refract_gpu(unit_dir, rec.normal, ratio);
            }

            scattered = {rec.point, out_dir};
            new_attenuation = make_float3(1, 1, 1);
        }
        attenuation = hadamard_prod_f(attenuation, new_attenuation);
        r = scattered;
    }
    // Max depth reached, return black colour
    return make_float3(0, 0, 0); 
}

// Render the scene using CUDA. Output all rendered pixel colour values to
// the frame buffer pointer fb.
__global__ void render_kernel(uchar3* fb, int W, int H, scene_gpu scene,
                                int spp, int max_depth, unsigned long long seed) {

    // Assign each thread a pixel
    int p_x = blockIdx.x*blockDim.x + threadIdx.x;
    int p_y = blockIdx.y*blockDim.y + threadIdx.y;
    if (p_x >= W || p_y >= H) return;

    // Generate a random state for each pixel.
    curandState rng;
    curand_init(seed, p_y*W+p_x, 0, &rng);

    float3 colour = make_float3(0, 0, 0);
    for (int sample = 0; sample < spp; ++sample) {
        float r_u = curand_uniform(&rng) - 0.5f;
        float r_v = curand_uniform(&rng) - 0.5f;

        // make_float3 is so cooool look how clean this is in comparison to cpu implementaiton
        float3 target = make_float3(
            scene.pixel00_loc.x + (p_x+r_u)*scene.pixel_delta_u.x + (p_y+r_v)*scene.pixel_delta_v.x, 
            scene.pixel00_loc.y + (p_x+r_u)*scene.pixel_delta_u.y + (p_y+r_v)*scene.pixel_delta_v.y,
            scene.pixel00_loc.z + (p_x+r_u)*scene.pixel_delta_u.z + (p_y+r_v)*scene.pixel_delta_v.z  
        );
        float3 dir = unit_vector_f(make_float3(target.x - scene.cam_origin.x,
                                                target.y - scene.cam_origin.y,
                                                target.z - scene.cam_origin.z));
        ray_gpu r{scene.cam_origin, dir};
        float3 new_colour = ray_colour_gpu(r, scene, &rng, max_depth);
        colour.x += new_colour.x;
        colour.y += new_colour.y;
        colour.z += new_colour.z;
    }
    // Get the average colour from all samples
    colour.x /= spp;
    colour.y /= spp;
    colour.z /= spp;
    // Write to the frame buffer 
    fb[p_y*W+p_x] = make_uchar3(
        (unsigned char) (255.f * sqrtf(fminf(colour.x, 1.f))),
        (unsigned char) (255.f * sqrtf(fminf(colour.y, 1.f))),
        (unsigned char) (255.f * sqrtf(fminf(colour.z, 1.f)))
    );
}

// Globally accessible call to launch the CUDA rendering
void cuda_render(uchar3** d_fb, int W, int H, const scene_gpu& scene,
                    int spp, int max_depth, int block_size) {

    // Used for curand functions
    unsigned long long rng_seed = 1234LL;
    
    cuda_check(cudaMalloc(d_fb, W*H*sizeof(uchar3)));
    
    dim3 block(block_size, block_size);
    dim3 grid((W+block_size-1)/block_size, (H+block_size-1)/block_size);

    render_kernel<<<grid, block>>>(*d_fb, W, H, scene, spp, max_depth, rng_seed);
    cuda_check(cudaDeviceSynchronize());
} 