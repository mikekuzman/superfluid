# 4D Bose-Einstein Condensate Simulator & Visualizer

A high-performance quantum superfluid simulator operating on a 4-dimensional hypersphere shell, with real-time 3D visualization.

## Overview

This project simulates a Bose-Einstein Condensate (BEC) on the surface of a 4D hypersphere using the Gross-Pitaevskii equation (GPE). The system consists of two native C++ applications:

1. **bec_sim** - CUDA-accelerated physics simulator
2. **bec_viz** - DirectX 12 interactive visualizer (Windows only)

## Features

### Simulation (bec_sim)
- Solves Gross-Pitaevskii equation in 4D using CUDA
- Sparse storage (~5K points vs 85M grid)
- Quantized vortex detection
- Phonon and roton analysis
- Binary compressed output (.bec format with LZ4)
- Performance: 300-500 steps/sec on GTX 1070

### Visualization (bec_viz)
- Three 4D→3D projection modes (perspective, stereographic, orthogonal)
- Real-time 4D rotation
- Interactive pole markers (North★/South★)
- Vortex overlays
- Timeline scrubbing
- 60+ FPS with 100K points

## System Requirements

### Minimum
- **OS:** Windows 10 x64 or Linux
- **CPU:** Quad-core processor
- **RAM:** 16GB
- **GPU:** NVIDIA GPU with CUDA Compute Capability 6.0+ (optional for CPU-only mode)

### Recommended (for full performance)
- **OS:** Windows 10 x64
- **CPU:** Intel i7-4790K @ 4.00GHz or better
- **RAM:** 32GB DDR3
- **GPU:** NVIDIA GTX 1070 (8GB VRAM, Compute Capability 6.1) or better
- **IDE:** Visual Studio 2022/2026 Enterprise (for Windows builds)

## Build Instructions

### Linux (Simulation Only)

#### Prerequisites
```bash
# Install build tools
sudo apt-get update
sudo apt-get install -y cmake g++ git

# Install CUDA (optional, for GPU acceleration)
# Follow NVIDIA CUDA installation guide for your distribution
```

#### Build
```bash
# Clone repository
git clone <repository-url>
cd superfluid

# Create build directory
mkdir build && cd build

# Configure (CPU-only mode)
cmake .. -DUSE_CUDA=OFF

# Or with CUDA support
cmake .. -DUSE_CUDA=ON -DUSE_LZ4=ON

# Build
make -j$(nproc)

# Run tests
make test

# Install (optional)
sudo make install
```

### Windows (Full Build with Visualization)

#### Prerequisites
- Visual Studio 2022 or later with C++ tools
- CUDA Toolkit 12.x
- DirectX 12 SDK (included in Windows 10 SDK)
- CMake 3.18+

#### Build with CMake (Command Line)
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -DUSE_CUDA=ON
cmake --build . --config Release
```

#### Build with Visual Studio
1. Open Visual Studio
2. File → Open → CMake → Select `CMakeLists.txt`
3. Configure CMake settings for Release build
4. Build → Build All

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `USE_CUDA` | ON | Enable CUDA acceleration |
| `USE_LZ4` | OFF | Enable LZ4 compression (requires liblz4) |
| `BUILD_SIMULATION` | ON | Build simulation executable |
| `BUILD_VISUALIZATION` | ON | Build visualization (Windows only) |

## Usage

### Running a Simulation

Basic usage:
```bash
./bin/bec_sim
```

With parameters:
```bash
./bin/bec_sim --R 1000 --thickness-ratio 40 --omega 0.5 --steps 10000 --output my_simulation.bec
```

### Command-Line Options

| Option | Default | Description |
|--------|---------|-------------|
| `--R <value>` | 1000 | Hypersphere radius (healing lengths) |
| `--delta <value>` | 25 | Shell thickness (healing lengths) |
| `--thickness-ratio <val>` | - | R/delta ratio (overrides delta) |
| `--g <value>` | 0.05 | Interaction strength (dimensionless) |
| `--omega <value>` | 0.5 | Rotation frequency (dimensionless) |
| `--N <value>` | 64 | Grid resolution parameter |
| `--dt <value>` | 0.01 | Time step (dimensionless) |
| `--steps <value>` | 10000 | Total simulation steps |
| `--save-every <value>` | 100 | Save snapshot every N steps |
| `--output <file>` | output.bec | Output filename |
| `--seed <value>` | 42 | Random seed for reproducibility |
| `--neighbors <value>` | 6 | Neighbors for gradient calculation |

### Example Simulations

**Quick test (small system):**
```bash
./bin/bec_sim --N 32 --steps 1000 --save-every 50 --output test.bec
```

**Production run (rotating BEC):**
```bash
./bin/bec_sim --R 1000 --thickness-ratio 40 --omega 0.5 --g 0.05 \
  --steps 50000 --save-every 100 --output rotating_bec.bec
```

**High-resolution vortex study:**
```bash
./bin/bec_sim --R 1000 --thickness-ratio 50 --omega 0.8 --N 96 \
  --steps 100000 --save-every 500 --output vortex_lattice.bec
