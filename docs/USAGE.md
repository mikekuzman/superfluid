# 4D BEC Simulator - Usage Guide

## Overview

The 4D BEC Simulator consists of three executables:
- **bec_sim** - Main simulation executable
- **inspect_bec** - File inspection and validation tool
- **bec_sweep** - Parameter space exploration tool

---

## 1. Running Simulations (bec_sim)

### Basic Usage

```bash
bec_sim --R 1000 --omega 0.5 --steps 10000 --output my_sim.bec
```

### Command-Line Options

**Physical Parameters:**
- `--R <value>` - Hypersphere radius in healing lengths (default: 1000)
- `--delta <value>` - Shell thickness (default: 25)
- `--thickness-ratio <value>` - R/delta ratio (overrides delta)
- `--g <value>` - Interaction strength (default: 0.05)
- `--omega <value>` - Rotation frequency (default: 0.5)

**Computational Parameters:**
- `--N <value>` - Grid resolution parameter (default: 64)
- `--dt <value>` - Time step (default: 0.01)
- `--steps <value>` - Total simulation steps (default: 10000)
- `--save-every <value>` - Snapshot save interval (default: 100)
- `--neighbors <value>` - Number of neighbors for gradients (default: 6)
- `--seed <value>` - Random seed for reproducibility (default: 42)

**Input/Output:**
- `--output <file>` - Output .bec file (default: output.bec)
- `--init-state <file>` - **NEW**: Load initial state from .bec file
- `--init-snapshot <index>` - **NEW**: Which snapshot to load (default: 0)

### Examples

**Standard simulation:**
```bash
bec_sim --R 1000 --thickness-ratio 40 --omega 1.0 --g 0.1 --steps 20000
```

**Continue from previous simulation:**
```bash
# Run initial simulation
bec_sim --omega 0.5 --steps 5000 --output initial.bec

# Continue from final state with higher omega
bec_sim --init-state initial.bec --init-snapshot -1 --omega 1.5 --steps 10000 --output continued.bec
```

**Resume from specific snapshot:**
```bash
# Load snapshot 50 and continue
bec_sim --init-state previous.bec --init-snapshot 50 --steps 5000 --output resumed.bec
```

---

## 2. Parameter Space Exploration (bec_sweep)

The `bec_sweep` tool runs multiple simulations with different parameter combinations and outputs results to CSV for analysis.

### Basic Usage

```bash
bec_sweep --omega 0.5,1.0,2.0 --g 0.05,0.1 --steps 5000 --output sweep_results.csv
```

### Sweep Parameters

Specify comma-separated values for each parameter to sweep:

- `--omega <val1,val2,...>` - Rotation frequencies
- `--g <val1,val2,...>` - Interaction strengths
- `--R <val1,val2,...>` - Radii
- `--thickness-ratio <val1,val2,...>` - Thickness ratios

### Base Parameters

These apply to all runs:
- `--steps <value>` - Steps per simulation (default: 5000)
- `--dt <value>` - Time step (default: 0.01)
- `--N <value>` - Grid resolution (default: 64)
- `--seed <value>` - Random seed (default: 42)
- `--save-every <value>` - Snapshot interval (default: 500)
- `--output <file>` - Output CSV file (default: sweep_results.csv)

### Examples

**Sweep rotation frequency:**
```bash
bec_sweep --omega 0.1,0.5,1.0,1.5,2.0 --steps 10000 --output omega_sweep.csv
```

**Multi-parameter sweep:**
```bash
bec_sweep \
  --omega 0.5,1.0,2.0 \
  --g 0.05,0.1,0.2 \
  --thickness-ratio 20,40,60 \
  --steps 5000 \
  --output full_sweep.csv
```

**Quick exploration (short runs):**
```bash
bec_sweep --omega 0.5,1.0,1.5,2.0,3.0 --steps 2000 --save-every 1000
```

### Output Format

Results are saved as CSV with columns:
- All parameter values (R, delta, thickness_ratio, g, omega, N, dt, steps, random_seed)
- `total_vortices` - Total vortices across all snapshots
- `max_vortices` - Maximum vortices in any single snapshot
- `final_time` - Final simulation time reached
- `computation_time_sec` - Wall-clock time in seconds
- `completed` - Whether simulation finished successfully

### Analysis Workflow

