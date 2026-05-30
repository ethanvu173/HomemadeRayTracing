#pragma once
#include "vec3.h"

struct ray;
struct hit_record;


struct material {
    virtual bool scatter(const ray& r, const hit_record& rec, 
                         colour& attenuation, ray& scattered) const = 0;
    virtual ~material() = default;
};

// Lambertian (diffuse) material. Light is scattered in a random direction.
struct lambertian : material {
    colour albedo; // Surface colour

    lambertian(const colour& a) : albedo(a) {}

    bool scatter(const ray& r, const hit_record& rec,
                 colour& attenuation, ray& scattered) const override;   
};

// Metal material for reflective surfaces.
struct metal : material {
    colour albedo;
    double fuzz; // 0 = completely reflective, 1 = very rough surface
    metal(colour a, double f) : albedo(a), fuzz(f) {}

    bool scatter(const ray& r, const hit_record& rec, 
                 colour& attenuation, ray& scattered) const override;
};

// Dielectric (glass) material for transparency.
struct dielectric : material {
    double refrac_idx;
    dielectric(double r) : refrac_idx(r) {}
    
    bool scatter(const ray& r, const hit_record& rec, 
                 colour& attenuation, ray& scattered) const override;
    

    private: 
        static double reflectance(double cos, double refrac_idx);
};

