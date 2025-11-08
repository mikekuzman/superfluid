#include "HypersphereBEC.h"
#include "SimulationParams.h"
#include "BECFileReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <functional>

/**
 * Parameter space sweep for 4D BEC simulator
 *
 * Efficiently explores parameter space to find vortex formation conditions.
 * Results are saved to CSV for analysis.
 */

struct SweepRange {
    std::string name;
    std::vector<float> values;
};

struct SweepResult {
    BEC::SimulationParams params;
    uint32_t total_vortices;
    uint32_t max_vortices_per_snapshot;
    float final_time;
    float computation_time_sec;
    bool completed;
};

// Parse sweep configuration from command line
std::vector<SweepRange> parse_sweep_config(int argc, char** argv) {
    std::vector<SweepRange> ranges;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--omega" && i + 1 < argc) {
            SweepRange range;
            range.name = "omega";

            // Parse comma-separated values: --omega 0.1,0.5,1.0,2.0
            std::string values_str = argv[++i];
            std::stringstream ss(values_str);
            std::string value;
            while (std::getline(ss, value, ',')) {
                range.values.push_back(std::atof(value.c_str()));
            }

            ranges.push_back(range);
        }
        else if (arg == "--g" && i + 1 < argc) {
            SweepRange range;
            range.name = "g";

            std::string values_str = argv[++i];
            std::stringstream ss(values_str);
            std::string value;
            while (std::getline(ss, value, ',')) {
                range.values.push_back(std::atof(value.c_str()));
            }

            ranges.push_back(range);
        }
        else if (arg == "--R" && i + 1 < argc) {
            SweepRange range;
            range.name = "R";

            std::string values_str = argv[++i];
            std::stringstream ss(values_str);
            std::string value;
            while (std::getline(ss, value, ',')) {
                range.values.push_back(std::atof(value.c_str()));
            }

            ranges.push_back(range);
        }
        else if (arg == "--thickness-ratio" && i + 1 < argc) {
            SweepRange range;
            range.name = "thickness_ratio";

            std::string values_str = argv[++i];
            std::stringstream ss(values_str);
            std::string value;
            while (std::getline(ss, value, ',')) {
                range.values.push_back(std::atof(value.c_str()));
            }

            ranges.push_back(range);
        }
    }

    return ranges;
}

// Get base parameters from command line
BEC::SimulationParams get_base_params(int argc, char** argv) {
    BEC::SimulationParams params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--steps" && i + 1 < argc) {
            params.n_steps = std::atoi(argv[++i]);
        }
        else if (arg == "--dt" && i + 1 < argc) {
            params.dt = std::atof(argv[++i]);
        }
        else if (arg == "--N" && i + 1 < argc) {
            params.N = std::atoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc) {
            params.random_seed = std::atoi(argv[++i]);
        }
        else if (arg == "--save-every" && i + 1 < argc) {
            params.save_every = std::atoi(argv[++i]);
        }
        else if (arg == "--init-state" && i + 1 < argc) {
            params.init_state_file = argv[++i];
        }
        else if (arg == "--init-snapshot" && i + 1 < argc) {
            params.init_snapshot_index = std::atoi(argv[++i]);
        }
    }

    // Disable file writing for sweep (too much disk space)
    params.output_file = "";

    return params;
}

// Run single simulation and extract results
SweepResult run_simulation(const BEC::SimulationParams& params) {
    SweepResult result;
    result.params = params;
    result.total_vortices = 0;
    result.max_vortices_per_snapshot = 0;
    result.completed = false;

    auto start_time = std::chrono::high_resolution_clock::now();

    try {
        // Create temporary output file
        BEC::SimulationParams temp_params = params;
        temp_params.output_file = "sweep_temp.bec";

        BEC::HypersphereBEC sim(temp_params);
        sim.run(temp_params.n_steps, temp_params.save_every);

        result.completed = true;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        result.computation_time_sec = duration.count() / 1000.0f;

        // Extract vortex statistics from file
        BEC::BECFileReader reader;
        if (reader.open("sweep_temp.bec")) {
            for (size_t i = 0; i < reader.get_snapshot_count(); ++i) {
                const auto& stats = reader.get_snapshot_stats(i);
                result.total_vortices += stats.n_vortices;
                if (stats.n_vortices > result.max_vortices_per_snapshot) {
                    result.max_vortices_per_snapshot = stats.n_vortices;
                }
                result.final_time = stats.time;
            }
            reader.close();
        }

        // Clean up temporary file
        std::remove("sweep_temp.bec");

    } catch (const std::exception& e) {
        std::cerr << "Simulation failed: " << e.what() << std::endl;
        result.completed = false;
    }

    return result;
}

// Generate all parameter combinations
void generate_combinations(
    const std::vector<SweepRange>& ranges,
    const BEC::SimulationParams& base_params,
    std::vector<BEC::SimulationParams>& combinations)
{
    if (ranges.empty()) {
        combinations.push_back(base_params);
        return;
    }

    // Recursive combination generation
    std::function<void(size_t, BEC::SimulationParams)> generate;
    generate = [&](size_t range_idx, BEC::SimulationParams current_params) {
        if (range_idx >= ranges.size()) {
            combinations.push_back(current_params);
            return;
        }

        const SweepRange& range = ranges[range_idx];
        for (float value : range.values) {
            BEC::SimulationParams next_params = current_params;

            // Set parameter based on name
            if (range.name == "omega") next_params.omega = value;
            else if (range.name == "g") next_params.g = value;
            else if (range.name == "R") next_params.R = value;
            else if (range.name == "thickness_ratio") next_params.thickness_ratio = value;

            generate(range_idx + 1, next_params);
        }
    };

    generate(0, base_params);
}

