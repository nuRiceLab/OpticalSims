#include <optix.h>
#include <cuda_runtime.h>
#include "photon_types.h"

// Vector math helpers
__device__ __forceinline__ float3 operator+(const float3& a, const float3& b) {
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__device__ __forceinline__ float3 operator-(const float3& a, const float3& b) {
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__device__ __forceinline__ float3 operator*(float s, const float3& v) {
    return make_float3(s * v.x, s * v.y, s * v.z);
}

__device__ __forceinline__ float3 operator*(const float3& v, float s) {
    return make_float3(v.x * s, v.y * s, v.z * s);
}

__device__ __forceinline__ float3 operator-(const float3& v) {
    return make_float3(-v.x, -v.y, -v.z);
}

__device__ __forceinline__ float dot(const float3& a, const float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ __forceinline__ float3 cross(const float3& a, const float3& b) {
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

__device__ __forceinline__ float length(const float3& v) {
    return sqrtf(dot(v, v));
}

__device__ __forceinline__ float3 normalize(const float3& v) {
    float inv_len = 1.0f / length(v);
    return v * inv_len;
}

extern "C" {
__constant__ PhotonParams params;
}

// Simple LCG random number generator
__device__ __forceinline__ unsigned int lcg(unsigned int& seed)
{
    seed = seed * 1664525u + 1013904223u;
    return seed;
}

__device__ __forceinline__ float rnd(unsigned int& seed)
{
    return static_cast<float>(lcg(seed)) / static_cast<float>(0xFFFFFFFFu);
}

// Sample direction on unit sphere
__device__ __forceinline__ float3 sample_sphere(unsigned int& seed)
{
    float z = 2.0f * rnd(seed) - 1.0f;
    float phi = 2.0f * M_PI * rnd(seed);
    float r = sqrtf(1.0f - z * z);
    return make_float3(r * cosf(phi), r * sinf(phi), z);
}

// Fresnel reflectance for dielectric
__device__ __forceinline__ float fresnel_dielectric(float cos_i, float n1, float n2)
{
    float n = n1 / n2;
    float sin_t2 = n * n * (1.0f - cos_i * cos_i);
    
    if (sin_t2 > 1.0f) return 1.0f; // Total internal reflection
    
    float cos_t = sqrtf(1.0f - sin_t2);
    float r_s = (n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t);
    float r_p = (n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t);
    
    return 0.5f * (r_s * r_s + r_p * r_p);
}

// Reflect direction
__device__ __forceinline__ float3 reflect(const float3& dir, const float3& normal)
{
    return dir - 2.0f * dot(dir, normal) * normal;
}

// Refract direction
__device__ __forceinline__ float3 refract(const float3& dir, const float3& normal, float eta)
{
    float cos_i = -dot(dir, normal);
    float sin_t2 = eta * eta * (1.0f - cos_i * cos_i);
    float cos_t = sqrtf(1.0f - sin_t2);
    return eta * dir + (eta * cos_i - cos_t) * normal;
}

extern "C" __global__ void __raygen__photon_trace()
{
    const uint3 idx = optixGetLaunchIndex();
    const uint3 dim = optixGetLaunchDimensions();
    
    unsigned int photon_idx = idx.x + idx.y * dim.x;
    if (photon_idx >= params.num_photons) return;
    
    // Initialize random seed per photon
    unsigned int seed = params.seed ^ (photon_idx * 1973u + 9277u);
    
    // Get initial photon state
    float3 pos = params.photon_positions[photon_idx];
    float3 dir = params.photon_directions[photon_idx];
    float wavelength = params.photon_wavelengths[photon_idx];
    float time = params.photon_times[photon_idx];
    
    unsigned int bounces = 0;
    unsigned int flag = PHOTON_ALIVE;
    
    // Speed of light in medium (simplified)
    const float c = 299.792458f / params.refractive_index; // mm/ns
    
    while (flag == PHOTON_ALIVE && bounces < params.max_bounces)
    {
        // Sample interaction distance
        float scatter_dist = -params.scattering_length * logf(rnd(seed) + 1e-10f);
        float absorb_dist = -params.absorption_length * logf(rnd(seed) + 1e-10f);
        float interaction_dist = fminf(scatter_dist, absorb_dist);
        
        // Trace ray
        unsigned int p0, p1, p2, p3;
        p0 = __float_as_uint(pos.x);
        p1 = __float_as_uint(pos.y);
        p2 = __float_as_uint(pos.z);
        p3 = 0; // hit flag
        
        optixTrace(
            params.handle,
            pos,
            dir,
            0.001f,              // tmin
            interaction_dist,    // tmax
            0.0f,                // ray time
            OptixVisibilityMask(255),
            OPTIX_RAY_FLAG_NONE,
            0,                   // SBT offset
            1,                   // SBT stride
            0,                   // miss SBT index
            p0, p1, p2, p3
        );
        
        float hit_t = __uint_as_float(p0);
        float hit_nx = __uint_as_float(p1);
        float hit_ny = __uint_as_float(p2);
        unsigned int hit_flag = p3;
        
        if (hit_flag == 1) // Hit geometry
        {
            // Move to hit point
            float3 hit_normal = make_float3(hit_nx, hit_ny, __uint_as_float(p0));
            pos = pos + hit_t * dir;
            time += hit_t / c;
            
            // Get surface normal from payload (simplified - using sphere normal)
            float3 normal = normalize(pos); // Assumes sphere at origin
            if (dot(dir, normal) > 0) normal = -normal;
            
            // Fresnel reflection/refraction
            float cos_i = -dot(dir, normal);
            float R = fresnel_dielectric(cos_i, 1.0f, params.refractive_index);
            
            if (rnd(seed) < R)
            {
                // Reflect
                dir = reflect(dir, normal);
            }
            else
            {
                // Refract (or absorb at detector)
                float rand_val = rnd(seed);
                if (rand_val < 0.1f) // 10% detection probability
                {
                    flag = PHOTON_DETECTED;
                    atomicAdd(&params.hit_counts[0], 1);
                }
                else if (rand_val < 0.2f) // 10% absorption
                {
                    flag = PHOTON_ABSORBED;
                }
                else
                {
                    dir = refract(dir, normal, 1.0f / params.refractive_index);
                }
            }
            bounces++;
        }
        else // No hit - check for scattering vs absorption
        {
            if (scatter_dist < absorb_dist)
            {
                // Scatter - move and randomize direction
                pos = pos + scatter_dist * dir;
                time += scatter_dist / c;
                
                // Rayleigh-like scattering (isotropic for simplicity)
                dir = sample_sphere(seed);
                bounces++;
            }
            else
            {
                // Absorbed in medium
                flag = PHOTON_ABSORBED;
            }
        }
        
        // Check if escaped world
        float dist_from_origin = length(pos);
        if (dist_from_origin > params.world_size)
        {
            flag = PHOTON_ESCAPED;
        }
    }
    
    // Write final state
    params.photon_positions[photon_idx] = pos;
    params.photon_directions[photon_idx] = dir;
    params.photon_flags[photon_idx] = flag;
    params.photon_times[photon_idx] = time;
    params.bounce_counts[photon_idx] = bounces;
}

extern "C" __global__ void __miss__photon()
{
    // No geometry hit - photon continues in medium
    optixSetPayload_3(0); // hit_flag = 0
}

extern "C" __global__ void __closesthit__photon()
{
    // Get hit information
    const float t = optixGetRayTmax();
    
    // Get triangle data
    HitGroupData* data = reinterpret_cast<HitGroupData*>(optixGetSbtDataPointer());
    const uint3 idx = data->indices[optixGetPrimitiveIndex()];
    
    const float3 v0 = data->vertices[idx.x];
    const float3 v1 = data->vertices[idx.y];
    const float3 v2 = data->vertices[idx.z];
    
    // Compute normal
    const float3 e1 = v1 - v0;
    const float3 e2 = v2 - v0;
    float3 normal = normalize(cross(e1, e2));
    
    // Pack into payload
    optixSetPayload_0(__float_as_uint(t));
    optixSetPayload_1(__float_as_uint(normal.x));
    optixSetPayload_2(__float_as_uint(normal.y));
    optixSetPayload_3(1); // hit_flag = 1 (hit)
}
