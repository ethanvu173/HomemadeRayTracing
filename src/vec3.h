#pragma once
#include <cmath>
#include <iostream>
#include <random>

// Struct with 3 components for locations, colours, directions, etc.
struct vec3 {
    double x, y, z;
    vec3 (double x=0, double y=0, double z=0) : x(x), y(y), z(z) {}

    vec3 operator+(const vec3& o) const {
        return {x+o.x, y+o.y, z+o.z};
    }
    vec3 operator-(const vec3& o) const {
        return {x-o.x, y-o.y, z-o.z};
    }
    vec3 operator*(double t) const {
        return {x*t, y*t, z*t};
    }
    vec3 operator/(double t) const {
        return *this * (1/t);
    }
    vec3 operator-() const {
        return {-x, -y, -z};
    }
    vec3& operator+=(const vec3& o) {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    double dot(const vec3& o) const {
        return x*o.x + y*o.y + z*o.z;
    }
    vec3 cross(const vec3& o) const {
        return {y*o.z-z*o.y,
                z*o.x-x*o.z,
                x*o.y-y*o.x};
    }
    double length() const {
        return std::sqrt(length_squared());
    }
    double length_squared() const {
        return dot(*this);
    }
    vec3 unit_vector() const {
        return *this / length();
    }
    vec3 hadamard_prod(const vec3& o) const {
        return {x*o.x, y*o.y, z*o.z};
    }
};

inline vec3 operator*(double t, const vec3& v) {
    return v * t;
}

using colour = vec3;

void write_colour(std::ostream& out, const colour& pixel_colour) {
    double r = pixel_colour.x;
    double g = pixel_colour.y;  
    double b = pixel_colour.z;  

    int rbyte = int(255.999 * std::sqrt(std::clamp(r, 0.0, 1.0)));
    int gbyte = int(255.999 * std::sqrt(std::clamp(g, 0.0, 1.0)));
    int bbyte = int(255.999 * std::sqrt(std::clamp(b, 0.0, 1.0)));

    out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}

struct ray {
    vec3 origin, direction;
    vec3 at(double t) const {
        return origin + t * direction;
    }
};

inline double rand_double() {
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

inline vec3 rand_unit_vector() {
    while (true) {
        vec3 p{rand_double()*2-1, rand_double()*2-1, rand_double()*2-1};
        if (p.length_squared() < 1) return p.unit_vector();
    }
};

// Find a reflected ray by adding twice the height in the opposite direction
inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - 2 * v.dot(n) * n;
}

// Find the refracted ray using the ratio of refractive indices
inline vec3 refract(const vec3& uv, const vec3& n, double eta_ratio) {
    double cos_theta = (-uv).dot(n);

    // Perpendicular componenent of refracted ray
    vec3 r_perp = eta_ratio * (uv + cos_theta * n);

    // Parallel component
    vec3 r_parallel = -std::sqrt(std::abs(1.0-r_perp.length_squared())) * n;

    // Perp + parallel = total length of 1
    return r_perp + r_parallel;
}