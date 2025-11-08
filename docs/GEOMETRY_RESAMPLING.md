# Geometry-Aware Loading & Spatial Resampling

## Overview

The 4D BEC simulator now supports **intelligent initial state loading** with automatic handling of geometry changes. This enables:

- **Fast reloading** when geometry matches (saves neighbor graph)
- **Geometry changes** with physical structure preservation
- **Deterministic noise** from seed for all scenarios

---

## How It Works

### Three Loading Paths

When you use `--init-state`, the simulator automatically chooses the best path:

#### 1. Exact Geometry Match (Fast Path) ⚡

**Triggers when:**
- R, delta, N, and random_seed all match the file

**What happens:**
- Loads coordinates directly from file
- Loads wavefunction directly from file
- **Loads neighbor graph from file** (skips expensive neighbor finding!)

**Time saved:** ~1-2 seconds per load

**Output:**
```
Geometry matches saved state - loading directly...
  Loaded 5000 points with neighbor graph
  Skipped geometry generation and neighbor finding (saved ~1-2 seconds)
```

#### 2. Geometry Change (Spatial Resampling) 🔄

**Triggers when:**
- R, delta, or N differ from file

**What happens:**
- Generates new geometry with new parameters
- Loads old wavefunction from file
- **Spatially resamples** using inverse distance weighted interpolation:
  - Physical structures (vortices, density patterns) preserved
  - New regions get deterministic noise from seed + point_index
- Computes neighbors for new geometry

**Physics preservation:**
- Vortices stay at same 4D locations (if geometry still contains them)
- Density modulations interpolated smoothly
- Phase structure maintained
- Energy approximately conserved

**Output:**
```
Geometry change detected:
  Old: R=1000, delta=25, N=64
  New: R=1500, delta=37.5, N=64
Resampling wavefunction with spatial interpolation...
Resampling complete (5000 points)
Physics preserved via interpolation, new regions use deterministic noise
```

#### 3. Random Initialization (Fallback) 🎲

**Triggers when:**
- No `--init-state` specified
- File fails to load

**What happens:**
- Standard noise initialization: ψ = 1 + ε

---

## Deterministic Noise

**Key innovation:** Noise generation is **fully deterministic** from seed + point_index

```cpp
// For point i:
rng.seed(random_seed + i);
noise_i = rng.normal(0, noise_amplitude);
```

This means:
- **Same seed** → Same random sequence
- **Point index** determines which part of sequence to use
- Changing N (more points) uses **next numbers in sequence**
- Fully reproducible!

---

## Usage Examples

### Example 1: Fast Reload (Same Geometry)

```bash
# Run initial simulation
bec_sim --R 1000 --N 64 --seed 42 --steps 10000 --output phase1.bec

# Continue from final state (FAST - loads neighbors)
bec_sim --init-state phase1.bec \
        --init-snapshot -1 \
        --steps 10000 \
        --output phase2.bec
```

**Result:** Neighbor graph loaded from file, saves ~1-2 seconds

### Example 2: Change Resolution (Upsampling)

```bash
# Low resolution exploration
bec_sim --R 1000 --N 64 --seed 42 --steps 5000 --output low_res.bec

# Upsample to high resolution (2× more points)
bec_sim --init-state low_res.bec \
        --N 128 --seed 42 \
        --steps 10000 \
        --output high_res.bec
```

**Result:** Wavefunction resampled with interpolation, new points use deterministic noise

### Example 3: Change Scale

```bash
# Small system
bec_sim --R 500 --N 64 --seed 42 --steps 5000 --output small.bec

# Scale up system (3× larger radius)
bec_sim --init-state small.bec \
        --R 1500 --N 64 --seed 42 \
        --steps 10000 \
        --output large.bec
```

**Result:** Vortices (if present) scaled to new geometry, physics interpolated

### Example 4: Parameter Sweep from Saved State

```bash
# Create equilibrated state at one omega
bec_sim --omega 0.5 --steps 10000 --output equilibrated.bec

# Sweep higher omegas starting from equilibrated state
bec_sweep \
  --init-state equilibrated.bec \
  --init-snapshot 100 \
  --omega 1.0,1.5,2.0,2.5,3.0 \
  --steps 5000 \
  --output omega_response.csv
```

**Result:** All sweep runs start from same physical state, only changing omega

---

## Technical Details

### Spatial Interpolation Algorithm

For each new point **p**:

1. **Find K=4 nearest neighbors** in old geometry
2. **Check distance** to closest neighbor:
   - If `d < 2×spacing`: Use **inverse distance weighted interpolation**
   - If `d ≥ 2×spacing`: Use **deterministic noise** (new region)

3. **Inverse Distance Weighting:**
   ```
   ψ_new = Σ(ψ_old[i] / d_i) / Σ(1/d_i)
   ```

4. **Deterministic Noise:**
   ```cpp
   rng.seed(random_seed + point_index);
   ψ_new = (1, 0) + noise_amplitude × rng.normal()
   ```

### Neighbor Graph Storage

**File structure:**
```
[Header: 4096 bytes]
  ├─ neighbor_data_offset (where neighbor data starts)
  ├─ neighbor_data_size (bytes)
  └─ has_neighbor_data (flag)
[Neighbor Indices: n_points × n_neighbors × 4 bytes]
[Neighbor Distances: n_points × n_neighbors × 4 bytes]
[Snapshot 0: compressed PointData]
[Snapshot 1: compressed PointData]
...
[Snapshot Index]
```

**Size:** ~240 KB for 5000 points × 6 neighbors

---

## Performance Impact

| Scenario | Time Saved | Memory Cost |
|----------|-----------|-------------|
| Exact match reload | ~1-2 seconds | +240 KB file |
| Geometry change | ~0 seconds | +240 KB file |
| Random init | N/A | +240 KB file |

**Conclusion:** Small file size increase, significant time savings on reload

---

## Reproducibility Guarantees

✅ **Same seed** → Same noise sequence
✅ **Same seed + N** → First N noise values identical
✅ **Same seed + geometry** → Exact reproduction
✅ **Same seed + different N** → Deterministic extension

---

## Limitations & Caveats

### When Interpolation Works Well:
- ✅ Modest geometry changes (R: 1000→1500, N: 64→128)
- ✅ Vortices in central region of new geometry
- ✅ Smooth density variations

### When Interpolation May Struggle:
- ⚠️ Extreme scaling (R: 1000→5000)
- ⚠️ Very different resolutions (N: 64→256)
- ⚠️ Vortices near geometry boundaries

### Not Preserved:
- ❌ Exact energy (approximation only)
- ❌ Angular momentum (recomputed for new geometry)
- ❌ Precise neighbor connectivity

---

## Debugging

Enable verbose output to see what path is taken:

```bash
bec_sim --init-state file.bec --steps 1
```

**Look for:**
- "Geometry matches saved state" → Fast path
- "Geometry change detected" → Resampling path
- "Falling back to random initialization" → Fallback path

---

## Future Enhancements

Possible improvements:
1. Energy minimization after resampling
2. Adaptive interpolation threshold
3. Angular momentum preservation
4. Wavefunction normalization enforcement

---

## Summary

The geometry-aware loading system provides:
- **Convenience:** Change parameters freely when loading
- **Speed:** Fast reloading when geometry matches
- **Reproducibility:** Fully deterministic from seed
- **Physics:** Intelligent interpolation preserves structures

Enjoy exploring parameter space without constraints!
