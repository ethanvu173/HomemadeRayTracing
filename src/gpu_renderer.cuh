#pragma once
#include "scene_gpu.h"

// Globally accessible call to launch the CUDA rendering
void cuda_render(uchar3** d_fb, int W, int H, const scene_gpu& scene,
                    int spp, int max_depth, int block_size);