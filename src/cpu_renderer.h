#pragma once
#include <thread>
#include <atomic>
#include "camera.h"
#include "hittable.h"


colour ray_colour(const ray& r, const world& w, int depth);

struct section {
    int x0, y0, x1, y1;
};

// Render a single pixel, taking spp samples and returning the average colour.
inline colour render_pixel(int x, int y, 
                            const camera& cam, const world &w, 
                            int spp, int max_depth) {

    colour pixel_colour{0, 0, 0};
    for (int sample = 0; sample < spp; sample++) {
        double r_u = rand_double() - 0.5;
        double r_v = rand_double() - 0.5;
        vec3 centre = cam.pixel00_loc + (x+r_u) * cam.pixel_delta_u + (y+r_v) * cam.pixel_delta_v;
        
        ray r{cam.origin, (centre - cam.origin).unit_vector()};
        pixel_colour += ray_colour(r, w, max_depth);
    }
    return pixel_colour / spp;
}

// Render the world by splitting the scene into sections, and dispatching
// the sections to be rendered by individual threads.
inline void render_sections(colour* fb, int W, int H,
                            const camera& cam, const world& w, 
                            int spp, int max_depth, 
                            int tile_size=64, int num_threads=0) {

    // Generate sections and their coordinates
    std::vector<section> sections;
    for (int y = 0; y < H; y += tile_size) {
        for (int x = 0; x < W; x += tile_size) {
            sections.push_back({x, y,
                                std::min(x+tile_size, W),
                                std::min(y+tile_size, H)});
        }
    }

    if (num_threads == 0)
        num_threads = std::thread::hardware_concurrency();

    std::atomic<int> next_section{0};
    std::vector<std::thread> workers;

    // Assign threads to a section to render
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back([&]() {
            while (true) {
                int index = next_section.fetch_add(1);
                if (index >= (int) sections.size()) break;  // threads exceed the sections 

                section& s = sections[index];
                
                // Render the pixels bounded by the section coordinates
                for (int y = s.y0; y < s.y1; ++y) 
                    for (int x = s.x0; x < s.x1; ++x)
                        fb[y*W + x] = render_pixel(x, y, cam, w, spp, max_depth);
            }
        });
    }

    for (auto& worker : workers) worker.join();
}