// Save results to CSV
void save_results(const std::string& filename, const std::vector<SweepResult>& results) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }

    // Header
    file << "R,delta,thickness_ratio,g,omega,N,dt,steps,random_seed,";
    file << "total_vortices,max_vortices,final_time,computation_time_sec,completed\n";

    // Data
    for (const auto& result : results) {
        const auto& p = result.params;

        file << p.R << ","
             << p.delta << ","
             << p.thickness_ratio << ","
             << p.g << ","
             << p.omega << ","
             << p.N << ","
             << p.dt << ","
             << p.n_steps << ","
             << p.random_seed << ","
             << result.total_vortices << ","
             << result.max_vortices_per_snapshot << ","
             << result.final_time << ","
             << result.computation_time_sec << ","
             << (result.completed ? "true" : "false") << "\n";
    }

    file.close();
    std::cout << "\nResults saved to: " << filename << std::endl;
}

void print_help() {
    std::cout << "4D BEC Parameter Space Sweep Tool\n\n";
    std::cout << "Usage: bec_sweep [options]\n\n";
    std::cout << "Sweep Parameters (comma-separated values):\n";
    std::cout << "  --omega <values>         Rotation frequencies (e.g., 0.1,0.5,1.0,2.0)\n";
    std::cout << "  --g <values>             Interaction strengths (e.g., 0.05,0.1,0.2)\n";
    std::cout << "  --R <values>             Radii (e.g., 500,1000,1500)\n";
    std::cout << "  --thickness-ratio <vals> Thickness ratios (e.g., 20,40,60)\n\n";
    std::cout << "Base Parameters:\n";
    std::cout << "  --steps <value>          Steps per simulation (default: 5000)\n";
    std::cout << "  --dt <value>             Time step (default: 0.01)\n";
    std::cout << "  --N <value>              Grid resolution (default: 64)\n";
    std::cout << "  --seed <value>           Random seed (default: 42)\n";
    std::cout << "  --save-every <value>     Snapshot interval (default: 500)\n";
    std::cout << "  --init-state <file>      Load initial state from .bec file (all runs use same state)\n";
    std::cout << "  --init-snapshot <index>  Which snapshot to load (default: 0)\n";
    std::cout << "  --output <file>          Results CSV file (default: sweep_results.csv)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Sweep from random initial state\n";
    std::cout << "  bec_sweep --omega 0.5,1.0,2.0 --g 0.05,0.1 --steps 5000 --output results.csv\n\n";
    std::cout << "  # Sweep from pre-equilibrated state\n";
    std::cout << "  bec_sweep --omega 1.0,1.5,2.0 --init-state equilibrated.bec --init-snapshot 100\n\n";
}

int main(int argc, char** argv) {
    // Check for help
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            print_help();
            return 0;
        }
    }

    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         4D BEC PARAMETER SPACE SWEEP                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    // Parse configuration
    auto sweep_ranges = parse_sweep_config(argc, argv);
    auto base_params = get_base_params(argc, argv);

    // Override some defaults for sweep
    if (base_params.n_steps == 10000) base_params.n_steps = 5000;  // Shorter sims
    if (base_params.save_every == 100) base_params.save_every = 500;  // Less frequent saves

    // Generate all combinations
    std::vector<BEC::SimulationParams> combinations;
    generate_combinations(sweep_ranges, base_params, combinations);

    std::cout << "Sweep Configuration:\n";
    for (const auto& range : sweep_ranges) {
        std::cout << "  " << range.name << ": ";
        for (size_t i = 0; i < range.values.size(); ++i) {
            std::cout << range.values[i];
            if (i < range.values.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "\nTotal combinations: " << combinations.size() << "\n";
    std::cout << "Steps per simulation: " << base_params.n_steps << "\n";
    std::cout << "Starting sweep...\n\n";

    // Run all simulations
    std::vector<SweepResult> results;
    for (size_t i = 0; i < combinations.size(); ++i) {
        std::cout << "=== Run " << (i + 1) << "/" << combinations.size() << " ===\n";
        std::cout << "  omega = " << combinations[i].omega
                  << ", g = " << combinations[i].g
                  << ", R = " << combinations[i].R
                  << ", thickness_ratio = " << combinations[i].thickness_ratio << "\n";

        auto result = run_simulation(combinations[i]);
        results.push_back(result);

        std::cout << "  Vortices: " << result.total_vortices
                  << " (max " << result.max_vortices_per_snapshot << " per snapshot)\n";
        std::cout << "  Time: " << result.computation_time_sec << " sec\n";
        std::cout << "  Status: " << (result.completed ? "SUCCESS" : "FAILED") << "\n\n";
    }

    // Get output filename
    std::string output_file = "sweep_results.csv";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        }
    }

    // Save results
    save_results(output_file, results);

    // Summary
    size_t successful = 0;
    size_t with_vortices = 0;
    for (const auto& result : results) {
        if (result.completed) successful++;
        if (result.total_vortices > 0) with_vortices++;
    }

    std::cout << "\n=== Summary ===\n";
    std::cout << "Total runs: " << combinations.size() << "\n";
    std::cout << "Successful: " << successful << "\n";
    std::cout << "With vortices: " << with_vortices << "\n";
    std::cout << "Success rate: " << (100.0f * successful / combinations.size()) << "%\n";
    std::cout << "Vortex rate: " << (100.0f * with_vortices / combinations.size()) << "%\n";

    return 0;
}
