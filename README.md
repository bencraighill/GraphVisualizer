<br />
<div align="center">
  <a href="https://github.com/bencraighill/GraphVisualizer.git">
    <img src="Resources/Branding/Logo.png" alt="Logo" width="350" height="350">
  </a>

  <h3 align="center">GraphVisualizer</h3>

  <p align="center">
    An awesome Visualier to demonstrate Pathfinding Algorithms
  </p>
</div>

# Overview
This project is an interactive graph creation tool as well as a visualizer for a range of pathfinding algorithms. It includes generation tools for a range of graph types. Notably, it supports importing real-world street network data to highlight the applicability of these algorithms to GPS and navigation solutions.

## Features
- An interactive graph editor, including a viewport and tools to modify vertices and edges.
- City-based graph generation is supported for networks using driving, walking, cycling or full multi-modal transport layers. Roads are labelled, and the resulting structure mirrors actual navigable pathways to highlight their real-world applicability.
- Random graph generation is supported to produce a variety of graphs with different densities, structures, sizes, and connectedness. This is particularly ideal for benchmarking and experimentation.
- Text-based graph generation is supported, allowing character glyph geometry to be converted to a navigable graph.
- Save graphs, load them back, or append multiple graphs together to build composite structures.
- Each supported algorithm can be played back using our timeline, highlighting each logical step made during traversal, as well as the final path. These can be overlaid to allow for direct comparisons between algorithms.
- Memory usage can be tracked throughout execution and visualized using our timeline overlay.
- A statistics section provides metrics for directly comparing the optimality and efficiency of each solution.

## Supported Pathfinding Algorithms
- BFS
- DFS
- Dijkstra (array-based)
- Dijkstra (priority queue–based)
- D’Esopo–Pape
- Bellman–Ford
- Floyd–Warshall

# Getting Started
## Installation
Clone the repository and all of its submodules
```bash
git clone --recurse-submodules https://github.com/bencraighill/GraphVisualizer.git
```
If previously cloned non-recursively, clone the necessary submodules
```bash
git submodule update --init
```
## Dependencies
Run the `Setup.bat`/`Setup.sh` script inside the root directory to install all necessary components. Currently, Python is **required** for this project to execute correctly.

Build the project just like any other CMake project
```bash
mkdir ./build
cmake -S . -B build
cmake --build build
```

## Usage
Run the project at the project's root
```bash
.\build\Debug\GraphVisualizer.exe
```
If you are on UNIX
```bash
./build/GraphVisualizer
```

# Preview
![Playback](/Resources/Branding/Screenshots/VisualizerPlayback.gif)
![Single Path](/Resources/Branding/Screenshots/VisualizerSingle.png)
![Multiple Paths](/Resources/Branding/Screenshots/VisualizerMultiple.png)
![Map Menu](/Resources/Branding/Screenshots/VisualizerMapMenu.png)
![Startup](/Resources/Branding/Screenshots/VisualizerHello.png)

# Acknowledgments
 - [imgui](https://github.com/ocornut/imgui.git)
 - [glfw](https://github.com/glfw/glfw.git)
 - [yaml-cpp](https://github.com/jbeder/yaml-cpp.git)
 - [libtinyfiledialogs](https://github.com/native-toolkit/libtinyfiledialogs.git)
 - [stb_image and stb_freetype](https://github.com/nothings/stb)
