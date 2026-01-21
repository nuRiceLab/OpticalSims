/*
 * OptiX Photon Simulation Benchmark
 * 
 * Designed to compare ray tracing performance between compute GPUs (A100)
 * and RTX GPUs (RTX 5090) for optical photon simulation workloads.
 * 
 * Benchmark scenarios:
 * 1. Simple geometry (spheres) - baseline BVH traversal
 * 2. Complex geometry (tessellated detector) - realistic detector simulation
 * 3. High scatter rate - many short rays (Opticks-like workload)
 * 4. Low scatter rate - long rays with few bounces
 * 
 * Metrics reported:
 * - BVH build time
 * - Trace time per photon batch
 * - Photons/second throughput
 * - Memory bandwidth utilization
 */

#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include <optix_stack_size.h>

#include <cuda_runtime.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <fstream>
#include <cmath>
#include <cstring>
#include <random>
#include <map>
#include <utility>

#include "photon_types.h"

#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t error = call;                                              \
        if (error != cudaSuccess) {                                            \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__        \
                      << ": " << cudaGetErrorString(error) << std::endl;       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define OPTIX_CHECK(call)                                                      \
    do {                                                                       \
        OptixResult res = call;                                                \
        if (res != OPTIX_SUCCESS) {                                            \
            std::cerr << "OptiX error at " << __FILE__ << ":" << __LINE__       \
                      << ": " << optixGetErrorString(res) << std::endl;        \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

static void context_log_cb(unsigned int level, const char* tag, const char* message, void*)
{
    std::cerr << "[" << std::setw(2) << level << "][" << std::setw(12) << tag << "]: "
              << message << std::endl;
}

// Generate sphere mesh
void generate_sphere_mesh(
    std::vector<float3>& vertices,
    std::vector<uint3>& indices,
    float3 center,
    float radius,
    int subdivisions)
{
    // Icosphere generation
    const float t = (1.0f + sqrtf(5.0f)) / 2.0f;
    
    std::vector<float3> base_verts = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };
    
    std::vector<uint3> base_faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };
    
    // Normalize base vertices
    for (auto& v : base_verts) {
        float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
        v.x /= len; v.y /= len; v.z /= len;
    }
    
    // Subdivide
    auto midpoint = [](const float3& a, const float3& b) -> float3 {
        float3 m = {(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
        float len = sqrtf(m.x*m.x + m.y*m.y + m.z*m.z);
        return {m.x / len, m.y / len, m.z / len};
    };
    
    for (int s = 0; s < subdivisions; s++) {
        std::vector<uint3> new_faces;
        std::vector<std::pair<std::pair<int,int>, int>> edge_midpoints;
        
        auto find_midpoint = [&](int i1, int i2) -> int {
            auto key = std::make_pair(std::min(i1, i2), std::max(i1, i2));
            for (const auto& em : edge_midpoints) {
                if (em.first == key) return em.second;
            }
            return -1;
        };
        
        auto get_midpoint = [&](int i1, int i2) -> int {
            int found = find_midpoint(i1, i2);
            if (found >= 0) return found;
            
            auto key = std::make_pair(std::min(i1, i2), std::max(i1, i2));
            int idx = base_verts.size();
            base_verts.push_back(midpoint(base_verts[i1], base_verts[i2]));
            edge_midpoints.push_back(std::make_pair(key, idx));
            return idx;
        };
        
        for (const auto& f : base_faces) {
            int a = get_midpoint(f.x, f.y);
            int b = get_midpoint(f.y, f.z);
            int c = get_midpoint(f.z, f.x);
            
            new_faces.push_back({f.x, a, c});
            new_faces.push_back({f.y, b, a});
            new_faces.push_back({f.z, c, b});
            new_faces.push_back({a, b, c});
        }
        base_faces = new_faces;
    }
    
    // Transform to final position
    unsigned int base_idx = vertices.size();
    for (const auto& v : base_verts) {
        vertices.push_back({
            center.x + v.x * radius,
            center.y + v.y * radius,
            center.z + v.z * radius
        });
    }
    
    for (const auto& f : base_faces) {
        indices.push_back({
            base_idx + f.x,
            base_idx + f.y,
            base_idx + f.z
        });
    }
}

// Generate box mesh
void generate_box_mesh(
    std::vector<float3>& vertices,
    std::vector<uint3>& indices,
    float3 min_corner,
    float3 max_corner)
{
    unsigned int base_idx = vertices.size();
    
    // 8 corners
    vertices.push_back({min_corner.x, min_corner.y, min_corner.z}); // 0
    vertices.push_back({max_corner.x, min_corner.y, min_corner.z}); // 1
    vertices.push_back({max_corner.x, max_corner.y, min_corner.z}); // 2
    vertices.push_back({min_corner.x, max_corner.y, min_corner.z}); // 3
    vertices.push_back({min_corner.x, min_corner.y, max_corner.z}); // 4
    vertices.push_back({max_corner.x, min_corner.y, max_corner.z}); // 5
    vertices.push_back({max_corner.x, max_corner.y, max_corner.z}); // 6
    vertices.push_back({min_corner.x, max_corner.y, max_corner.z}); // 7
    
    // 12 triangles (2 per face)
    // Front
    indices.push_back({base_idx + 0, base_idx + 1, base_idx + 2});
    indices.push_back({base_idx + 0, base_idx + 2, base_idx + 3});
    // Back
    indices.push_back({base_idx + 5, base_idx + 4, base_idx + 7});
    indices.push_back({base_idx + 5, base_idx + 7, base_idx + 6});
    // Left
    indices.push_back({base_idx + 4, base_idx + 0, base_idx + 3});
    indices.push_back({base_idx + 4, base_idx + 3, base_idx + 7});
    // Right
    indices.push_back({base_idx + 1, base_idx + 5, base_idx + 6});
    indices.push_back({base_idx + 1, base_idx + 6, base_idx + 2});
    // Top
    indices.push_back({base_idx + 3, base_idx + 2, base_idx + 6});
    indices.push_back({base_idx + 3, base_idx + 6, base_idx + 7});
    // Bottom
    indices.push_back({base_idx + 4, base_idx + 5, base_idx + 1});
    indices.push_back({base_idx + 4, base_idx + 1, base_idx + 0});
}

struct BenchmarkConfig
{
    std::string name;
    unsigned int num_photons;
    unsigned int max_bounces;
    float absorption_length;  // mm
    float scattering_length;  // mm
    float world_size;         // mm
    int geometry_complexity;  // sphere subdivisions
    int num_objects;
};

struct BenchmarkResult
{
    std::string config_name;
    std::string gpu_name;
    double bvh_build_time_ms;
    double trace_time_ms;
    double photons_per_second;
    unsigned int total_bounces;
    unsigned int detected_photons;
    unsigned int absorbed_photons;
    unsigned int escaped_photons;
    size_t geometry_triangles;
    size_t geometry_memory_bytes;
};

class PhotonBenchmark
{
public:
    PhotonBenchmark();
    ~PhotonBenchmark();
    
    void initialize();
    BenchmarkResult run(const BenchmarkConfig& config);
    void printResults(const std::vector<BenchmarkResult>& results);
    
private:
    void createContext();
    void createModule();
    void createProgramGroups();
    void createPipeline();
    
    OptixDeviceContext context = nullptr;
    OptixModule module = nullptr;
    OptixPipelineCompileOptions pipeline_compile_options = {};
    OptixPipeline pipeline = nullptr;
    
    OptixProgramGroup raygen_pg = nullptr;
    OptixProgramGroup miss_pg = nullptr;
    OptixProgramGroup hitgroup_pg = nullptr;
    
    CUdeviceptr d_raygen_record = 0;
    CUdeviceptr d_miss_record = 0;
    CUdeviceptr d_hitgroup_record = 0;
    
    std::string gpu_name;
};

PhotonBenchmark::PhotonBenchmark()
{
}

PhotonBenchmark::~PhotonBenchmark()
{
    if (d_raygen_record) cudaFree(reinterpret_cast<void*>(d_raygen_record));
    if (d_miss_record) cudaFree(reinterpret_cast<void*>(d_miss_record));
    if (d_hitgroup_record) cudaFree(reinterpret_cast<void*>(d_hitgroup_record));
    
    if (hitgroup_pg) optixProgramGroupDestroy(hitgroup_pg);
    if (miss_pg) optixProgramGroupDestroy(miss_pg);
    if (raygen_pg) optixProgramGroupDestroy(raygen_pg);
    if (pipeline) optixPipelineDestroy(pipeline);
    if (module) optixModuleDestroy(module);
    if (context) optixDeviceContextDestroy(context);
}

void PhotonBenchmark::createContext()
{
    CUDA_CHECK(cudaFree(0)); // Initialize CUDA
    
    CUcontext cu_ctx = 0;
    OPTIX_CHECK(optixInit());
    
    OptixDeviceContextOptions options = {};
    options.logCallbackFunction = &context_log_cb;
    options.logCallbackLevel = 4;
    
    OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &options, &context));
    
    // Get GPU name
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    gpu_name = prop.name;
    
    std::cout << "GPU: " << gpu_name << std::endl;
    std::cout << "Compute capability: " << prop.major << "." << prop.minor << std::endl;
    std::cout << "SM count: " << prop.multiProcessorCount << std::endl;
    std::cout << "Memory: " << (prop.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
    std::cout << std::endl;
}

void PhotonBenchmark::createModule()
{
    OptixModuleCompileOptions module_compile_options = {};
    module_compile_options.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    module_compile_options.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_3;
    module_compile_options.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE;
    
    pipeline_compile_options.usesMotionBlur = false;
    pipeline_compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipeline_compile_options.numPayloadValues = 4;
    pipeline_compile_options.numAttributeValues = 2;
    pipeline_compile_options.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
    pipeline_compile_options.pipelineLaunchParamsVariableName = "params";
    
    // Load PTX
    std::string ptx_path = std::string(PTX_DIR) + "/photon_kernels.optixir";
    std::ifstream file(ptx_path, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open PTX file: " << ptx_path << std::endl;
        exit(1);
    }
    
    std::vector<char> ptx_data((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
    
    char log[2048];
    size_t log_size = sizeof(log);
    
    OPTIX_CHECK(optixModuleCreate(
        context,
        &module_compile_options,
        &pipeline_compile_options,
        ptx_data.data(),
        ptx_data.size(),
        log, &log_size,
        &module
    ));
    
    if (log_size > 1) std::cout << "Module log: " << log << std::endl;
}

void PhotonBenchmark::createProgramGroups()
{
    char log[2048];
    size_t log_size;
    
    // Ray generation
    OptixProgramGroupOptions pg_options = {};
    OptixProgramGroupDesc raygen_desc = {};
    raygen_desc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygen_desc.raygen.module = module;
    raygen_desc.raygen.entryFunctionName = "__raygen__photon_trace";
    
    log_size = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(context, &raygen_desc, 1, &pg_options,
                                        log, &log_size, &raygen_pg));
    
    // Miss
    OptixProgramGroupDesc miss_desc = {};
    miss_desc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    miss_desc.miss.module = module;
    miss_desc.miss.entryFunctionName = "__miss__photon";
    
    log_size = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(context, &miss_desc, 1, &pg_options,
                                        log, &log_size, &miss_pg));
    
    // Hit group
    OptixProgramGroupDesc hitgroup_desc = {};
    hitgroup_desc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitgroup_desc.hitgroup.moduleCH = module;
    hitgroup_desc.hitgroup.entryFunctionNameCH = "__closesthit__photon";
    
    log_size = sizeof(log);
    OPTIX_CHECK(optixProgramGroupCreate(context, &hitgroup_desc, 1, &pg_options,
                                        log, &log_size, &hitgroup_pg));
}

void PhotonBenchmark::createPipeline()
{
    OptixProgramGroup program_groups[] = { raygen_pg, miss_pg, hitgroup_pg };
    
    OptixPipelineLinkOptions pipeline_link_options = {};
    pipeline_link_options.maxTraceDepth = 1;
    
    char log[2048];
    size_t log_size = sizeof(log);
    
    OPTIX_CHECK(optixPipelineCreate(
        context,
        &pipeline_compile_options,
        &pipeline_link_options,
        program_groups,
        sizeof(program_groups) / sizeof(program_groups[0]),
        log, &log_size,
        &pipeline
    ));
    
    // Set stack sizes
    OptixStackSizes stack_sizes = {};
    for (auto& pg : program_groups) {
        OPTIX_CHECK(optixUtilAccumulateStackSizes(pg, &stack_sizes, pipeline));
    }
    
    uint32_t direct_callable_stack_size_from_traversal;
    uint32_t direct_callable_stack_size_from_state;
    uint32_t continuation_stack_size;
    OPTIX_CHECK(optixUtilComputeStackSizes(
        &stack_sizes, 1, 0, 0,
        &direct_callable_stack_size_from_traversal,
        &direct_callable_stack_size_from_state,
        &continuation_stack_size
    ));
    
    OPTIX_CHECK(optixPipelineSetStackSize(
        pipeline,
        direct_callable_stack_size_from_traversal,
        direct_callable_stack_size_from_state,
        continuation_stack_size,
        1
    ));
}

void PhotonBenchmark::initialize()
{
    createContext();
    createModule();
    createProgramGroups();
    createPipeline();
}

BenchmarkResult PhotonBenchmark::run(const BenchmarkConfig& config)
{
    BenchmarkResult result;
    result.config_name = config.name;
    result.gpu_name = gpu_name;
    
    std::cout << "Running benchmark: " << config.name << std::endl;
    std::cout << "  Photons: " << config.num_photons << std::endl;
    std::cout << "  Max bounces: " << config.max_bounces << std::endl;
    std::cout << "  Geometry complexity: " << config.geometry_complexity << " subdivisions" << std::endl;
    std::cout << "  Objects: " << config.num_objects << std::endl;
    
    // Generate geometry
    std::vector<float3> vertices;
    std::vector<uint3> indices;
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> pos_dist(-config.world_size * 0.4f, config.world_size * 0.4f);
    std::uniform_real_distribution<float> size_dist(config.world_size * 0.02f, config.world_size * 0.1f);
    
    for (int i = 0; i < config.num_objects; i++) {
        float3 center = {pos_dist(rng), pos_dist(rng), pos_dist(rng)};
        float radius = size_dist(rng);
        generate_sphere_mesh(vertices, indices, center, radius, config.geometry_complexity);
    }
    
    // Add outer boundary box
    float bs = config.world_size * 0.9f;
    generate_box_mesh(vertices, indices, {-bs, -bs, -bs}, {bs, bs, bs});
    
    result.geometry_triangles = indices.size();
    std::cout << "  Triangles: " << result.geometry_triangles << std::endl;
    
    // Upload geometry to GPU
    CUdeviceptr d_vertices, d_indices;
    size_t vertices_size = vertices.size() * sizeof(float3);
    size_t indices_size = indices.size() * sizeof(uint3);
    
    result.geometry_memory_bytes = vertices_size + indices_size;
    
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_vertices), vertices_size));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_indices), indices_size));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_vertices), vertices.data(), vertices_size, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_indices), indices.data(), indices_size, cudaMemcpyHostToDevice));
    
    // Build acceleration structure
    OptixAccelBuildOptions accel_options = {};
    accel_options.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION | OPTIX_BUILD_FLAG_PREFER_FAST_TRACE;
    accel_options.operation = OPTIX_BUILD_OPERATION_BUILD;
    
    OptixBuildInput build_input = {};
    build_input.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    
    uint32_t triangle_flags[] = { OPTIX_GEOMETRY_FLAG_NONE };
    
    build_input.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    build_input.triangleArray.numVertices = vertices.size();
    build_input.triangleArray.vertexBuffers = &d_vertices;
    build_input.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    build_input.triangleArray.numIndexTriplets = indices.size();
    build_input.triangleArray.indexBuffer = d_indices;
    build_input.triangleArray.flags = triangle_flags;
    build_input.triangleArray.numSbtRecords = 1;
    
    OptixAccelBufferSizes gas_buffer_sizes;
    OPTIX_CHECK(optixAccelComputeMemoryUsage(context, &accel_options, &build_input, 1, &gas_buffer_sizes));
    
    CUdeviceptr d_temp_buffer, d_gas_output;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_temp_buffer), gas_buffer_sizes.tempSizeInBytes));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_gas_output), gas_buffer_sizes.outputSizeInBytes));
    
    // Time BVH build
    cudaEvent_t start, stop;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    
    CUDA_CHECK(cudaEventRecord(start));
    
    OptixTraversableHandle gas_handle;
    OPTIX_CHECK(optixAccelBuild(
        context,
        0, // stream
        &accel_options,
        &build_input,
        1,
        d_temp_buffer,
        gas_buffer_sizes.tempSizeInBytes,
        d_gas_output,
        gas_buffer_sizes.outputSizeInBytes,
        &gas_handle,
        nullptr, 0
    ));
    
    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK(cudaEventSynchronize(stop));
    
    float bvh_time_ms;
    CUDA_CHECK(cudaEventElapsedTime(&bvh_time_ms, start, stop));
    result.bvh_build_time_ms = bvh_time_ms;
    
    std::cout << "  BVH build time: " << bvh_time_ms << " ms" << std::endl;
    
    cudaFree(reinterpret_cast<void*>(d_temp_buffer));
    
    // Allocate photon data
    CUdeviceptr d_positions, d_directions, d_wavelengths, d_flags, d_times;
    CUdeviceptr d_hit_counts, d_bounce_counts;
    
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_positions), config.num_photons * sizeof(float3)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_directions), config.num_photons * sizeof(float3)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_wavelengths), config.num_photons * sizeof(float)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_flags), config.num_photons * sizeof(unsigned int)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_times), config.num_photons * sizeof(float)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_hit_counts), sizeof(unsigned int)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_bounce_counts), config.num_photons * sizeof(unsigned int)));
    
    // Initialize photon data
    std::vector<float3> positions(config.num_photons);
    std::vector<float3> directions(config.num_photons);
    std::vector<float> wavelengths(config.num_photons);
    std::vector<unsigned int> flags(config.num_photons, PHOTON_ALIVE);
    std::vector<float> times(config.num_photons, 0.0f);
    
    std::uniform_real_distribution<float> dir_dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> wl_dist(380.0f, 700.0f);
    
    for (unsigned int i = 0; i < config.num_photons; i++) {
        // Start photons at center
        positions[i] = {0.0f, 0.0f, 0.0f};
        
        // Random direction (isotropic)
        float3 dir;
        do {
            dir = {dir_dist(rng), dir_dist(rng), dir_dist(rng)};
        } while (dir.x*dir.x + dir.y*dir.y + dir.z*dir.z > 1.0f);
        float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        directions[i] = {dir.x/len, dir.y/len, dir.z/len};
        
        wavelengths[i] = wl_dist(rng);
    }
    
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_positions), positions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_directions), directions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_wavelengths), wavelengths.data(), config.num_photons * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_flags), flags.data(), config.num_photons * sizeof(unsigned int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_times), times.data(), config.num_photons * sizeof(float), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_hit_counts), 0, sizeof(unsigned int)));
    CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_bounce_counts), 0, config.num_photons * sizeof(unsigned int)));
    
    // Setup launch parameters
    PhotonParams params;
    params.handle = gas_handle;
    params.photon_positions = reinterpret_cast<float3*>(d_positions);
    params.photon_directions = reinterpret_cast<float3*>(d_directions);
    params.photon_wavelengths = reinterpret_cast<float*>(d_wavelengths);
    params.photon_flags = reinterpret_cast<unsigned int*>(d_flags);
    params.photon_times = reinterpret_cast<float*>(d_times);
    params.hit_counts = reinterpret_cast<unsigned int*>(d_hit_counts);
    params.bounce_counts = reinterpret_cast<unsigned int*>(d_bounce_counts);
    params.num_photons = config.num_photons;
    params.max_bounces = config.max_bounces;
    params.world_size = config.world_size;
    params.absorption_length = config.absorption_length;
    params.scattering_length = config.scattering_length;
    params.refractive_index = 1.38f;  // LAr
    params.seed = 12345;
    
    CUdeviceptr d_params;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(PhotonParams)));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(PhotonParams), cudaMemcpyHostToDevice));
    
    // Setup SBT
    RayGenSbtRecord rg_sbt;
    OPTIX_CHECK(optixSbtRecordPackHeader(raygen_pg, &rg_sbt));
    
    MissSbtRecord ms_sbt;
    ms_sbt.data.bg_color = {0.0f, 0.0f, 0.0f};
    OPTIX_CHECK(optixSbtRecordPackHeader(miss_pg, &ms_sbt));
    
    HitGroupSbtRecord hg_sbt;
    hg_sbt.data.vertices = reinterpret_cast<float3*>(d_vertices);
    hg_sbt.data.indices = reinterpret_cast<uint3*>(d_indices);
    hg_sbt.data.material_id = 0;
    hg_sbt.data.reflectivity = 0.9f;
    OPTIX_CHECK(optixSbtRecordPackHeader(hitgroup_pg, &hg_sbt));
    
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_raygen_record), sizeof(RayGenSbtRecord)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_miss_record), sizeof(MissSbtRecord)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_hitgroup_record), sizeof(HitGroupSbtRecord)));
    
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_raygen_record), &rg_sbt, sizeof(RayGenSbtRecord), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_miss_record), &ms_sbt, sizeof(MissSbtRecord), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_hitgroup_record), &hg_sbt, sizeof(HitGroupSbtRecord), cudaMemcpyHostToDevice));
    
    OptixShaderBindingTable sbt = {};
    sbt.raygenRecord = d_raygen_record;
    sbt.missRecordBase = d_miss_record;
    sbt.missRecordStrideInBytes = sizeof(MissSbtRecord);
    sbt.missRecordCount = 1;
    sbt.hitgroupRecordBase = d_hitgroup_record;
    sbt.hitgroupRecordStrideInBytes = sizeof(HitGroupSbtRecord);
    sbt.hitgroupRecordCount = 1;
    
    // Warm-up run
    OPTIX_CHECK(optixLaunch(pipeline, 0, d_params, sizeof(PhotonParams), &sbt, config.num_photons, 1, 1));
    CUDA_CHECK(cudaDeviceSynchronize());
    
    // Reset photon data
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_positions), positions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_directions), directions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_flags), flags.data(), config.num_photons * sizeof(unsigned int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_hit_counts), 0, sizeof(unsigned int)));
    CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_bounce_counts), 0, config.num_photons * sizeof(unsigned int)));
    CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(PhotonParams), cudaMemcpyHostToDevice));
    
    // Timed run
    const int num_runs = 5;
    double total_time = 0.0;
    
    for (int run = 0; run < num_runs; run++) {
        CUDA_CHECK(cudaEventRecord(start));
        
        OPTIX_CHECK(optixLaunch(pipeline, 0, d_params, sizeof(PhotonParams), &sbt, config.num_photons, 1, 1));
        
        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK(cudaEventSynchronize(stop));
        
        float trace_time_ms;
        CUDA_CHECK(cudaEventElapsedTime(&trace_time_ms, start, stop));
        total_time += trace_time_ms;
        
        // Reset for next run
        if (run < num_runs - 1) {
            CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_positions), positions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_directions), directions.data(), config.num_photons * sizeof(float3), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_flags), flags.data(), config.num_photons * sizeof(unsigned int), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_hit_counts), 0, sizeof(unsigned int)));
            CUDA_CHECK(cudaMemset(reinterpret_cast<void*>(d_bounce_counts), 0, config.num_photons * sizeof(unsigned int)));
            CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(PhotonParams), cudaMemcpyHostToDevice));
        }
    }
    
    result.trace_time_ms = total_time / num_runs;
    result.photons_per_second = (config.num_photons / (result.trace_time_ms / 1000.0));
    
    std::cout << "  Trace time (avg): " << result.trace_time_ms << " ms" << std::endl;
    std::cout << "  Photons/sec: " << std::scientific << result.photons_per_second << std::fixed << std::endl;
    
    // Read back results
    std::vector<unsigned int> final_flags(config.num_photons);
    std::vector<unsigned int> bounce_counts(config.num_photons);
    unsigned int hit_count;
    
    CUDA_CHECK(cudaMemcpy(final_flags.data(), reinterpret_cast<void*>(d_flags), config.num_photons * sizeof(unsigned int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(bounce_counts.data(), reinterpret_cast<void*>(d_bounce_counts), config.num_photons * sizeof(unsigned int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&hit_count, reinterpret_cast<void*>(d_hit_counts), sizeof(unsigned int), cudaMemcpyDeviceToHost));
    
    result.detected_photons = 0;
    result.absorbed_photons = 0;
    result.escaped_photons = 0;
    result.total_bounces = 0;
    
    for (unsigned int i = 0; i < config.num_photons; i++) {
        switch (final_flags[i]) {
            case PHOTON_DETECTED: result.detected_photons++; break;
            case PHOTON_ABSORBED: result.absorbed_photons++; break;
            case PHOTON_ESCAPED: result.escaped_photons++; break;
        }
        result.total_bounces += bounce_counts[i];
    }
    
    std::cout << "  Detected: " << result.detected_photons << std::endl;
    std::cout << "  Absorbed: " << result.absorbed_photons << std::endl;
    std::cout << "  Escaped: " << result.escaped_photons << std::endl;
    std::cout << "  Total bounces: " << result.total_bounces << std::endl;
    std::cout << "  Avg bounces/photon: " << (float)result.total_bounces / config.num_photons << std::endl;
    std::cout << std::endl;
    
    // Cleanup
    cudaFree(reinterpret_cast<void*>(d_vertices));
    cudaFree(reinterpret_cast<void*>(d_indices));
    cudaFree(reinterpret_cast<void*>(d_gas_output));
    cudaFree(reinterpret_cast<void*>(d_positions));
    cudaFree(reinterpret_cast<void*>(d_directions));
    cudaFree(reinterpret_cast<void*>(d_wavelengths));
    cudaFree(reinterpret_cast<void*>(d_flags));
    cudaFree(reinterpret_cast<void*>(d_times));
    cudaFree(reinterpret_cast<void*>(d_hit_counts));
    cudaFree(reinterpret_cast<void*>(d_bounce_counts));
    cudaFree(reinterpret_cast<void*>(d_params));
    
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    
    return result;
}

void PhotonBenchmark::printResults(const std::vector<BenchmarkResult>& results)
{
    std::cout << "\n========== BENCHMARK SUMMARY ==========" << std::endl;
    std::cout << "GPU: " << gpu_name << std::endl;
    std::cout << std::endl;
    
    std::cout << std::left << std::setw(30) << "Configuration"
              << std::right << std::setw(15) << "BVH Build (ms)"
              << std::setw(15) << "Trace (ms)"
              << std::setw(18) << "Photons/sec"
              << std::setw(12) << "Triangles"
              << std::endl;
    std::cout << std::string(90, '-') << std::endl;
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(30) << r.config_name
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << r.bvh_build_time_ms
                  << std::setw(15) << r.trace_time_ms
                  << std::setw(18) << std::scientific << std::setprecision(2) << r.photons_per_second
                  << std::setw(12) << std::fixed << std::setprecision(0) << r.geometry_triangles
                  << std::endl;
    }
    
    std::cout << std::endl;
    
    // Output JSON for comparison
    std::ofstream json_file("benchmark_results.json");
    json_file << "{\n";
    json_file << "  \"gpu\": \"" << gpu_name << "\",\n";
    json_file << "  \"results\": [\n";
    for (size_t i = 0; i < results.size(); i++) {
        const auto& r = results[i];
        json_file << "    {\n";
        json_file << "      \"config\": \"" << r.config_name << "\",\n";
        json_file << "      \"bvh_build_ms\": " << r.bvh_build_time_ms << ",\n";
        json_file << "      \"trace_ms\": " << r.trace_time_ms << ",\n";
        json_file << "      \"photons_per_sec\": " << r.photons_per_second << ",\n";
        json_file << "      \"triangles\": " << r.geometry_triangles << ",\n";
        json_file << "      \"total_bounces\": " << r.total_bounces << ",\n";
        json_file << "      \"detected\": " << r.detected_photons << ",\n";
        json_file << "      \"absorbed\": " << r.absorbed_photons << ",\n";
        json_file << "      \"escaped\": " << r.escaped_photons << "\n";
        json_file << "    }" << (i < results.size() - 1 ? "," : "") << "\n";
    }
    json_file << "  ]\n";
    json_file << "}\n";
    json_file.close();
    
    std::cout << "Results saved to benchmark_results.json" << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "OptiX Photon Simulation Benchmark" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    PhotonBenchmark benchmark;
    benchmark.initialize();
    
    std::vector<BenchmarkConfig> configs = {
        // Basic test - few photons, simple geometry
        {
            "Simple (1M photons, 100 spheres)",
            1000000,    // num_photons
            100,        // max_bounces
            10000.0f,   // absorption_length (mm)
            1000.0f,    // scattering_length (mm)
            5000.0f,    // world_size (mm)
            2,          // geometry_complexity (subdivisions)
            100         // num_objects
        },
        
        // High scatter (Opticks-like LAr)
        {
            "High Scatter (1M photons, LAr-like)",
            1000000,
            200,
            20000.0f,   // long absorption
            200.0f,     // short scattering (Rayleigh in LAr)
            5000.0f,
            2,
            100
        },
        
        // Complex geometry
        {
            "Complex Geometry (1M, 500 spheres)",
            1000000,
            100,
            10000.0f,
            1000.0f,
            5000.0f,
            3,          // more subdivisions
            500         // more objects
        },
        
        // Very complex geometry (detector-scale)
        {
            "Detector Scale (1M, 1000 spheres)",
            1000000,
            150,
            15000.0f,
            500.0f,
            10000.0f,
            3,
            1000
        },
        
        // High photon count
        {
            "High Photon (10M, moderate geom)",
            10000000,
            100,
            10000.0f,
            1000.0f,
            5000.0f,
            2,
            200
        },
        
        // Extreme bounces (stress test)
        {
            "Many Bounces (1M, 500 bounces)",
            1000000,
            500,
            50000.0f,
            100.0f,     // very short scatter
            5000.0f,
            2,
            100
        }
    };
    
    std::vector<BenchmarkResult> results;
    
    for (const auto& config : configs) {
        results.push_back(benchmark.run(config));
    }
    
    benchmark.printResults(results);
    
    return 0;
}
