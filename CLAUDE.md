# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SimpleMap is a C++ vehicle navigation system implementing map generation, shortest path algorithms, and traffic flow simulation. This is a data structures course assignment.

## Build & Run

- **IDE**: Visual Studio 2022 (MSVC) — no Makefile/CMake; build via VS solution
- **Standard**: C++11+
- **Planned dependency**: nlohmann/json (header-only, for JSON persistence — not yet integrated)

## Source Files

| File | Role |
|------|------|
| `dataStructure.h` | All struct/class declarations and public/private interfaces |
| `dataStricture.cpp` | Implementations (note: typo in filename — "Stricture" vs "Structure") |
| `parameters.h` | Global constants (`CACHE_SIZE=100`) |

No `main.cpp` exists yet; no test suite exists yet.

## Architecture

### Core Data Structures

- **`mapNode`**: Graph vertex with ID, x/y coordinates, dynamic neighbor list (`int* Neighbour`, `int* distanceToNeighbour`). Manual memory management via `new[]`.
- **`EdgeAttr`**: Directed edge with length, speedLimit (default 10.0), capacity (length×0.8), and congestion.
- **`cacheNode`**: Shortest-path result cache (begin/end node IDs, distance, path pointer).
- **`mapGraphBase`**: Main class holding the graph as a flat `mapNode*` array with separate `EdgeAttr*` array. Uses adjacency-list-style neighbor arrays within each node.

### Key Design Decisions

- Graph stored as a contiguous `mapNode[]` array (allocated via `allocateMainGraphMemory`), not a true adjacency list of linked lists.
- Each node owns dynamically allocated `Neighbour[]` and `distanceToNeighbour[]` arrays — careful memory cleanup required in `clearMap()` and destructor.
- `EdgeAttr` tracks both topological info (fromNode, toNode) and traffic simulation attributes (congestion, capacity, speedLimit).
- Path cache is a fixed-size `cacheNode[CACHE_SIZE]` array.

### Implementation Status

**Implemented**: Constructor, destructor (`clearMap`), `addNode`, `calculateDistance`

**Declared but not implemented**: `generateGridMap`, `generateRandomMap`, `addTwoWayEdge`, `addOneWayEdge`, `getNodeById`, `getEdgeById`, `getEdgeByNodes`, `updateEdgeCongestion`, `isMapFullyConnected`, `isDataValid`, `clearMap`, `allocateMainGraphMemory`, `freeMainGraphMemory`, `saveMapToFile`, `loadMapFromFile`, pathfinding (Dijkstra/A*)

## Known Issues

- `dataStructure.h` has **duplicate** constructor/destructor declarations (lines 69-70, 77-78, 84-85) — only one set should remain.
- The `EdgeAttr` struct in the header uses `congestion` but the README's code example references `currentCars` — these may need to be separate fields or reconciled.
- `parameters.h` defines `CACHE_SIZE` as a non-const global `int` in a header — will cause multiple-definition errors if included from multiple .cpp files. Should be `const int` or `inline constexpr int`.
- `mapGraphBase` constructor ignores its `nodes`, `edges`, `graph` parameters — all fields are hardcoded to 0/nullptr.

## Conventions

- Chinese comments throughout (consistent with the team's language)
- camelCase for methods and struct fields; `m_` prefix for private member variables (e.g., `m_allocatedNodeCount`)
- snake_case not used despite appearing in some field names in README examples