1. **Run sweep:**
   ```bash
   bec_sweep --omega 0.5,1.0,1.5,2.0,2.5,3.0 --g 0.05,0.1,0.15 --steps 5000
   ```

2. **Analyze results:**
   ```python
   import pandas as pd

   df = pd.read_csv('sweep_results.csv')

   # Find parameter sets with vortices
   with_vortices = df[df['total_vortices'] > 0]

   # Sort by vortex count
   best = df.sort_values('total_vortices', ascending=False).head(10)

   # Plot vortices vs omega
   import matplotlib.pyplot as plt
   plt.scatter(df['omega'], df['total_vortices'])
   plt.xlabel('Rotation frequency')
   plt.ylabel('Total vortices')
   plt.show()
   ```

3. **Run detailed simulation with best parameters:**
   ```bash
   # Use parameters from best result
   bec_sim --omega 1.5 --g 0.1 --steps 20000 --output detailed.bec
   ```

---

## 3. Inspecting Results (inspect_bec)

### Basic Usage

```bash
inspect_bec output.bec
```

### Options

- `--validate` - Only validate file integrity
- `--snapshot <index>` - Show specific snapshot details
- `--all` - Show all snapshots (may be verbose)

### Examples

**Quick validation:**
```bash
inspect_bec output.bec --validate
```

**Examine specific snapshot:**
```bash
inspect_bec output.bec --snapshot 50
```

**Full file inspection:**
```bash
inspect_bec output.bec --all
```

---

## Typical Workflow

### 1. Parameter Exploration

```bash
# Quick sweep to find promising parameters
bec_sweep \
  --omega 0.5,1.0,1.5,2.0,2.5,3.0 \
  --g 0.05,0.1,0.15,0.2 \
  --thickness-ratio 20,40,60 \
  --steps 3000 \
  --output initial_sweep.csv

# Analyze results (in Python/Excel)
# Identify parameter range with vortex formation
```

### 2. Focused Investigation

```bash
# Run detailed simulation with best parameters
bec_sim \
  --omega 1.5 \
  --g 0.1 \
  --thickness-ratio 40 \
  --steps 20000 \
  --save-every 100 \
  --output detailed.bec

# Inspect results
inspect_bec detailed.bec
```

### 3. Extended Simulation

```bash
# Continue from final state for longer evolution
bec_sim \
  --init-state detailed.bec \
  --init-snapshot -1 \
  --steps 30000 \
  --output extended.bec
```

### 4. Parameter Refinement

```bash
# Fine-grained sweep around optimal region
bec_sweep \
  --omega 1.2,1.3,1.4,1.5,1.6,1.7,1.8 \
  --g 0.08,0.09,0.10,0.11,0.12 \
  --steps 10000 \
  --output refined_sweep.csv
```

---

## Performance Tips

### For bec_sim:
- Use `--save-every 500` or higher for long runs to reduce I/O
- Adjust `--dt` for stability (smaller if NaN appears)
- Use `--N 64` for quick tests, `--N 128` for production

### For bec_sweep:
- Start with `--steps 2000-5000` for quick exploration
- Use `--save-every 1000` to reduce file I/O overhead
- Run overnight for large sweeps (e.g., 100+ combinations)
- Consider running multiple sweeps in parallel with different seed values

### CUDA Performance:
- Ensure your GPU is detected: check for "CUDA initialized" in output
- GTX 1070 should achieve 200-400 steps/sec depending on parameters
- If "Warning: Running without CUDA" appears, rebuild with `-DUSE_CUDA=ON`

---

## Troubleshooting

### NaN in simulation:
- Reduce time step: `--dt 0.005` or `--dt 0.001`
- Check parameters are physical (g > 0, R > 0, etc.)
- Try different random seed

### No vortices detected:
- Try higher omega: `--omega 1.5` or `--omega 2.0`
- Run longer: `--steps 20000` or `--steps 50000`
- Increase interaction strength: `--g 0.1` or `--g 0.2`
- Use parameter sweep to find working regime

### File loading errors:
- Verify file with `inspect_bec <file> --validate`
- Check snapshot index is valid (0 to n_snapshots-1)
- Ensure parameter compatibility (same R, N, etc.)

### Slow performance:
- Check CUDA is enabled (see output at start)
- Reduce resolution: `--N 48` or `--N 32`
- Reduce neighbors: `--neighbors 4`
- Use fewer save points: `--save-every 500`
