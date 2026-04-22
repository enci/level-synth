# Level Synth

A node-based level generation tool — conceptually "Houdini for Level Generation" — with a runtime SDK. Built as part of a PhD research project on procedural content generation.

The tool is a testbed for diverse generation techniques, where each technique is a node type. Users compose node graphs to create level generators that can run in the editor with visual feedback or at runtime in a game via a C++ SDK.

## Architecture

### Design principles

**No game-specific semantics in the core.** The tool doesn't know what a "room," "enemy," or "door" is. The attributed grid is a generic 2D container with named integer layers. Meaning comes from the user's node graph, not the data model. This keeps the tool general and extensible across genres and styles.

**One universal data primitive.** Rather than having separate types for spatial grids, topology graphs, adjacency matrices, and property tables, everything is an attributed grid. A dungeon floor is a 64×64 grid with a "terrain" attribute. A room connectivity graph is a 12×12 grid with a "connected" attribute. A list of room properties is a 12×1 grid with "type" and "difficulty" attributes. Same primitive, different interpretation. This mirrors Houdini's philosophy of "everything is geometry with attributes."

**Multiple output pins, not bundled data.** When a node produces grids of different dimensions (e.g. a BSP node outputs both a 64×64 spatial grid and a 12×12 room adjacency grid), it uses separate output pins rather than bundling them into a compound type. This keeps the attributed grid simple (one grid = one resolution) and lets users wire each output independently.

