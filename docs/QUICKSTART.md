# Quick Start Guide

## 5-Minute Quick Start

### 1. Build the Simulator

```bash
cd superfluid
mkdir build && cd build
cmake .. -DUSE_CUDA=OFF  # Use ON if you have CUDA
make -j4
```

### 2. Run Your First Simulation

```bash
./bin/bec_sim --N 32 --steps 1000 --output first_simulation.bec
```

This will:
- Create a small system (~500 points)
- Run 1000 time steps (~5-10 seconds)
- Save 10 snapshots to `first_simulation.bec`

### 3. Check the Output

```bash
ls -lh first_simulation.bec
```

You should see a file of ~1-2 MB.

## Understanding the Parameters

### Key Parameters

**Geometry:**
- `--R 1000`: Hypersphere radius (keep this constant)
- `--thickness-ratio 40`: Shell thickness = R/40 = 25

**Physics:**
- `--g 0.05`: Interaction strength (0.01 = weak, 0.1 = strong)
- `--omega 0.5`: Rotation speed (0 = no rotation, 1 = fast)

**Computational:**
- `--N 64`: Resolution (32 = ~500 points, 64 = ~5000 points, 96 = ~20000 points)
- `--dt 0.01`: Time step (smaller = more accurate but slower)
- `--steps 10000`: How many steps to simulate
- `--save-every 100`: Save snapshot every 100 steps

## Example Workflows

### Study Vortex Formation

Run with increasing rotation:

```bash
# No rotation (ground state)
./bin/bec_sim --omega 0.0 --steps 5000 --output vortex_omega0.bec

# Slow rotation
./bin/bec_sim --omega 0.3 --steps 10000 --output vortex_omega03.bec

# Fast rotation (vortex lattice)
./bin/bec_sim --omega 0.8 --steps 20000 --output vortex_omega08.bec
```

### High-Resolution Production Run

For publication-quality results:

```bash
./bin/bec_sim \
  --R 1000 \
  --thickness-ratio 50 \
  --N 96 \
  --omega 0.5 \
  --g 0.05 \
  --dt 0.005 \
  --steps 100000 \
  --save-every 500 \
  --output production_run.bec
```

**Warning:** This will take 1-2 hours and produce a ~500 MB file.

### Quick Test Suite

```bash
# Test 1: Minimal system (30 seconds)
./bin/bec_sim --N 16 --steps 500 --output test_small.bec

# Test 2: Medium system (2 minutes)
./bin/bec_sim --N 48 --steps 2000 --output test_medium.bec

# Test 3: Large system (10 minutes)
./bin/bec_sim --N 80 --steps 5000 --output test_large.bec
```

## Interpreting Results

### Console Output

You'll see:
```
=== Initializing 4D BEC Simulation ===
Parameters:
  R = 1000 (healing lengths)
  delta = 25 (healing lengths)
  ...
Generated 5234 shell points (plus 2 poles = 5236 total)
Finding 6 neighbors for each of 5236 points...
...

=== Starting Simulation ===
Step 0 / 10000 (0%) | t = 0 | Snapshots saved: 1
Step 100 / 10000 (1%) | t = 1 | Snapshots saved: 2
...

=== Simulation Complete ===
Total time: 45.2 seconds
Average speed: 221 steps/sec
```

### Output File

The `.bec` file contains:
- Header with all parameters
- Compressed snapshots of the wavefunction
- Statistics (density, phase, velocity)
- Detected vortices

## Common Issues

### "CUDA error: no CUDA-capable device"

**Solution:** Build with `-DUSE_CUDA=OFF` to use CPU-only mode.

### Simulation is very slow

**Solutions:**
- Reduce `--N` (halving N gives 8× speedup)
- Increase `--dt` (double dt gives 2× speedup, but less accurate)
- Use CUDA if available
- Increase `--save-every` to save less often

### Out of memory

**Solutions:**
- Reduce `--N`
- Reduce `--steps` or increase `--save-every`

### File size too large

**Solutions:**
- Build with `-DUSE_LZ4=ON` for compression
- Increase `--save-every` to save fewer snapshots
- Reduce `--N` to have fewer points

## Next Steps

1. **Explore parameters** - Try different values of g and omega
2. **Visualize results** - Build BEC_Viz on Windows (coming soon)
3. **Analyze vortices** - Use the included analysis tools
4. **Read the docs** - See `docs/PHYSICS.md` for theory

## Getting Help

- Check `README.md` for full documentation
- See `docs/TROUBLESHOOTING.md` for common issues
- Open an issue on GitHub

Happy simulating! 🌌
