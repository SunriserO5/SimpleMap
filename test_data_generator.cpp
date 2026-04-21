#include "dataStructure.h"
#include "pathfinding.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string mode = "random";   // random | grid
    int nodes = 1000;
    int width = 5000;
    int height = 5000;
    int cols = 0;
    int rows = 0;
    std::string output;
};

void printUsage(const char* program) {
    std::cout
        << "Usage:\n"
        << "  " << program << " [--mode random|grid] [--nodes N] [--width W] [--height H]\\n"
        << "             [--cols C --rows R] [--output FILE]\\n\\n"
        << "Examples:\n"
        << "  " << program << "\\n"
        << "  " << program << " --nodes 1000 --output map_1000.json\\n"
        << "  " << program << " --mode grid --nodes 1000 --cols 40 --rows 25 --output grid_1000.json\\n";
}

bool parsePositiveInt(const std::string& text, int& value) {
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    long parsed = std::strtol(text.c_str(), &end, 10);
    if (*end != '\0' || parsed <= 0 || parsed > 1000000000L) {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return false;
        }

        if (i + 1 >= argc) {
            std::cerr << "[Error] Missing value for argument: " << arg << "\n";
            return false;
        }

        std::string value = argv[++i];
        if (arg == "--mode") {
            if (value != "random" && value != "grid") {
                std::cerr << "[Error] --mode must be random or grid\n";
                return false;
            }
            opt.mode = value;
        } else if (arg == "--nodes") {
            if (!parsePositiveInt(value, opt.nodes) || opt.nodes < 2) {
                std::cerr << "[Error] --nodes must be an integer >= 2\n";
                return false;
            }
        } else if (arg == "--width") {
            if (!parsePositiveInt(value, opt.width)) {
                std::cerr << "[Error] --width must be a positive integer\n";
                return false;
            }
        } else if (arg == "--height") {
            if (!parsePositiveInt(value, opt.height)) {
                std::cerr << "[Error] --height must be a positive integer\n";
                return false;
            }
        } else if (arg == "--cols") {
            if (!parsePositiveInt(value, opt.cols)) {
                std::cerr << "[Error] --cols must be a positive integer\n";
                return false;
            }
        } else if (arg == "--rows") {
            if (!parsePositiveInt(value, opt.rows)) {
                std::cerr << "[Error] --rows must be a positive integer\n";
                return false;
            }
        } else if (arg == "--output") {
            opt.output = value;
        } else {
            std::cerr << "[Error] Unknown argument: " << arg << "\n";
            return false;
        }
    }

    return true;
}

void autoSelectGridShape(int nodes, int& cols, int& rows) {
    if (cols > 0 && rows > 0) {
        return;
    }

    int bestCols = 0;
    int bestRows = 0;
    for (int c = 2; c * c <= nodes; ++c) {
        if (nodes % c == 0) {
            int r = nodes / c;
            if (r >= 2) {
                bestCols = c;
                bestRows = r;
            }
        }
    }

    if (bestCols > 0 && bestRows > 0) {
        cols = bestCols;
        rows = bestRows;
        return;
    }

    cols = 0;
    rows = 0;
}

void printSummary(const mapGraphBase& graphRef, int expectedNodes, const std::string& mode, double ms) {
    mapGraphBase& graph = const_cast<mapGraphBase&>(graphRef);
    int actualNodes = graph.getNumOfNodes();
    int actualEdges = graph.getNumOfEdges();
    bool connected = graph.isMapFullyConnected();
    bool valid = graph.isDataValid();

    std::cout << "[Generator] mode=" << mode
              << ", expectedNodes=" << expectedNodes
              << ", actualNodes=" << actualNodes
              << ", edges=" << actualEdges
              << ", timeMs=" << ms << "\n";
    std::cout << "[Quality] connected=" << (connected ? "true" : "false")
              << ", dataValid=" << (valid ? "true" : "false") << "\n";

    if (actualNodes != expectedNodes) {
        std::cout << "[Warning] actualNodes != expectedNodes ("
                  << actualNodes << " vs " << expectedNodes << ")\n";
    }

    if (actualNodes >= 2) {
        std::vector<int> path = PathFinder::dijkstra(&graph, 0, actualNodes - 1, false);
        std::cout << "[PathCheck] 0->" << (actualNodes - 1)
                  << " nodesInPath=" << path.size() << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) {
        return 1;
    }

    if (opt.output.empty()) {
        opt.output = "test_map_" + std::to_string(opt.nodes) + ".json";
    }

    mapGraphBase graph;
    bool generated = false;

    auto start = std::chrono::steady_clock::now();

    if (opt.mode == "random") {
        generated = graph.generateRandomMap(opt.width, opt.height, opt.nodes);
    } else {
        autoSelectGridShape(opt.nodes, opt.cols, opt.rows);
        if (opt.cols < 2 || opt.rows < 2 || opt.cols * opt.rows != opt.nodes) {
            std::cerr << "[Error] Grid mode requires cols*rows == nodes and both >= 2.\n"
                      << "        Example for 1000 nodes: --cols 40 --rows 25\n";
            return 1;
        }
        generated = graph.generateGridMap(opt.width, opt.height, opt.cols, opt.rows);
    }

    auto end = std::chrono::steady_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    if (!generated) {
        std::cerr << "[Error] map generation failed.\n";
        return 1;
    }

    printSummary(graph, opt.nodes, opt.mode, elapsedMs);

    if (!graph.saveMapToFile(opt.output.c_str())) {
        std::cerr << "[Error] saveMapToFile failed: " << opt.output << "\n";
        return 1;
    }

    std::cout << "[Output] saved to: " << opt.output << "\n";
    return 0;
}
