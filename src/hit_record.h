#pragma once
#include "vec3.h"

struct material;

struct hit_record {
    vec3 point, normal;
    double t;
    bool front_face;
    material* mat = nullptr;
};