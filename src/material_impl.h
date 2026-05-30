#pragma once
#include "material.h"
#include "sphere.h"

inline bool lambertian::scatter(const ray& r, const hit_record& rec,
                                colour& attenuation, ray& scattered) const {
    
    // Scatter the ray in a random direction
    vec3 scatter_dir = rec.normal + rand_unit_vector();

    if (scatter_dir.length_squared() < 1e-8) scatter_dir = rec.normal;

    scattered = {rec.point, scatter_dir.unit_vector()};
    attenuation = albedo;

    return true;
}

inline bool metal::scatter(const ray& r, const hit_record& rec,
                           colour& attenuation, ray& scattered) const {

    // Calculate the reflected ray by adding twice the height of the original
    // ray in the opposite direction
    vec3 reflection = r.direction - 2*r.direction.dot(rec.normal)*rec.normal;
    
    // Add random dispersion based on fuzz
    reflection = reflection.unit_vector() + fuzz * rand_unit_vector();
    scattered = {rec.point, reflection};
    attenuation = albedo;

    return (scattered.direction.dot(rec.normal) > 0);
}

inline bool dielectric::scatter(const ray& r, const hit_record& rec,
                           colour& attenuation, ray& scattered) const {
                    
    attenuation = colour(1, 1, 1);
    
    // Ratio of the refractive indices of both materials. The ratio changes 
    // depending on if the ray is entering or exiting the object.
    double ratio = rec.front_face ? 1/refrac_idx : refrac_idx;

    vec3 unit_dir = r.direction.unit_vector();
    double cos_theta = (-unit_dir).dot(rec.normal);
    double sin_theta = std::sqrt(1 - cos_theta*cos_theta);

    // Check if a refraction is possible (if not, total internal reflection
    // has occuredd)
    bool cannot_refract = ratio * sin_theta > 1;

    vec3 out_dir;
    // Randomly chose if a ray should be reflected or refracted based on
    // the material's reflectance.
    if (cannot_refract || reflectance(cos_theta, ratio) > rand_double())
        out_dir = reflect(unit_dir, rec.normal);
    else
        out_dir = refract(unit_dir, rec.normal, ratio);

    scattered = {rec.point, out_dir};
    return true;
}

// Calculate the reflectance of a material using its refractive index
// and Schlick's approximation.
inline double dielectric::reflectance(double cos, double refrac_idx) {
    double r0 = (1-refrac_idx) / (1+refrac_idx);
    r0 = r0*r0;
    return r0 + (1-r0) * std::pow((1-cos), 5);
}