**Immutable inputs, fresh outputs.** Nodes receive const references to their input grids and always produce new output grids. This enables caching (if inputs haven't changed, skip re-evaluation) and step-through debugging (every intermediate result is preserved and inspectable). The evaluation engine owns all cached data; nodes borrow via references.

**Runtime descriptors, not compile-time types.** Node pin definitions are runtime data (the `node_descriptor` struct), not template parameters or macros. This enables the plugin/SDK model where third parties register node types via shared libraries without recompiling the editor. Type checking (number↔number, grid↔grid) happens at wire connection time in the editor.

**Coroutines for granular debugging.** Node evaluation uses C++20 coroutines. A node can `co_yield` after each meaningful step (e.g. each iteration of cellular automata, each room carved). In normal mode, the engine drives the coroutine to completion instantly. In debug mode, each yield pauses execution, updates the preview, and waits for the user. This is a single code path — no separate "debug evaluate" method.

**Separation of editor and runtime.** The library has zero UI dependencies. The `#ifdef LS_EDITOR` guard on `draw_ui()` means node authors write logic and UI in one class, but the SDK build never sees ImGui headers. The generator class treats a node graph as a black box with named inputs and outputs, suitable for embedding in a game.

**Integer-only grid cells.** Grid attributes store `int` values. Floats are unnecessary for the core level generation domain — tile types, room IDs, flags, connectivity weights, and depth values are all naturally discrete. Scalar parameters flowing between nodes as pin values are `double`, but once data enters a grid cell it's an int. This simplifies the attribute storage and avoids type variants per cell.

**Two pin types only.** Early designs had separate integer and float pins, but the distinction added complexity without practical benefit. Collapsing to `number` (double) and `grid` (layered_grid) simplifies wire validation, the pin color palette, and the node authoring API.

### Build targets

The project has two main targets:

**`level_synth_library`** — A static C++ library with no UI dependencies. Contains the evaluation engine, node system, and all built-in node types. This is what the runtime SDK ships: a game loads a serialized graph file and evaluates it.

**`level_synth_editor`** — An ImGui-based visual editor built on SDL3. Provides a node graph editor, preview panel, and debugging tools. Links against the library and defines `LS_EDITOR` to enable node UI methods.

### Core data primitive

The **attributed grid** is the universal data container. It is a 2D array where each cell can carry multiple named integer attributes (e.g. "terrain", "room_id", "depth"). All data flowing through grid pins is an attributed grid. There are no hardcoded game-specific semantics — interpretation is entirely up to the user's node graph.

Graphs, adjacency matrices, room topologies, and other abstract structures can all be represented as grids. A room graph with 12 rooms is a 12×12 grid where cell (i,j) stores edge weight. Room attributes are additional columns on an N×1 grid.

### Pin types

There are two pin types:

- **Number** (`double`) — scalar parameters like room count, threshold, difficulty
- **Grid** (`layered_grid`) — spatial data, topology data, any 2D attributed array

Wires are type-checked at connection time: number↔number and grid↔grid only.

### Node system

Nodes are defined as C++ classes inheriting from `ls::node`. Each provides:

- A **descriptor** (name, category, pin definitions) — static, used by the editor for rendering
- An **evaluate** method (a C++20 coroutine returning `eval_task`) — can `co_yield` intermediate steps for debug visualization
- A **draw_ui** method (editor-only, behind `#ifdef LS_EDITOR`) — custom ImGui widgets for node-specific parameters
- Member variables for node configuration (defaults for unconnected pins, property strings, etc.)

Node types are registered in the `node_registry` with a string key and factory function. The editor's right-click context menu is populated from the registry, grouped by category.

### Evaluation

The `eval_engine` owns all nodes and wires. Evaluation proceeds in topological order. For each node:

1. A fresh `eval_context` is created
2. Inputs are populated from upstream cached outputs
3. RNG is seeded deterministically: `hash(master_seed, node_id)`
4. The node's coroutine runs
5. Outputs are cached

Caching means only invalidated nodes re-evaluate. Adding/removing a wire invalidates the target node and everything downstream.

### Stepping and debugging

The coroutine-based evaluation supports three modes:

- **Normal** — run to completion instantly
- **Step-through** — pause after each node, inspect intermediate outputs
- **Animated** — resume on a timer, watch generation unfold (e.g. cellular automata iterating)

### Generator I/O

Special node types define the graph's public API:

- **Input Number** — exposes a named parameter the game can set (`gen.set_parameter("difficulty", 0.7)`)
- **Output Grid** / **Output Number** — exposes a named result the game can retrieve (`gen.get_grid_output("terrain")`)

The `generator` class wraps the engine and provides the SDK-facing interface: set parameters, set seed, evaluate, get outputs.

### Seeding

Hierarchical model: each node derives its seed from `hash(master_seed, node_id)`. This ensures reproducibility (same master seed = same output) and locality (changing one node's parameters doesn't affect unrelated nodes' random decisions).

## Tech stack

- **Language**: C++20 (coroutines, std::span, designated initializers)
- **Build**: CMake
- **UI**: ImGui (with node editor extension), SDL3, FreeType
- **Icons**: Fluent System Icons

## Project structure

```
library/level_synth/
    level_synth.hpp              umbrella header
    layered_grid.hpp/.cpp      core data primitive
    pin.hpp                      pin types and pin_value variant
    eval_task.hpp                coroutine return type (header-only)
    eval_context.hpp/.cpp        per-node input/output access
    eval_engine.hpp/.cpp         graph ownership, topological eval, caching, stepping
    node.hpp                     base class for all nodes
    node_registry.hpp/.cpp       string-keyed factory registry
    generator.hpp/.cpp           SDK-facing wrapper
    nodes/
        node_create_grid.hpp/.cpp
        node_cellular_automata.hpp/.cpp
        node_input_number.hpp/.cpp
        node_output_grid.hpp/.cpp
        node_output_number.hpp/.cpp

editor/
    main.cpp
    application.hpp/.cpp         SDL3/ImGui app, node editor, toolbar
    fluent_glyph.hpp             icon font integration
    nodes/
        node_colors.hpp          pin/header color definitions
```

## Implementation status

### Core
- [x] Attributed grid (int attributes, named, per-cell access, bulk span access)
- [x] Pin types (number, grid)
- [x] Pin value variant (double, shared_ptr\<layered_grid\>)
- [x] Node base class with virtual evaluate (coroutine), draw_ui (editor-only)
- [x] Node descriptor (name, category, typed pin definitions)
- [x] Eval context (input/output access, seeded RNG)
- [x] Eval task (C++20 coroutine wrapper with yield/resume/run)
- [x] Eval engine (topological sort, build context, evaluate, cache, invalidate)
- [x] Eval engine stepping (begin_stepping, step, current_node_id, current_step)
- [x] Node registry (string-keyed factory, create, registered_types, descriptor)
- [x] Generator (set_parameter, set_seed, evaluate, get_grid_output, get_number_output, rebuild_bindings)

### Built-in nodes
- [x] Create Grid (width, height, fill_value, attribute_name)
- [x] Cellular Automata (input grid, iterations, birth/death thresholds, co_yield per iteration)
- [x] Input Number (named parameter with default and runtime override)
- [x] Output Grid (named grid sink)
- [x] Output Number (named number sink)

### Editor
- [x] SDL3 + ImGui application shell
- [x] ImGui node editor integration (node builder, pin icons, link rendering)
- [x] Toolbar (menu, theme toggle, zoom controls, fps display)
- [x] Light and dark themes
- [x] Fluent icon font integration
- [x] Event-driven idle loop (SDL_WaitEvent with cooldown frames)
- [ ] Data-driven node rendering (nodes rendered from eval engine, not hardcoded)
- [ ] Right-click context menu (add nodes from registry)
- [ ] Wire creation with type checking (number↔number, grid↔grid)
- [ ] Wire and node deletion
- [ ] Node property panel (inspector for selected node's parameters)
- [ ] Grid preview panel (color-mapped cell view of any output pin)
- [ ] Attribute selector in preview (choose which attribute to visualize)
- [ ] Step-through debug UI (play/pause/step buttons, current node highlight)
- [ ] Animated step-through (timer-driven resume)
- [ ] Diff view (highlight changed cells after re-evaluation)

### Planned node types
- [ ] Random Fill (binary noise from seeded RNG, density parameter)
- [ ] Perlin/Simplex Noise (continuous noise quantized to int grid)
- [ ] BSP Split (recursive partition, outputs spatial grid + adjacency grid)
- [ ] Scatter Rooms (place rooms randomly within grid bounds)
- [ ] Carve Rooms (carve floor regions within partitions)
- [ ] Connect Rooms (corridor carving between room pairs)
- [ ] Assemble Templates (expand macro grid + template library into spatial grid)
- [ ] Build Adjacency (derive connectivity grid from spatial grid)
- [ ] Minimum Spanning Tree (on adjacency grid)
- [ ] Add Extra Edges (add cycles beyond MST)
- [ ] Random Walk Path (Spelunky-style guaranteed path)
- [ ] Grow Critical Path (branching walk with depth tracking)
- [ ] Grow Side Branches (extend branches off critical path)
- [ ] Flood Fill (region labeling)
- [ ] Longest / Shortest Path (on adjacency grid)
- [ ] Dead End Finder (identify leaf nodes in topology)
- [ ] Critical Path / Depth (BFS distance from source)
- [ ] Template Classifier (assign template type based on neighbor connections)
- [ ] Room Type Tagger (assign roles: shop, vault, boss, heal)
- [ ] Assign by Rule (generic conditional attribute writer)
- [ ] Place Entrance & Exit
- [ ] Place Stairs
- [ ] Place Doors
- [ ] Place Locks & Keys (depth-aware placement)
- [ ] Place Enemies (scaled by depth/difficulty)
- [ ] Place Treasure / Loot
- [ ] Place Traps
- [ ] Threshold / Remap (value mapping)
- [ ] Mask (boolean/float region constraint)
- [ ] Merge Grids (combine two grids by attribute)

### Planned features
- [ ] Serialization (save/load node graphs to JSON)
- [ ] Undo/redo (command pattern on graph operations)
- [ ] Alphabets / legends (optional value-to-label-and-color mappings for visualization)
- [ ] Rule node with legend integration (human-readable conditions using alphabet labels)
- [ ] Copy/paste nodes and subgraphs
- [ ] Node groups / subgraphs (collapse a section into a single node)
- [ ] Runtime SDK packaging (graph file + evaluator, no editor dependency)
- [ ] Plugin system (shared library node registration)
- [ ] Hand-painting tool (paint grid cells directly, used as mask or seed data)
- [ ] Constraint validation (flag broken invariants: unreachable rooms, key behind its own lock)
- [ ] Export to common formats (JSON tilemap, Tiled, engine-specific)
- [ ] Multiple output pin preview (click any output pin to preview that grid)
- [ ] Minimap / overview of large grids
- [ ] Performance profiling per node (time spent in evaluate)
- [ ] Seed management UI (re-roll, lock, seed history/bookmarks)
- [ ] Batch generation (generate N variants, browse results)

### Example generators to recreate
- [ ] Spelunky (template-driven, 4×4 macro grid with guaranteed path)
- [ ] Diablo 1 Cathedral (quad-tree BSP, room carving, themed corridors)
- [ ] Moonlighter (critical path growth, room type assignment, template selection)
- [ ] NetHack (scatter rooms, MST corridors, key/lock, enemy/loot scaling)
- [ ] Brogue (hybrid caves + rooms, machine contraptions)
- [ ] Minecraft terrain (noise composition, biome rules, ore/cave passes)
- [ ] Dead Cells (biome graph + chunk assembly, hierarchical generation)
- [ ] Slay the Spire (DAG encounter map, non-spatial, rule-based typing)
- [ ] Hades (curated arena sequence, probability-driven encounter/reward)
- [ ] Dwarf Fortress (geology, hydrology, deep pipeline stress test)
- [ ] Into the Breach (small grid, constraint-heavy tactical puzzle generation)

## Research foundation

This tool is informed by a survey of 120 game development professionals examining PCG tool adoption. Key findings driving design decisions:

- **Adoption gap**: designers use PCG far less than artists (p=0.001). The tool must speak the designer's language.
- **Control over constraints** is the #1 feature request for node-based tools.
- **Creative control and transparency** consistently prioritized over automation across all AI-related questions.
- **Rectangular grids** dominate level representation (57% of respondents).
- **Real-time feedback** rated essential or very important by 67%.
- **Pre-built configurable components** preferred by 47.5% as the generator creation approach.

## License

MIT License — see license.txt
