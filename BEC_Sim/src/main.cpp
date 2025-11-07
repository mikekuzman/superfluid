#include "HypersphereBEC.h"
#include "SimulationParams.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

// Simple command-line argument parsing
BEC::SimulationParams parse_arguments(int argc, char** argv) {
    BEC::SimulationParams params;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--R" && i + 1 < argc) {
            params.R = std::atof(argv[++i]);
        }
        else if (arg == "--delta" && i + 1 < argc) {
            params.delta = std::atof(argv[++i]);
        }
        else if (arg == "--thickness-ratio" && i + 1 < argc) {
            params.thickness_ratio = std::atof(argv[++i]);
        }
        else if (arg == "--g" && i + 1 < argc) {
            params.g = std::atof(argv[++i]);
        }
        else if (arg == "--omega" && i + 1 < argc) {
            params.omega = std::atof(argv[++i]);
        }
        else if (arg == "--N" && i + 1 < argc) {
            params.N = std::atoi(argv[++i]);
        }
        else if (arg == "--dt" && i + 1 < argc) {
            params.dt = std::atof(argv[++i]);
        }
        else if (arg == "--steps" && i + 1 < argc) {
            params.n_steps = std::atoi(argv[++i]);
        }
        else if (arg == "--save-every" && i + 1 < argc) {
            params.save_every = std::atoi(argv[++i]);
        }
        else if (arg == "--output" && i + 1 < argc) {
            params.output_file = argv[++i];
        }
        else if (arg == "--seed" && i + 1 < argc) {
            params.random_seed = std::atoi(argv[++i]);
        }
        else if (arg == "--neighbors" && i + 1 < argc) {
            params.n_neighbors = std::atoi(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "4D Bose-Einstein Condensate Simulator\n\n";
            std::cout << "Usage: bec_sim [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --R <value>              Hypersphere radius (default: 1000)\n";
            std::cout << "  --delta <value>          Shell thickness (default: 25)\n";
            std::cout << "  --thickness-ratio <val>  R/delta ratio (overrides delta)\n";
            std::cout << "  --g <value>              Interaction strength (default: 0.05)\n";
            std::cout << "  --omega <value>          Rotation frequency (default: 0.5)\n";
            std::cout << "  --N <value>              Grid resolution (default: 64)\n";
            std::cout << "  --dt <value>             Time step (default: 0.01)\n";
            std::cout << "  --steps <value>          Total steps (default: 10000)\n";
            std::cout << "  --save-every <value>     Save interval (default: 100)\n";
            std::cout << "  --output <file>          Output file (default: output.bec)\n";
            std::cout << "  --seed <value>           Random seed (default: 42)\n";
            std::cout << "  --neighbors <value>      Neighbors for gradients (default: 6)\n";
            std::cout << "  --help, -h               Show this help\n\n";
            std::cout << "Example:\n";
            std::cout << "  bec_sim --R 1000 --thickness-ratio 40 --omega 0.5 --steps 5000\n";
            std::exit(0);
        }
    }

    return params;
}

void print_banner() {
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║                                                               ║
║      4D BOSE-EINSTEIN CONDENSATE SIMULATOR                    ║
║                                                               ║
║      Quantum Superfluid on Hypersphere Shell                  ║
║      CUDA-Accelerated GPE Solver                              ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

int main(int argc, char** argv) {
    try {
        print_banner();

        // Parse arguments
        BEC::SimulationParams params = parse_arguments(argc, argv);

        // Apply thickness ratio if specified
        params.apply_thickness_ratio();

        // Create and run simulation
        BEC::HypersphereBEC simulation(params);
        simulation.run(params.n_steps, params.save_every);

        std::cout << "\n✓ Simulation completed successfully!" << std::endl;
        std::cout << "Output written to: " << params.output_file << std::endl;

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "\n✗ Unknown error occurred" << std::endl;
        return 1;
    }
}
