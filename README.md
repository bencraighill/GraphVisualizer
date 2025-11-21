<br />
<div align="center">
  <a href="https://github.com/bencraighill/GraphVisualizer.git">
    <img src="/Resrouces/Branding/Logo.png" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">GraphVisualizer</h3>

  <p align="center">
    An awesome Visualier to demonstrate Algorithm
  </p>
</div>

# Getting Started
## Installation
Clone the repo and all its submodules
```bash
git clone --recurse-submodules https://github.com/bencraighill/GraphVisualizer.git
```
If previously cloned non-recursively, clone the necessary submodules
```bash
git submodule update --init
```

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
![Startup](/Resources/Branding/Screenshots/Startup.png)
![Generate_Path](/Resources/Branding/Screenshots/GeneratePath.png)

# Acknowledgments
- ![imgui](https://github.com/ocornut/imgui.git)
- ![glfw](https://github.com/glfw/glfw.git)
- ![yaml-cpp](https://github.com/jbeder/yaml-cpp.git)
- ![libtinyfiledialogs](https://github.com/native-toolkit/libtinyfiledialogs.git)
