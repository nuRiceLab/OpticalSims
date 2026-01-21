A benchmark tool designed to compare GPU ray tracing performance for optical photon simulations, specifically useful for comparing compute GPUs vs RTX GPUs in Opticks-like workloads.

## What it measures

This benchmark simulates optical photon propagation through a medium with:
- **Rayleigh scattering** (configurable scattering length)
- **Absorption** (configurable absorption length)  
- **Fresnel reflection/refraction** at surfaces
- **Multiple bounces** per photon

### Benchmark configurations

| Configuration | Description | Use case |
|--------------|-------------|----------|
| Simple | 1M photons, 100 spheres, 100 bounces | Baseline BVH traversal |
| High Scatter | 1M photons, short scattering length | LAr-like optical properties |
| Complex Geometry | 1M photons, 500 high-detail spheres | Realistic detector complexity |
| Detector Scale | 1M photons, 1000 objects | Full detector simulation |
| High Photon | 10M photons, moderate geometry | Throughput stress test |
| Many Bounces | 1M photons, 500 bounces max | Ray coherence stress test |

### Metrics reported

- **BVH build time**: Time to construct acceleration structure
- **Trace time**: Time for photon propagation kernel
- **Photons/second**: Primary throughput metric
- **Total bounces**: Measures ray coherence behavior
- **Detection/absorption/escape rates**: Validates physics

### Prerequisites

- CUDA Toolkit 11.4+
- OptiX 8.1.0 SDK
- CMake 3.16+
- C++17 compiler