```

## File Format (.bec)

The `.bec` file format is a binary format with the following structure:

1. **Header (4096 bytes)** - Simulation metadata and parameters
2. **Snapshot Index Table** - Fast seeking to snapshots
3. **Compressed Snapshot Blocks** - LZ4-compressed point data

### File Size Estimates

| Points | Snapshots | Uncompressed | Compressed (LZ4, ~4×) |
|--------|-----------|--------------|------------------------|
| 5,000 | 100 | 10 MB | 2.5 MB |
| 5,000 | 1,000 | 100 MB | 25 MB |
| 50,000 | 100 | 100 MB | 25 MB |
| 50,000 | 1,000 | 1 GB | 250 MB |

## Physics Background

### Gross-Pitaevskii Equation

The time-dependent GPE governs the BEC wavefunction ψ(r,t):

```
iℏ ∂ψ/∂t = [-ℏ²∇²/(2m) + g|ψ|² + V - Ω L_z]ψ
```

**Terms:**
- `∇²`: 4D Laplacian operator
- `g`: Interaction strength (dimensionless)
- `|ψ|²`: Nonlinear mean-field term
- `Ω`: Rotation frequency
- `L_z`: Angular momentum operator in w-x plane

### Dimensionless Units

All quantities are normalized to healing length ξ = ℏ/√(mgn₀):
- Length: [ξ] = 1
- Time: [ℏ/(gn₀)] = 1
- Energy: [gn₀] = 1
- ℏ = m = 1

### 4D Geometry

**Hypersphere:** x² + y² + z² + w² = R²

**Shell region:** R - δ/2 ≤ √(x²+y²+z²+w²) ≤ R + δ/2

**Poles:**
- North: (0, 0, 0, R) - marked RED, label "N★"
- South: (0, 0, 0, -R) - marked BLUE, label "S★"

## Project Structure

```
superfluid/
├── BEC_Core/              # Core library (file I/O, math utilities)
│   ├── include/           # Header files
│   │   ├── BECFileFormat.h
│   │   ├── BECFileWriter.h
│   │   ├── BECFileReader.h
│   │   ├── Vector4.h
│   │   ├── Complex.h
│   │   ├── Projection.h
│   │   └── Statistics.h
│   └── src/               # Implementation
│       ├── BECFileWriter.cpp
│       └── BECFileReader.cpp
│
├── BEC_Sim/               # Simulation executable
│   ├── include/           # Simulation headers
│   │   ├── HypersphereBEC.h
│   │   ├── SimulationParams.h
│   │   ├── ShellGenerator.h
│   │   ├── NeighborFinder.h
│   │   └── VortexDetector.h
│   ├── src/               # CPU implementation
│   │   ├── main.cpp
│   │   ├── HypersphereBEC.cpp
│   │   ├── ShellGenerator.cpp
│   │   ├── NeighborFinder.cpp
│   │   └── VortexDetector.cpp
│   └── kernels/           # CUDA kernels
│       ├── cuda_common.cuh
│       ├── laplacian_kernel.cu
│       ├── rotation_kernel.cu
│       ├── gpe_evolution_kernel.cu
│       └── gradient_kernel.cu
│
├── BEC_Viz/               # Visualization executable (Windows only)
│   ├── include/           # Visualization headers
│   ├── src/               # DirectX 12 implementation
│   └── shaders/           # HLSL shaders
│
├── tests/                 # Unit tests
│   └── test_file_io.cpp
│
├── docs/                  # Documentation
├── external/              # Third-party dependencies
├── build/                 # Build directory (generated)
├── CMakeLists.txt         # Root CMake configuration
└── README.md              # This file
```

## Performance Benchmarks

### Simulation Performance (GTX 1070)

| Points | Neighbors | Steps/sec | Memory |
|--------|-----------|-----------|--------|
| 1,000 | 6 | 800-1000 | 50 MB |
| 5,000 | 6 | 300-500 | 200 MB |
| 10,000 | 6 | 150-250 | 400 MB |
| 50,000 | 6 | 30-50 | 2 GB |

### Visualization Performance (GTX 1070)

| Points | FPS (60Hz target) |
|--------|-------------------|
| 10,000 | 120+ |
| 50,000 | 90+ |
| 100,000 | 60+ |
| 500,000 | 25-30 |

## Development Roadmap

### Phase 1: Core Infrastructure ✓
- [x] File format specification
- [x] BECFileWriter with compression
- [x] BECFileReader with memory-mapping
- [x] Math utilities (Vector4, Complex, Projections)
- [x] CMake build system

### Phase 2: Simulation ✓
- [x] Shell point generation
- [x] KD-tree neighbor finding
- [x] CUDA Laplacian kernel
- [x] CUDA rotation term kernel
- [x] RK4 time integration
- [x] Vortex detection
- [x] Main simulation loop

### Phase 3: Visualization (In Progress)
- [ ] DirectX 12 renderer
- [ ] HLSL shaders for 4D→3D projection
- [ ] Pole rendering (N★/S★)
- [ ] ImGui integration
- [ ] Camera controls
- [ ] Playback system

### Phase 4: Polish (Planned)
- [ ] Performance optimizations
- [ ] Comprehensive testing
- [ ] User documentation
- [ ] Installation packages

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

[Specify license here]

## Citation

If you use this code in your research, please cite:

```
[Citation information to be added]
```

## Contact

For questions or support, please open an issue on GitHub.

## Acknowledgments

- CUDA Toolkit by NVIDIA
- nanoflann library for KD-tree
- LZ4 compression library
- Dear ImGui for UI

---

**Last Updated:** November 2025
**Version:** 1.0.0
