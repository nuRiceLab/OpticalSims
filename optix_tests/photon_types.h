#pragma once

#include <cuda_runtime.h>
#include <optix.h>

// Photon simulation parameters
struct PhotonParams
{
    // Launch parameters
    OptixTraversableHandle handle;
    float3*                photon_positions;
    float3*                photon_directions;
    float*                 photon_wavelengths;
    unsigned int*          photon_flags;      // alive/absorbed/detected
    float*                 photon_times;
    unsigned int*          hit_counts;
    unsigned int*          bounce_counts;
    
    // Geometry info
    unsigned int           num_photons;
    unsigned int           max_bounces;
    float                  world_size;
    
    // Optical properties (simplified)
    float                  absorption_length;
    float                  scattering_length;
    float                  refractive_index;
    
    // Random seed
    unsigned int           seed;
};

// Photon state flags
enum PhotonFlag
{
    PHOTON_ALIVE    = 0,
    PHOTON_ABSORBED = 1,
    PHOTON_DETECTED = 2,
    PHOTON_ESCAPED  = 3
};

// SBT record structures
struct RayGenData
{
    // empty for now
};

struct MissData
{
    float3 bg_color;
};

struct HitGroupData
{
    float3* vertices;
    uint3*  indices;
    int     material_id;
    float   reflectivity;
};

template<typename T>
struct SbtRecord
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

typedef SbtRecord<RayGenData>   RayGenSbtRecord;
typedef SbtRecord<MissData>     MissSbtRecord;
typedef SbtRecord<HitGroupData> HitGroupSbtRecord;
