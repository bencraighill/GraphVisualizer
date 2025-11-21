// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <glad/glad.h>

#define TEMP_FILE_NAME "network.algograph"
#define GENERATE_SCRIPT_PATH "Resources/Scripts/generate_graph.py"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

#include "text_symbols.hpp"

#include "memory_tracker.hpp"

#include "algorithm.hpp"
#include "algorithms/BFS.hpp"
#include "algorithms/DFS.hpp"
#include "algorithms/DijkstraArray.hpp"
#include "algorithms/DijkstraQueue.hpp"
#include "algorithms/DEsopoPape.hpp"
#include "algorithms/BellmanFord.hpp"
#include "algorithms/FloydWarshall.hpp"

#include <vector>
#include <unordered_set>
#include <queue>
#include <fstream>
#include <chrono>
#include <cmath>

#include <yaml-cpp/yaml.h>
#include <tinyfiledialogs.h>
#include <filesystem>
#include <iostream>
#include <numeric>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static void Init();
static void OnUpdate();
static void OnImGuiRender();

// TODO: Remove
#define __APPLE__

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Graph Visualizer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return 1;
    }

    glfwMaximizeWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 17.0f);

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 25.0f;
    //static const ImWchar icon_ranges[] = { min_range, max_range, 0 };
    ImWchar* icon_ranges = new ImWchar[]{ (ImWchar)0xE000, (ImWchar)0xF8FF, 0 };
    ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 25.0f, &config, icon_ranges);

    ImWchar* other_ranges = new ImWchar[]{ (ImWchar)0xf041, (ImWchar)0xf041, (ImWchar)0xf3c5,(ImWchar)0xf3c5, (ImWchar)0xf64f, (ImWchar)0xf64f, (ImWchar)0xf894, (ImWchar)0xf894, (ImWchar)0xf640, (ImWchar)0xf640, 0 };
    io.Fonts->AddFontFromFileTTF("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 60, nullptr, other_ranges);
    
    io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Bold.ttf", 18.0f);
    ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 27.0f, &config, icon_ranges);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
#endif

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;

		style.WindowRounding = 5.0f;
		style.FrameRounding = 3.0f;
		style.PopupRounding = 5.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    Init();

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    // Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        OnUpdate();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw using ImGui
        OnImGuiRender();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Config
constexpr float PinDragThreshold = 20.0f;
constexpr float PlaybackCycleTime = 5.0f;
constexpr float PlaybackPaddingPercentage = 0.1f;
constexpr ImGuiMouseButton SelectMouseButton = ImGuiMouseButton_Left;
constexpr ImGuiMouseButton DragMouseButton = ImGuiMouseButton_Right;
constexpr float ZoomFactor = 1.1f;

static ImVec2 viewport_size = { 1920, 1080 };

static ImVec2 s_SourcePinPosition = { 100, 100 };
static ImVec2 s_TargetPinPosition = { 500, 100 };

static float s_VertexRadius = 5.0f;
static float s_EdgeThickness = 2.0f;
static float s_PlaybackSpeed = 1.0f;

static constexpr size_t NumGraceFrames = 3;

static float s_PreviousIOTime;
static float s_Time = 0.0f;
static size_t s_GraceFrames = NumGraceFrames;
static bool s_Paused = true;
static bool s_Loop = false;

static bool s_ShowEdgeLabels = false;
static bool s_ShowTraversalPaths = true;
static bool s_ShowFinalPaths = true;

static bool s_TrackMemory = true;
static float s_MemoryTrackingInterval = 10.0f; // ms

enum class DragType
{
    None,
    Viewport,
    Source,
    Target,
    Vertex,
    Edge,
};

struct DragContext
{
    DragType Type = DragType::None;
    int Index = -1;
};

static ImVec2 viewport_offset = { 0.0f, 0.0f };
static float viewport_zoom = 1.0f;
static ImVec2 drag_start_pos;
static ImVec2 drag_start_offset;
static DragContext s_DragContext;

static DragContext s_PopupContext;
static ImVec2 s_PopupWorldPosition;

static int s_AddEdgeIndex = -1;

static std::array<ImVec4, AlgorithmTypeCount> s_AlgorithmCompletedColors = {{
    { 0.941f, 0.394f, 0.082f, 1.0f },
    { 0.271f, 0.569f, 0.910f, 1.0f },
    { 0.941f, 0.082f, 0.082f, 1.0f },
    { 0.354f, 0.952f, 0.119f, 1.0f },
    { 0.905f, 0.072f, 0.940f, 1.0f },
    { 0.972f, 0.965f, 0.118f, 1.0f },
    { 0.080f, 0.859f, 0.821f, 1.0f },
}};

static std::array<ImVec4, AlgorithmTypeCount> s_AlgorithmTraversedColors;
static std::array<GLint, AlgorithmTypeCount> s_AlgorithmVisible;
static std::array<bool, AlgorithmTypeCount> s_AlgorithmTrackMemory;
static std::array<float, AlgorithmTypeCount> s_AlgorithmThickness;
static std::array<bool, AlgorithmTypeCount> s_AlgorithmEnabled = { true, true };

static std::array<size_t, AlgorithmTypeCount> s_SortedIndices;

static int s_PreviousHoveredEdge = -1;
static float s_HoveredEdgeTimer = 0.0f;

enum class GenerationType
{
    None,
    City,
    Text,
    Random,
};

struct GenerationData
{
    struct Address
    {
        std::string Suburb;
        std::string City;
        std::string State;
        std::string Country;
    };

    enum class CityTravelType
    {
        Drive,
        Walk,
        Bike,
        All,
    };

    std::vector<Address> Addresses = { { "Kensington", "Sydney", "NSW", "Australia" }};
    CityTravelType TravelType = CityTravelType::Drive;
    bool SimplifiedGraph = false;
    bool Append = false;
    float Scale = 1.0f;
};

static GenerationType s_GenerationType = GenerationType::None;
static GenerationData s_GenerationData;

static GLuint framebuffer = 0;
static ImVec2 framebuffer_size;
static GLuint color_tex = 0;
static GLuint depth_rbo = 0;

static GLuint circle_shader = 0;
static GLuint circle_vao = 0;
static GLuint circle_vbo = 0;
static GLuint instance_vbo = 0;

static GLuint line_shader = 0;
static GLuint line_vao = 0;
static GLuint line_vbo = 0;

struct VertexInstance
{
    ImVec2 Position;
};

struct EdgeVertex
{
    ImVec2 Position;
    ImVec2 Normal;
    std::array<float, AlgorithmTypeCount> TraversalTimes{};
    std::array<float, AlgorithmTypeCount> CompletionTimes{};

    EdgeVertex(const ImVec2& position, const ImVec2& normal)
        : Position(position), Normal(normal)
    {
        TraversalTimes.fill(-1.0f);
        CompletionTimes.fill(-1.0f);
    }
};

struct Edge
{
    std::string Name;
    uint32_t IndexA;
    uint32_t IndexB;
    // Note: weight is simply the distance between the two points

    Edge() = default;
    Edge(uint32_t indexA, uint32_t indexB)
        : IndexA(indexA), IndexB(indexB)
    {}

	Edge(const std::string& name, uint32_t indexA, uint32_t indexB)
		: Name(name), IndexA(indexA), IndexB(indexB)
    {}
};

struct SourceGraph
{
    std::vector<VertexInstance> Vertices;
    std::vector<Edge> Edges;
};

struct DrawGraphAlgorithmMetadata
{
    bool Valid = false;
    float Duration = 0.0f;
    float TotalDistance = 0.0f;
    size_t PeakMemoryUsage = 0.0f;
    float GraphTraversalPercentage = 0.0f;
    std::vector<size_t> MemoryTrackingData;
};

struct DrawGraph
{
    std::vector<VertexInstance> Vertices;
    std::vector<EdgeVertex> EdgeVertices;
    float Duration;

    std::array<DrawGraphAlgorithmMetadata, AlgorithmTypeCount> Metadata;
};

static SourceGraph s_SourceGraph;
static DrawGraph s_DrawGraph;

static void RegenerateGraph();
static void RegenerateTimedGraph();

static float Distance(const ImVec2& a, const ImVec2& b)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
}

static void RecomputeTraversalGPUData()
{
	for (size_t index = 0; index < AlgorithmTypeCount; index++)
		s_AlgorithmTraversedColors[index] = s_AlgorithmCompletedColors[index] * 0.4f + ImVec4(0.6f, 0.6f, 0.6f, 1.0f);

    glUseProgram(line_shader);

	GLint traversal_colors_loc = glGetUniformLocation(line_shader, "u_TraversalColors");
	glUniform4fv(traversal_colors_loc, AlgorithmTypeCount, (const GLfloat*)s_AlgorithmTraversedColors.data());

	GLint completed_colors_loc = glGetUniformLocation(line_shader, "u_CompletedColors");
	glUniform4fv(completed_colors_loc, AlgorithmTypeCount, (const GLfloat*)s_AlgorithmCompletedColors.data());

	GLint visible_loc = glGetUniformLocation(line_shader, "u_Visible");
	glUniform1iv(visible_loc, AlgorithmTypeCount, s_AlgorithmVisible.data());

	GLint thicknesses_loc = glGetUniformLocation(line_shader, "u_Thicknesses");
	glUniform1fv(thicknesses_loc, AlgorithmTypeCount, s_AlgorithmThickness.data());
}

static void AddVertex(const ImVec2& position)
{
	VertexInstance v;
	v.Position = position;
	s_SourceGraph.Vertices.push_back(v);

    RegenerateGraph();
}

static void DeleteVertex(const int index)
{
	if (index < 0 || index >= (int)s_SourceGraph.Vertices.size())
		return;

	// Remove edges connected to this vertex
	auto& edges = s_SourceGraph.Edges;
	edges.erase(std::remove_if(edges.begin(), edges.end(), [index](const Edge& e)
	{
		return e.IndexA == index || e.IndexB == index;
	}), edges.end());

	// Adjust edge indices greater than removed vertex
	for (auto& e : s_SourceGraph.Edges)
	{
		if (e.IndexA > index) e.IndexA--;
		if (e.IndexB > index) e.IndexB--;
	}

	// Remove vertex
	s_SourceGraph.Vertices.erase(s_SourceGraph.Vertices.begin() + index);

    RegenerateGraph();
}

static void AddEdge(const int indexA, const int indexB)
{
	if (indexA == indexB || indexA < 0 || indexB < 0 || indexA >= (int)s_SourceGraph.Vertices.size() || indexB >= (int)s_SourceGraph.Vertices.size())
		return;

	// Prevent duplicate edges
	for (const auto& e : s_SourceGraph.Edges)
	{
		if ((e.IndexA == indexA && e.IndexB == indexB) || (e.IndexA == indexB && e.IndexB == indexA))
			return;
	}

	Edge edge;
	edge.IndexA = indexA;
	edge.IndexB = indexB;
	s_SourceGraph.Edges.push_back(edge);

    RegenerateGraph();
}

static void DeleteEdge(const int index)
{
	if (index < 0 || index >= (int)s_SourceGraph.Edges.size())
		return;

	s_SourceGraph.Edges.erase(s_SourceGraph.Edges.begin() + index);

    RegenerateGraph();
}

static const char* AlgorithmTypeToString(const AlgorithmType type)
{
    switch (type)
    {
        case AlgorithmType::BFS:            return "BFS";
        case AlgorithmType::DFS:            return "DFS";
        case AlgorithmType::DijkstraArray:  return "Dijkstra (Array)";
        case AlgorithmType::DijkstraQueue:  return "Dijkstra (Priority Queue)";
        case AlgorithmType::DEsopoPape:     return "D'Esopo-Pape";
        case AlgorithmType::BellmanFord:    return "Bellman-Ford";
        case AlgorithmType::FloydWarshall:  return "Floyd-Warshall";
    }

    return "Unknown";
}

static GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        printf("Shader compilation failed: %s\n", log);
    }

    return shader;
}

static GLuint CompileProgram(const char* vertex, const char* fragment)
{
    const GLuint vs = CompileShader(GL_VERTEX_SHADER, vertex);
    const GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        printf("Shader linking failed: %s\n", log);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

static GLuint CreateCircleShader()
{
    const char* vertex = R"(
        #version 410 core

        layout(location = 0) in vec2 a_Position;
        layout(location = 1) in vec2 a_InstancePos;

        layout(location = 0) out vec2 v_LocalPos;

        uniform vec2 u_ViewportSize;
        uniform vec2 u_ViewportOffset;
        uniform float u_ViewportZoom;
        uniform float u_VertexRadius;

        void main()
        {
            v_LocalPos = a_Position;

            // Convert to screen space with offset
            vec2 worldPos = (a_InstancePos + a_Position * u_VertexRadius) * u_ViewportZoom + u_ViewportOffset;
            vec2 ndc = (worldPos / u_ViewportSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
        }
    )";

    const char* fragment = R"(
        #version 410 core

        layout(location = 0) in vec2 v_LocalPos;
        layout(location = 0) out vec4 FragColor;

        void main()
        {
            float dist = length(v_LocalPos);
            if (dist > 1.0)
                discard;

            // Smooth anti-aliasing
            float alpha = 1.0 - smoothstep(0.95, 1.0, dist);
            FragColor = vec4(1.0, 1.0, 1.0, alpha);
        }
    )";

    return CompileProgram(vertex, fragment);
}

static GLuint CreateLineShader()
{
    const char* vertex = R"(
        #version 410 core

        layout(location = 0) in vec2 a_Position;
        layout(location = 1) in vec2 a_Normal;
        layout(location = 2) in float a_TraversalTimes[7];
        layout(location = 9) in float a_CompletionTimes[7];

        layout(location = 0) out vec3 v_Color;

        uniform vec2 u_ViewportSize;
        uniform vec2 u_ViewportOffset;
        uniform float u_ViewportZoom;
        uniform float u_EdgeThickness;
        uniform float u_Time;

        uniform int u_ShowTraversalPaths;
        uniform int u_ShowFinalPaths;

        uniform vec4 u_TraversalColors[7];
        uniform vec4 u_CompletedColors[7];
        uniform int u_Visible[7];
        uniform float u_Thicknesses[7];

        void main()
        {
            vec3 colorSum = vec3(0.0);
            float thickness_weight = 0.5;
            float count = 0.0;
            
            for (int index = 0; index < 7; index++)
            {
                if (u_Visible[index] == 0)
                    continue;
                
                bool in_complete = (u_ShowFinalPaths != 0) && (a_CompletionTimes[index] >= 0.0) && (u_Time >= a_CompletionTimes[index]);
                bool in_traversed = (u_ShowTraversalPaths != 0) && (a_TraversalTimes[index] >= 0.0) && (u_Time >= a_TraversalTimes[index]);
                float t;

                float alpha = u_CompletedColors[index].a;

                if (in_complete)
                {
                    colorSum += u_CompletedColors[index].xyz * alpha;
                    t = 2.0;
                    count += alpha;
                }
                else if (in_traversed)
                {
                    colorSum += u_TraversalColors[index].xyz * alpha;
                    t = 1.0;
                    count += alpha;
                }

                thickness_weight = max(thickness_weight, t * u_Thicknesses[index]);
            }

            v_Color = count > 0 ? colorSum / count : vec3(0.5);
            float thickness = thickness_weight * u_EdgeThickness;

            vec2 worldPos = a_Position * u_ViewportZoom + u_ViewportOffset + (a_Normal * thickness);
            vec2 ndc = (worldPos / u_ViewportSize) * 2.0 - 1.0;
            ndc.y = -ndc.y;

            gl_Position = vec4(ndc, 0.0, 1.0);
        }
    )";

    const char* fragment = R"(
        #version 410 core

        layout(location = 0) in vec3 v_Color;
        layout(location = 0) out vec4 FragColor;

        void main()
        {
            FragColor = vec4(v_Color, 1.0);
        }
    )";

    return CompileProgram(vertex, fragment);
}

static void CreateCircleGeometry()
{
    // Create a quad that covers the circle (-1 to 1)
    float vertices[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &circle_vao);
    glBindVertexArray(circle_vao);

    // Vertex buffer (quad)
    glGenBuffers(1, &circle_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    // Instance buffer
    glGenBuffers(1, &instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);

    // Position attribute (per instance)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexInstance), (void*)0);
    glVertexAttribDivisor(1, 1);

    //// Radius attribute (per instance)
    //glEnableVertexAttribArray(2);
    //glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(VertexInstance), (void*)(2 * sizeof(float)));
    //glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
}

static void CreateLineGeometry()
{
    glGenVertexArrays(1, &line_vao);
    glBindVertexArray(line_vao);

    glGenBuffers(1, &line_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);

    glEnableVertexAttribArray(0); // a_Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)offsetof(EdgeVertex, Position));

    glEnableVertexAttribArray(1); // a_Normal
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)offsetof(EdgeVertex, Normal));

	for (int i = 0; i < AlgorithmTypeCount; i++)
	{
        // a_TraversalTimes[i]
		glEnableVertexAttribArray(2 + i);
		glVertexAttribPointer(2 + i, 1, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)(offsetof(EdgeVertex, TraversalTimes) + i * sizeof(float)));

        // a_CompletionTimes[i]
		glEnableVertexAttribArray(2 + AlgorithmTypeCount + i);
		glVertexAttribPointer(2 + AlgorithmTypeCount + i, 1, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)(offsetof(EdgeVertex, CompletionTimes) + i * sizeof(float)));
	}

    glBindVertexArray(0);
}

static void CreateFramebuffer(const ImVec2& size)
{
    if (framebuffer)
    {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &color_tex);
        glDeleteRenderbuffers(1, &depth_rbo);
    }

    framebuffer_size = size;

    // Create framebuffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create color texture
    glGenTextures(1, &color_tex);
    glBindTexture(GL_TEXTURE_2D, color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebuffer_size.x, framebuffer_size.y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, color_tex, 0);

    // Create depth renderbuffer
    glGenRenderbuffers(1, &depth_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer_size.x, framebuffer_size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, depth_rbo);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("Framebuffer not complete!\n");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static DrawGraph CreateDrawGraph(const SourceGraph& graph);
static DrawGraph CreateTimedDrawGraph(uint32_t source, uint32_t destination, const SourceGraph& graph);

static void UpdateDrawGraphGPUSide()
{
	// Update what the GPU data sees
	glBindVertexArray(line_vao);
	glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
	glBufferData(GL_ARRAY_BUFFER, s_DrawGraph.EdgeVertices.size() * sizeof(EdgeVertex), s_DrawGraph.EdgeVertices.data(), GL_DYNAMIC_DRAW);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(GL_ARRAY_BUFFER, s_DrawGraph.Vertices.size() * sizeof(VertexInstance), s_DrawGraph.Vertices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void RegenerateGraph()
{
	s_DrawGraph = CreateDrawGraph(s_SourceGraph);
	UpdateDrawGraphGPUSide();

    s_Time = 0.0f;
    s_GraceFrames = NumGraceFrames;
    s_Paused = true;
}

static void RegenerateTimedGraph()
{
    if (s_SourceGraph.Vertices.size() < 2 || s_SourceGraph.Edges.size() < 1)
    {
        RegenerateGraph();
        return;
    }

	// Find closest point to source and target
	uint32_t source = 0;
	float best_source_distance = Distance(s_SourceGraph.Vertices.front().Position, s_SourcePinPosition);

	uint32_t target = 0;
	float best_target_distance = Distance(s_SourceGraph.Vertices.front().Position, s_TargetPinPosition);

	for (size_t index = 0; index < s_SourceGraph.Vertices.size(); index++)
	{
		const auto& vertex = s_SourceGraph.Vertices[index];

		const float source_distance = Distance(vertex.Position, s_SourcePinPosition);
		if (source_distance < best_source_distance)
		{
			best_source_distance = source_distance;
			source = index;
		}

		const float target_distance = Distance(vertex.Position, s_TargetPinPosition);
		if (target_distance < best_target_distance)
		{
			best_target_distance = target_distance;
			target = index;
		}
	}

	s_DrawGraph = CreateTimedDrawGraph(source, target, s_SourceGraph);
    UpdateDrawGraphGPUSide();
}

static void NewGraph()
{
	s_SourcePinPosition = { 100, 100 };
	s_TargetPinPosition = { 500, 100 };

	s_SourceGraph.Vertices.clear();
	s_SourceGraph.Edges.clear();
	RegenerateGraph();
}

static bool LoadGraph(const char* filepath)
{
	YAML::Node config;
	try
	{
		config = YAML::LoadFile(filepath);
	}
	catch (...)
	{
		std::cerr << "Failed to load graph visualizer file: " << filepath << std::endl;
		return false;
	}

	if (std::filesystem::exists(TEMP_FILE_NAME))
		std::filesystem::remove(TEMP_FILE_NAME);

    const uint32_t offset = s_SourceGraph.Vertices.size();

	for (const auto& node : config["nodes"])
	{
		const float x = node["x"].as<float>();
		const float y = node["y"].as<float>();
		s_SourceGraph.Vertices.push_back({ ImVec2(x, y) });
	}

	for (const auto& edge : config["edges"])
	{
        const auto nameNode = edge["name"];
		const uint32_t source = edge["source"].as<uint32_t>();
		const uint32_t target = edge["target"].as<uint32_t>();
        s_SourceGraph.Edges.emplace_back(nameNode ? nameNode.as<std::string>() : std::string(), offset + source, offset + target);
	}

	std::cout << "Loaded " << s_SourceGraph.Vertices.size() << " nodes and " << s_SourceGraph.Edges.size() << " edges\n";
	return true;
}

static const char* filters[] = { "*.algograph" };

static bool OpenGraph()
{
	const char* filepath = tinyfd_openFileDialog(
		"Open a graph visualizer file",
		"",
		1,
		filters,
		"Graph Network Visualizer File (.algograph)",
		0
	);

    NewGraph();
    const bool result = LoadGraph(filepath);
	RegenerateGraph();
	return result;
}

static bool AppendGraph()
{
	const char* filepath = tinyfd_openFileDialog(
		"Append a graph visualizer file",
		"",
		1,
		filters,
		"Graph Network Visualizer File (.algograph)",
		0
	);

    const bool result = LoadGraph(filepath);
    RegenerateGraph();
    return result;
}

static bool SaveGraph()
{
	const char* filepath = tinyfd_saveFileDialog(
		"Save a graph visualizer file",
		"network.algograph",
		1,
		filters,
		"Graph Network Visualizer File (.algograph)"
	);

    if (!filepath || strlen(filepath) == 0)
        return false;

	YAML::Emitter out;
	out << YAML::BeginMap;

	// Serialize nodes
	out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
	for (const auto& vertex : s_SourceGraph.Vertices)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "x" << YAML::Value << vertex.Position.x;
		out << YAML::Key << "y" << YAML::Value << vertex.Position.y;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	// Serialize edges
	out << YAML::Key << "edges" << YAML::Value << YAML::BeginSeq;
	for (const auto& edge : s_SourceGraph.Edges)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "source" << YAML::Value << edge.IndexA;
		out << YAML::Key << "target" << YAML::Value << edge.IndexB;

        if (!edge.Name.empty())
            out << YAML::Key << "name" << YAML::Value << edge.Name;

		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	out << YAML::EndMap;

	try
	{
		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			return false;
		}

		fout << out.c_str();
		std::cout << "Saved " << s_SourceGraph.Vertices.size() << " nodes and " << s_SourceGraph.Edges.size() << " edges to " << filepath << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error saving graph: " << e.what() << std::endl;
		return false;
	}

	return true;
}

static void Init()
{
	std::cout << "Current working directory: "
		<< std::filesystem::current_path() << "\n";

    std::iota(s_SortedIndices.begin(), s_SortedIndices.end(), 0);

    s_PreviousIOTime = ImGui::GetTime();

    circle_shader = CreateCircleShader();
    CreateCircleGeometry();

    line_shader = CreateLineShader();
    CreateLineGeometry();

    s_AlgorithmVisible.fill(true);
    s_AlgorithmThickness.fill(1.0f);
    s_AlgorithmTrackMemory.fill(true);
    RecomputeTraversalGPUData();

    NewGraph();

#if 0
    // Generate a test graph
    int numVertices = 20;
    float radius = 300.0f;

    // Random vertices
    for (uint32_t i = 0; i < numVertices; i++)
    {
        float x = 200.0f + (rand() % 800);
        float y = 200.0f + (rand() % 800);
        float r = 20.0f + (rand() % 50);
        s_SourceGraph.Vertices.push_back({ {x, y}, 5.0f });
    }

    // Connect closest neighbors
    for (uint32_t i = 0; i < numVertices; i++)
    {
        for (uint32_t j = i + 1; j < numVertices; j++)
        {
            float dx = s_SourceGraph.Vertices[i].Position.x - s_SourceGraph.Vertices[j].Position.x;
            float dy = s_SourceGraph.Vertices[i].Position.y - s_SourceGraph.Vertices[j].Position.y;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist < radius)
                s_SourceGraph.Edges.push_back({ i, j });
        }
    }
#endif

    RegenerateGraph();
}

static bool LoadCityGraph(const std::vector<GenerationData::Address>& addresses, const GenerationData::CityTravelType travelType, bool simplify, float scale)
{
    std::ostringstream oss;

    // Locations: combine "Suburb, City, State, Country"
	if (!addresses.empty())
	{
		oss << "--location ";
		for (size_t i = 0; i < addresses.size(); ++i)
		{
			const auto& a = addresses[i];
			std::string loc = a.Suburb + ", " + a.City + ", " + a.State + ", " + a.Country;

			// Quote if contains spaces
			if (loc.find(' ') != std::string::npos)
				loc = "\"" + loc + "\"";

			oss << loc;
			if (i + 1 < addresses.size())
				oss << " ";
		}
		oss << " ";
	}

	// Network type
	std::string typeStr;
	switch (travelType)
	{
	case GenerationData::CityTravelType::Drive: typeStr = "drive"; break;
	case GenerationData::CityTravelType::Walk: typeStr = "walk"; break;
	case GenerationData::CityTravelType::Bike: typeStr = "bike"; break;
	case GenerationData::CityTravelType::All: typeStr = "all"; break;
	}
	oss << "--network-type " << typeStr << " ";

	// Output file
	oss << "--output-file " << TEMP_FILE_NAME << " ";

	// Normalize range
	oss << "--normalize-range " << 2000.0f * scale << " ";

	// Simplified graph
	if (simplify)
		oss << "--simplified-graph ";

    if (std::filesystem::exists(TEMP_FILE_NAME))
        std::filesystem::remove(TEMP_FILE_NAME);

#ifdef _WIN32
    std::string cmd = "python " GENERATE_SCRIPT_PATH " " + oss.str();
#else
    std::string cmd = "python3 " GENERATE_SCRIPT_PATH " " + oss.str();
#endif

    const int status = std::system(cmd.c_str());
    if (status != 0)
    {
        std::cout << "Search query failed for the specified location" << std::endl;
        return false;
    }

    LoadGraph("network.algograph");
}

static float GetPlaybackDuration()
{
    return s_DrawGraph.Duration * (1.0f + PlaybackPaddingPercentage);
}

static void OnUpdate()
{
    if (!framebuffer || framebuffer_size != viewport_size)
        CreateFramebuffer(viewport_size);

    const float time = ImGui::GetTime();
    const float deltaTime = time - s_PreviousIOTime;
    s_PreviousIOTime = time;

    const float duration = GetPlaybackDuration();

    s_HoveredEdgeTimer += deltaTime;

    if (!s_Paused)
    {
        if (s_GraceFrames > 0)
        {
            s_GraceFrames--;
        }
        else
        {
            s_Time += (deltaTime / PlaybackCycleTime) * duration * s_PlaybackSpeed;
        }
    }

    while (s_Time > duration)
    {
        s_Time = s_Loop ? (s_Time - duration) : duration;
    }

    // Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, (int)viewport_size.x, (int)viewport_size.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable blending for smooth circles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    auto& graph = s_DrawGraph;
    auto& edge_vertices = graph.EdgeVertices;
    auto& vertices = graph.Vertices;

    // Draw edges
    if (!edge_vertices.empty() && !vertices.empty())
    {
        glUseProgram(line_shader);

        GLint viewport_loc = glGetUniformLocation(line_shader, "u_ViewportSize");
        glUniform2f(viewport_loc, viewport_size.x, viewport_size.y);

        GLint offset_loc = glGetUniformLocation(line_shader, "u_ViewportOffset");
        glUniform2f(offset_loc, viewport_offset.x, viewport_offset.y);

        GLint zoom_loc = glGetUniformLocation(line_shader, "u_ViewportZoom");
        glUniform1f(zoom_loc, viewport_zoom);

        GLint thickness_loc = glGetUniformLocation(line_shader, "u_EdgeThickness");
        glUniform1f(thickness_loc, s_EdgeThickness);

        GLint time_loc = glGetUniformLocation(line_shader, "u_Time");
        glUniform1f(time_loc, s_Time);

		GLint show_traversal_paths_loc = glGetUniformLocation(line_shader, "u_ShowTraversalPaths");
		glUniform1i(show_traversal_paths_loc, s_ShowTraversalPaths);

		GLint show_final_paths_loc = glGetUniformLocation(line_shader, "u_ShowFinalPaths");
		glUniform1i(show_final_paths_loc, s_ShowFinalPaths);

        glBindVertexArray(line_vao);
        glDrawArrays(GL_TRIANGLES, 0, edge_vertices.size());
        glBindVertexArray(0);
    }

    // Draw circles
    if (!vertices.empty() && s_VertexRadius > 0.0f)
    {
        glUseProgram(circle_shader);

        // Set uniforms
        GLint viewport_loc = glGetUniformLocation(circle_shader, "u_ViewportSize");
        glUniform2f(viewport_loc, viewport_size.x, viewport_size.y);

        GLint offset_loc = glGetUniformLocation(circle_shader, "u_ViewportOffset");
        glUniform2f(offset_loc, viewport_offset.x, viewport_offset.y);

        GLint zoom_loc = glGetUniformLocation(circle_shader, "u_ViewportZoom");
        glUniform1f(zoom_loc, viewport_zoom);

        GLint radius_loc = glGetUniformLocation(circle_shader, "u_VertexRadius");
        glUniform1f(radius_loc, s_VertexRadius);

        // Draw
        glBindVertexArray(circle_vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, vertices.size());
        glBindVertexArray(0);
    }

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static bool DrawDiamond(ImDrawList* drawList, const ImVec2& center, const float size, const ImU32 color)
{
	const float radius = size * 0.5f;
	ImVec2 pts[4] = {
		ImVec2(center.x,     center.y - radius),
		ImVec2(center.x + radius, center.y),
		ImVec2(center.x,     center.y + radius),
		ImVec2(center.x - radius, center.y)
	};

    drawList->AddConvexPolyFilled(pts, 4, color);

    const ImVec2 mouse = ImGui::GetMousePos();
	const float dx = mouse.x - center.x;
	const float dy = mouse.y - center.y;
    return (dx * dx + dy * dy) <= (radius * radius);
}

static bool ImGuiSequencer(float& currentTime, float minTime, float maxTime, float height = 80.0f)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID("##sequencer");

    const ImVec2 canvas_pos = window->DC.CursorPos;
    const float canvas_width = ImGui::GetContentRegionAvail().x;
    const ImVec2 canvas_size(canvas_width, height);

    ImRect bb(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool value_changed = false;

    // Handle mouse interaction for dragging
    bool hovered = !ImGui::IsPopupOpen(ImGuiID(0), ImGuiPopupFlags_AnyPopupId) && ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

    if (hovered && ImGui::IsMouseClicked(0)) {
        ImGui::SetActiveID(id, window);
    }

    if (ImGui::GetActiveID() == id) {
        if (ImGui::IsMouseDown(0)) {
            ImVec2 mouse_pos = ImGui::GetMousePos();
            float normalized = (mouse_pos.x - canvas_pos.x) / canvas_width;
            normalized = ImClamp(normalized, 0.0f, 1.0f);
            currentTime = minTime + normalized * (maxTime - minTime);
            value_changed = true;
        }
        else {
            ImGui::ClearActiveID();
        }
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Background
    const ImU32 bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
    const ImU32 line_color = ImGui::GetColorU32(ImGuiCol_Border);
    const ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 playhead_color = ImGui::GetColorU32(ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), bg_color);
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), line_color);

    // Calculate interval spacing
    float range = maxTime - minTime;
    float magnitude = std::pow(10.0f, std::floor(std::log10(range)));
    float normalized_range = range / magnitude;

    float interval;
    if (normalized_range <= 1.0f) interval = 0.1f * magnitude;
    else if (normalized_range <= 2.0f) interval = 0.25f * magnitude;
    else if (normalized_range <= 5.0f) interval = 0.5f * magnitude;
    else interval = 1.0f * magnitude;

    // Draw time intervals
    float timeline_top = canvas_pos.y + 35.0f;
    float tick_height_major = 15.0f;
    float tick_height_minor = 8.0f;

    float start_time = std::ceil(minTime / interval) * interval;

    for (float t = start_time; t <= maxTime; t += interval) {
        float x = canvas_pos.x + ((t - minTime) / range) * canvas_width;

        // Major tick
        draw_list->AddLine(
            ImVec2(x, timeline_top),
            ImVec2(x, timeline_top + tick_height_major),
            line_color, 1.5f
        );

        // Time label - clamp to visible area
        char label[32];
        snprintf(label, sizeof(label), "%.2f ns", t);
        ImVec2 text_size = ImGui::CalcTextSize(label);
        float text_x = x - text_size.x * 0.5f;
        // Clamp label position to stay within bounds
        text_x = ImClamp(text_x, canvas_pos.x + 2.0f, canvas_pos.x + canvas_width - text_size.x - 2.0f);
        draw_list->AddText(
            ImVec2(text_x, canvas_pos.y + 5.0f),
            text_color,
            label
        );

        // Minor ticks (4 subdivisions between major ticks)
        for (int i = 1; i < 4; i++) {
            float minor_t = t + (interval * i / 4.0f);
            if (minor_t > maxTime) break;

            float minor_x = canvas_pos.x + ((minor_t - minTime) / range) * canvas_width;
            draw_list->AddLine(
                ImVec2(minor_x, timeline_top),
                ImVec2(minor_x, timeline_top + tick_height_minor),
                line_color, 1.0f
            );
        }
    }

    // Draw horizontal timeline
    draw_list->AddLine(
        ImVec2(canvas_pos.x, timeline_top),
        ImVec2(canvas_pos.x + canvas_width, timeline_top),
        line_color, 2.0f
    );

    // Draw Histogram
    if (s_TrackMemory)
    {
        const float graph_bottom = canvas_pos.y + canvas_size.y;
        const float graph_top = canvas_pos.y + 50.0f;
        const float graph_height = graph_bottom - graph_top;

        size_t maxMemory = 1;
        for (size_t index = 0; index < AlgorithmTypeCount; index++)
        {
            const auto& metadata = s_DrawGraph.Metadata[index];
            if (!metadata.Valid || !s_AlgorithmTrackMemory[index] || !s_AlgorithmVisible[index])
                continue;
            if (metadata.PeakMemoryUsage > maxMemory)
                maxMemory = metadata.PeakMemoryUsage;
        }

        for (size_t index = 0; index < AlgorithmTypeCount; index++)
        {
            const auto& metadata = s_DrawGraph.Metadata[index];
            if (!metadata.Valid || !s_AlgorithmTrackMemory[index] || metadata.MemoryTrackingData.empty())
                continue;

            const ImU32 color_line = ImGui::GetColorU32(s_AlgorithmCompletedColors[index]);
            const ImU32 color_fill = IM_COL32(
                ((color_line >> 0) & 0xFF),
                ((color_line >> 8) & 0xFF),
                ((color_line >> 16) & 0xFF),
                80
            );

            const size_t sampleCount = metadata.MemoryTrackingData.size();
            if (sampleCount < 2)
                continue;

            ImVector<ImVec2> points;
            points.reserve(sampleCount + 4);

			const float minTime = 0.0f;
			const float maxTime = s_DrawGraph.Duration;
			const float algorithmStart = 0.0f;
            const float algorithmEnd = metadata.Duration;
			const float algorithmRange = algorithmEnd - algorithmStart;
			const float timelineRange = maxTime - minTime;

            points.push_back(ImVec2(canvas_pos.x, graph_bottom));

            for (size_t i = 0; i < sampleCount; i++)
            {
				float fraction = (sampleCount == 1) ? 1.0f : (i / float(sampleCount - 1));
				float t = fraction * algorithmRange;

				float x = canvas_pos.x + ((t + algorithmStart - minTime) / timelineRange) * canvas_width;
				x = std::min(x, canvas_pos.x + ((algorithmEnd - minTime) / timelineRange) * canvas_width);

				float memFraction = metadata.MemoryTrackingData[i] / float(maxMemory);
				float y = graph_bottom - memFraction * graph_height;

                points.push_back(ImVec2(x, y));
            }

            const float xEnd = canvas_pos.x + ((algorithmEnd - minTime) / timelineRange) * canvas_width;
            points.push_back(ImVec2(xEnd, graph_bottom));
            draw_list->AddConcavePolyFilled(points.Data, points.size(), color_fill);
            draw_list->AddPolyline(points.Data + 1, points.size() - 2, color_line, 0, 2.0f);
        }
    }

    // Draw playhead
    float playhead_x = canvas_pos.x + ((currentTime - minTime) / range) * canvas_width;
    playhead_x = ImClamp(playhead_x, canvas_pos.x, canvas_pos.x + canvas_width);

    // Draw keyframes
	for (size_t index = 0; index < AlgorithmTypeCount; index++)
	{
        const auto& metadata = s_DrawGraph.Metadata[index];
		if (!metadata.Valid || !s_AlgorithmVisible[index] || metadata.Duration < minTime || metadata.Duration > maxTime)
			continue;

		const float x = canvas_pos.x + ((metadata.Duration - minTime) / (maxTime - minTime)) * canvas_width;
		if (DrawDiamond(draw_list, ImVec2(x, timeline_top), 10.0f, ImGui::GetColorU32(s_AlgorithmCompletedColors[index])))
            ImGui::SetTooltip(FA_CLOCK " %s finished in %.0f ms", AlgorithmTypeToString((AlgorithmType)index), metadata.Duration / 1'000'000.0f);
	}

    // Playhead arrow
    float arrow_width = 8.0f;
    float arrow_height = 10.0f;
    ImVec2 arrow_top(playhead_x, canvas_pos.y);
    ImVec2 arrow_left(playhead_x - arrow_width, canvas_pos.y + arrow_height);
    ImVec2 arrow_right(playhead_x + arrow_width, canvas_pos.y + arrow_height);

    draw_list->AddTriangleFilled(arrow_top, arrow_left, arrow_right, playhead_color);

    // Playhead line
    draw_list->AddLine(
        ImVec2(playhead_x, canvas_pos.y + arrow_height),
        ImVec2(playhead_x, canvas_pos.y + canvas_size.y),
        playhead_color, 2.0f
    );

    // Current time display
    char time_label[32];
    snprintf(time_label, sizeof(time_label), "Time: %.0f ms", currentTime / 1'000'000.0f);
    draw_list->AddText(
        ImVec2(canvas_pos.x + 5.0f, canvas_pos.y + canvas_size.y - 20.0f),
        text_color,
        time_label
    );

    return value_changed;
}

static ImVec2 ScreenToWorld(const ImVec2& screen_pos, const ImVec2& image_position)
{
    const ImVec2 viewport_pos = ImVec2(screen_pos.x - image_position.x, screen_pos.y - image_position.y);
    const ImVec2 world_pos = ImVec2((viewport_pos.x - viewport_offset.x) / viewport_zoom, (viewport_pos.y - viewport_offset.y) / viewport_zoom);
    return world_pos;
}

static ImVec2 WorldToScreen(const ImVec2& world_pos, const ImVec2& image_position)
{
    const ImVec2 viewport_pos = ImVec2(world_pos.x * viewport_zoom + viewport_offset.x, world_pos.y * viewport_zoom + viewport_offset.y);
    const ImVec2 screen_pos = ImVec2(viewport_pos.x + image_position.x, viewport_pos.y + image_position.y);
    return screen_pos;
}

static float DistancePointToSegment(const ImVec2& p, const ImVec2& a, const ImVec2& b)
{
	const float abx = b.x - a.x;
	const float aby = b.y - a.y;
	const float apx = p.x - a.x;
	const float apy = p.y - a.y;

	const float ab_len2 = abx * abx + aby * aby;
	if (ab_len2 == 0.0f)
		return sqrtf(apx * apx + apy * apy);

	float t = (apx * abx + apy * aby) / ab_len2;
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

	const float cx = a.x + t * abx;
	const float cy = a.y + t * aby;

	const float dx = p.x - cx;
	const float dy = p.y - cy;
	return sqrtf(dx * dx + dy * dy);
}

static int HitTestVertex(const ImVec2& mouse_pos, const ImVec2& image_position)
{
	const float threshold = s_VertexRadius * viewport_zoom * 1.5f;

	for (int i = 0; i < (int)s_SourceGraph.Vertices.size(); i++)
	{
		const ImVec2 screen = WorldToScreen(s_SourceGraph.Vertices[i].Position, image_position);
		if (Distance(mouse_pos, screen) <= threshold)
			return i;
	}

	return -1;
}

static int HitTestEdge(const ImVec2& mouse_pos, const ImVec2& image_position)
{
    const float threshold = 6.0f;

	for (int i = 0; i < (int)s_SourceGraph.Edges.size(); i++)
	{
		const auto& e = s_SourceGraph.Edges[i];

		const ImVec2 a = WorldToScreen(s_SourceGraph.Vertices[e.IndexA].Position, image_position);
		const ImVec2 b = WorldToScreen(s_SourceGraph.Vertices[e.IndexB].Position, image_position);

		if (DistancePointToSegment(mouse_pos, a, b) <= threshold)
			return i;
	}

	return -1;
}

static DragContext GetHoveredItem(const ImVec2& image_position)
{
    DragContext context;

	const ImVec2 mouse = ImGui::GetMousePos();

	const ImVec2 source_pin_screen = WorldToScreen(s_SourcePinPosition, image_position);
	const ImVec2 target_pin_screen = WorldToScreen(s_TargetPinPosition, image_position);

	if (Distance(mouse, source_pin_screen) < PinDragThreshold)
	{
        context.Type = DragType::Source;
        return context;
	}

	if (Distance(mouse, target_pin_screen) < PinDragThreshold)
	{
        context.Type = DragType::Target;
        return context;
	}

    // Vertex hit
	const int vertex_hit = HitTestVertex(mouse, image_position);
	if (vertex_hit >= 0)
	{
		context.Type = DragType::Vertex;
		context.Index = vertex_hit;
        return context;
	}

	// Edge hit
	const int edge_hit = HitTestEdge(mouse, image_position);
	if (edge_hit >= 0)
	{
		context.Type = DragType::Edge;
		context.Index = edge_hit;
		return context;
	}

    context.Type = DragType::None;
    return context;
}

static const char* GetClosestEdgeLabel(const ImVec2& position)
{
    const char* label = nullptr;
    float closest = std::numeric_limits<float>::max();

    for (const auto& edge : s_SourceGraph.Edges)
    {
        const auto& vertexA = s_SourceGraph.Vertices[edge.IndexA];
        const float distanceA = Distance(vertexA.Position, position);
        if (distanceA < closest)
        {
            closest = distanceA;
            label = !edge.Name.empty() ? edge.Name.c_str() : nullptr;
        }

        const auto& vertexB = s_SourceGraph.Vertices[edge.IndexB];
		const float distanceB = Distance(vertexB.Position, position);
		if (distanceB < closest)
		{
			closest = distanceB;
			label = !edge.Name.empty() ? edge.Name.c_str() : nullptr;
		}
    }

    return label;
}

static bool DrawBigTextButton(const char* id, const char* icon, const ImVec2& size)
{
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	const bool result = ImGui::Button(id, size);
	ImGui::GetWindowDrawList()->AddText(ImGui::GetItemRectMin() + ImVec2(8.5f, -15.0f), IM_COL32_WHITE, icon);
    ImGui::PopFont();

    return result;
}

static bool DrawTextButton(const char* label, const ImVec2& size = ImVec2(0, 0))
{
    ImGui::PushStyleColor(ImGuiCol_Button, {});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {});
    ImGui::PushStyleColor(ImGuiCol_Text, {});
    const bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor(4);

    ImGui::GetWindowDrawList()->AddText(ImGui::GetItemRectMin() + ImGui::GetStyle().FramePadding, ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonHovered : ImGui::IsItemHovered() ? ImGuiCol_TextDisabled : ImGuiCol_Text), label);

    return result;
}

static bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = ImGui::CalcItemWidth();

	const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
    ImGui::ItemSize(total_bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
		return false;

	// Default format string when passing NULL
	if (format == NULL)
		format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

	const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
	bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);
	if (!temp_input_is_active)
	{
		// Tabbing or CTRL+click on Slider turns it into an input box
		const bool clicked = hovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, id);
		const bool make_active = (clicked || g.NavActivateId == id);
		if (make_active && clicked)
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		if (make_active && temp_input_allowed)
			if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
				temp_input_is_active = true;

		// Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
		if (make_active)
			memcpy(&g.ActiveIdValueOnActivation, p_data, ImGui::DataTypeGetInfo(data_type)->Size);

		if (make_active && !temp_input_is_active)
		{
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		}
	}

	if (temp_input_is_active)
	{
		// Only clamp CTRL+Click input when ImGuiSliderFlags_ClampOnInput is set (generally via ImGuiSliderFlags_AlwaysClamp)
		const bool clamp_enabled = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
		return ImGui::TempInputScalar(frame_bb, id, label, data_type, p_data, format, clamp_enabled ? p_min : NULL, clamp_enabled ? p_max : NULL);
	}

	// Draw frame
	const ImU32 frame_col = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    ImGui::RenderNavCursor(frame_bb, id);
    //ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);
	const float vertical_center = (frame_bb.Min.y + frame_bb.Max.y) * 0.5f;
    ImGui::RenderFrame(ImVec2(frame_bb.Min.x, vertical_center - style.FramePadding.y), ImVec2(frame_bb.Max.x, vertical_center + style.FramePadding.y), frame_col, true, g.Style.FrameRounding);

	// Slider behavior
	ImRect grab_bb;
	const bool value_changed = ImGui::SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
	if (value_changed)
        ImGui::MarkItemEdited(id);

	// Render grab
	if (grab_bb.Max.x > grab_bb.Min.x)
		//window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        window->DrawList->AddCircleFilled((grab_bb.Min + grab_bb.Max) * 0.5f, grab_bb.GetHeight() * 0.55f - style.FramePadding.y, ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab));

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + ImGui::DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
	if (g.LogEnabled)
        ImGui::LogSetNextTextDecoration("{", "}");
    ImGui::RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

	if (label_size.x > 0.0f)
        ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
	return value_changed;
}

static bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
	return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

// Table Sorting logic
static ImGuiTableSortSpecs* g_SortSpecs = nullptr;

static int CompareAlgorithms(const void* lhs, const void* rhs)
{
	const int index1 = *(const int*)lhs;
	const int index2 = *(const int*)rhs;

	const DrawGraphAlgorithmMetadata& a = s_DrawGraph.Metadata[index1];
	const DrawGraphAlgorithmMetadata& b = s_DrawGraph.Metadata[index2];

	const ImGuiTableColumnSortSpecs* spec = g_SortSpecs->Specs;

	float delta = 0.0f;

	switch (spec->ColumnIndex)
	{
	case 0: delta = (float)(index1 - index2); break;
	case 1: delta = (a.Duration - b.Duration); break;
	case 2: delta = (a.TotalDistance - b.TotalDistance); break;
	case 3: delta = (a.PeakMemoryUsage - b.PeakMemoryUsage); break;
	case 4: delta = (a.GraphTraversalPercentage - b.GraphTraversalPercentage); break;
	}

	if (delta < 0) return spec->SortDirection == ImGuiSortDirection_Ascending ? -1 : +1;
	if (delta > 0) return spec->SortDirection == ImGuiSortDirection_Ascending ? +1 : -1;
	return 0;
}

static void OnImGuiRender()
{
    // Create a fullscreen dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockspace_window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, dockspace_window_flags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu(FA_FLOPPY_DISK " File"))
        {
            if (ImGui::MenuItem(FA_FILE_PLUS " New", "Ctrl+N"))
                NewGraph();
            if (ImGui::MenuItem(FA_FILE_IMPORT " Open", "Ctrl+O"))
                OpenGraph();
            if (ImGui::MenuItem(FA_FILE_EXPORT " Save", "Ctrl+S"))
                SaveGraph();
			if (ImGui::MenuItem(FA_LINK " Append", "Ctrl+A"))
				AppendGraph();
            ImGui::Separator();
            if (ImGui::BeginMenu(FA_GEAR " Generate"))
            {
                if (ImGui::MenuItem(FA_CITY " City Data"))
                    s_GenerationType = GenerationType::City;
                if (ImGui::MenuItem(FA_ABACUS " Random"))
                    s_GenerationType = GenerationType::Random;
                if (ImGui::MenuItem(FA_TEXT_SIZE " Text"))
                    s_GenerationType = GenerationType::Text;
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(FA_SKULL_CROSSBONES " Exit")) { exit(0); }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem(FA_ROUTE " Generate Route"))
        {
            RegenerateTimedGraph();
            s_Paused = false;
            s_Time = 0.0f;
            s_GraceFrames = NumGraceFrames;
        }

        if (ImGui::BeginMenu(FA_MAGNIFYING_GLASS " View"))
        {
            ImGui::DragFloat(FA_CIRCLE " Vertex Radius", &s_VertexRadius, 0.05f, 0.0f, 10.0f);
            ImGui::DragFloat(FA_DASH " Edge Thickness", &s_EdgeThickness, 0.05f, 0.25f, 5.0f);

            ImGui::Separator();

            ImGui::Checkbox(FA_TAG " Show Edge Labels", &s_ShowEdgeLabels);
            ImGui::Checkbox(FA_CHART_NETWORK " Show Traversal Paths", &s_ShowTraversalPaths);
            ImGui::Checkbox(FA_FLAG_CHECKERED " Show Final Paths", &s_ShowFinalPaths);

            ImGui::Separator();

            ImGui::Checkbox(FA_MEMORY " Track Memory", &s_TrackMemory);

            if (s_TrackMemory)
                ImGui::DragFloat(FA_STOPWATCH " Tracking Interval", &s_MemoryTrackingInterval, 1.0f, 0.1f, 50.0f, "%.3f ms");

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    // Initialize docking layout on first frame
    static bool first_time = true;
    if (first_time)
    {
        first_time = false;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

        // Split the dockspace into left and right
        ImGuiID dock_viewport, dock_controls, dock_timeline, dock_outline;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.65f, &dock_viewport, &dock_controls);
        ImGui::DockBuilderSplitNode(dock_viewport, ImGuiDir_Up, 0.75f, &dock_viewport, &dock_timeline);
        ImGui::DockBuilderSplitNode(dock_controls, ImGuiDir_Down, 0.6f, &dock_controls, &dock_outline);

        // Dock windows
        ImGui::DockBuilderDockWindow("Viewport", dock_viewport);
        ImGui::DockBuilderDockWindow("Controls", dock_controls);
        ImGui::DockBuilderDockWindow("Outline", dock_outline);
        ImGui::DockBuilderDockWindow("Timeline", dock_timeline);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoTabBar);
    ImGui::End();

    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Viewport", nullptr, window_flags);
    viewport_size = ImGui::GetContentRegionAvail();

    const ImVec2 image_position = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)color_tex, viewport_size, { 0, 1 }, { 1, 0 });
    const bool viewportHovered = ImGui::IsItemHovered();

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    const ImVec2 pin_size = ImGui::CalcTextSize(FA_LOCATION_PIN);
    ImGui::GetWindowDrawList()->AddText(WorldToScreen(s_SourcePinPosition, image_position) - ImVec2(pin_size.x * 0.5f, pin_size.y), IM_COL32(200, 130, 150, 255), FA_LOCATION_PIN);

    const ImVec2 dot_size = ImGui::CalcTextSize(FA_LOCATION_DOT);
    ImGui::GetWindowDrawList()->AddText(WorldToScreen(s_TargetPinPosition, image_position) - ImVec2(dot_size.x * 0.5f, dot_size.y), IM_COL32(150, 130, 200, 255), FA_LOCATION_DOT);
    ImGui::PopFont();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (s_AddEdgeIndex >= 0 && s_AddEdgeIndex < s_SourceGraph.Vertices.size())
    {
        const auto& sourceVertex = s_SourceGraph.Vertices[s_AddEdgeIndex];
        drawList->AddLine(WorldToScreen(sourceVertex.Position, image_position), ImGui::GetMousePos(), IM_COL32_WHITE, s_EdgeThickness);
    }

    // Show the labels of all streets if enabled
    if (s_ShowEdgeLabels)
    {
        for (const auto& edge : s_SourceGraph.Edges)
        {
            if (edge.Name.empty())
                continue;

            const auto& vertexA = s_SourceGraph.Vertices[edge.IndexA];
            const auto& vertexB = s_SourceGraph.Vertices[edge.IndexB];
            drawList->AddText(WorldToScreen((vertexA.Position + vertexB.Position) * 0.5f, image_position) - ImGui::CalcTextSize(edge.Name.c_str()) * 0.5f, IM_COL32_WHITE, edge.Name.c_str());
        }
    }

    // Draw the street name label associated with the start and end point
    if (const char* targetLabel = GetClosestEdgeLabel(s_TargetPinPosition))
        drawList->AddText(WorldToScreen(s_TargetPinPosition, image_position) - ImVec2(ImGui::CalcTextSize(targetLabel).x * 0.5f, 0.0f), IM_COL32_WHITE, targetLabel);

	if (const char* sourceLabel = GetClosestEdgeLabel(s_SourcePinPosition))
		drawList->AddText(WorldToScreen(s_SourcePinPosition, image_position) - ImVec2(ImGui::CalcTextSize(sourceLabel).x * 0.5f, 0.0f), IM_COL32_WHITE, sourceLabel);

    // Hanlde drawing of edge tooltips
    const int hoveredEdge = viewportHovered ? HitTestEdge(ImGui::GetMousePos(), image_position) : -1;
    if (hoveredEdge != s_PreviousHoveredEdge)
        s_HoveredEdgeTimer = 0.0f;

    if (s_HoveredEdgeTimer >= 0.35f && hoveredEdge >= 0 && hoveredEdge < s_SourceGraph.Edges.size())
    {
        const auto& edge = s_SourceGraph.Edges[hoveredEdge];
        if (!edge.Name.empty())
        {
            ImGui::BeginTooltip();
            ImGui::Text(FA_TAG " %s", edge.Name.c_str());
            ImGui::EndTooltip();
        }
    }

    s_PreviousHoveredEdge = hoveredEdge;

    // Handle viewport dragging
    if (ImGui::IsItemHovered() && s_GenerationType == GenerationType::None)
    {
        // Start dragging
        const bool select_clicked = ImGui::IsMouseClicked(SelectMouseButton);
        const bool drag_clicked = ImGui::IsMouseClicked(DragMouseButton);
        if (drag_clicked || select_clicked)
        {
            drag_start_pos = ImGui::GetMousePos();
            drag_start_offset = viewport_offset;

            if (drag_clicked)
            {
                s_DragContext.Type = DragType::Viewport;
                s_AddEdgeIndex = -1;
            }
            else if (select_clicked)
            {
                const DragContext hoveredItem = GetHoveredItem(image_position);

                if (s_AddEdgeIndex != -1)
                {
                    if (hoveredItem.Type == DragType::Vertex && s_AddEdgeIndex != hoveredItem.Index)
                        AddEdge(s_AddEdgeIndex, hoveredItem.Index);

                    s_AddEdgeIndex = -1;
                }
                else
                {
                    s_DragContext = hoveredItem;
                }

                ImGui::ResetMouseDragDelta();
            }
        }

        // Scroll Handling
        const float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f)
        {
            const ImVec2 mouse_pos = ImGui::GetMousePos();
            const ImVec2 mouse_viewport_pos = ImVec2(mouse_pos.x - image_position.x, mouse_pos.y - image_position.y);

            const ImVec2 world_pos_before = ImVec2((mouse_viewport_pos.x - viewport_offset.x) / viewport_zoom, (mouse_viewport_pos.y - viewport_offset.y) / viewport_zoom);

            if (scroll > 0)
                viewport_zoom *= ZoomFactor;
            else
                viewport_zoom /= ZoomFactor;

            viewport_zoom = ImClamp(viewport_zoom, 0.1f, 10.0f);

            ImVec2 world_pos_after = ImVec2(
                mouse_viewport_pos.x - world_pos_before.x * viewport_zoom,
                mouse_viewport_pos.y - world_pos_before.y * viewport_zoom
            );

            viewport_offset = world_pos_after;
        }
    }

    static bool drag_occurred = false;

    // Detect release
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        if (!drag_occurred && viewportHovered)
            ImGui::OpenPopup("##ContextMenu");

        drag_occurred = false;
    }

    if (s_GenerationType == GenerationType::None && ImGui::BeginPopup("##ContextMenu"))
    {
        if (ImGui::IsWindowAppearing())
        {
            s_PopupContext = GetHoveredItem(image_position);
            s_PopupWorldPosition = ScreenToWorld(ImGui::GetMousePos(), image_position);
        }

        char buffer[256];

        if (s_PopupContext.Type == DragType::Vertex || s_PopupContext.Type == DragType::Edge)
        {
            const char* context_type = s_PopupContext.Type == DragType::Vertex ? "Vertex" : "Edge";
			ImGui::TextDisabled(FA_CIRCLE_INFO " %s %d Context", context_type, s_PopupContext.Index);
        }
        else
        {
			ImGui::TextDisabled(FA_CIRCLE_INFO " Graph Context");
        }

        if (s_PopupContext.Type == DragType::Vertex)
        {
            ImGui::Separator();
            const auto& vertex = s_SourceGraph.Vertices[s_PopupContext.Index];
            ImGui::TextDisabled(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Position: (%.2f, %.2f)", vertex.Position.x, vertex.Position.y);
        }

		if (s_PopupContext.Type == DragType::Edge)
		{
			ImGui::Separator();
			const auto& edge = s_SourceGraph.Edges[s_PopupContext.Index];
			ImGui::TextDisabled(FA_CIRCLE " Endpoint A: %d", edge.IndexA);
			ImGui::TextDisabled(FA_CIRCLE " Endpoint B: %d", edge.IndexB);
		}

        ImGui::Separator();

        if (s_PopupContext.Type == DragType::None)
        {
            if (ImGui::MenuItem(FA_PLUS_LARGE " Add Vertex"))
                AddVertex(s_PopupWorldPosition);
        }

        if (s_PopupContext.Type == DragType::Vertex)
        {
			if (ImGui::MenuItem(FA_PLUS " Add Edge"))
				s_AddEdgeIndex = s_PopupContext.Index;

            if (ImGui::MenuItem(FA_TRASH " Delete Vertex"))
                DeleteVertex(s_PopupContext.Index);
        }

		if (s_PopupContext.Type == DragType::Edge)
		{
			if (ImGui::MenuItem(FA_TRASH " Delete Edge"))
                DeleteEdge(s_PopupContext.Index);
		}

        ImGui::EndPopup();
    }

    // Continue dragging
    if (s_DragContext.Type != DragType::None)
    {
        const bool select_down = ImGui::IsMouseDown(SelectMouseButton);
        const bool drag_down = ImGui::IsMouseDown(DragMouseButton);
        if (select_down || drag_down)
        {
            const ImVec2 current_pos = ImGui::GetMousePos();
            const ImVec2 delta = ImVec2(current_pos.x - drag_start_pos.x, current_pos.y - drag_start_pos.y);

            if (Distance(delta, { 0.0f, 0.0f }))
                drag_occurred = true;

            if (drag_down)
                viewport_offset = ImVec2(drag_start_offset.x + delta.x, drag_start_offset.y + delta.y);
            else if (select_down && s_DragContext.Type == DragType::Source)
            {
                s_SourcePinPosition = ScreenToWorld(current_pos, image_position);
            }
            else if (select_down && s_DragContext.Type == DragType::Target)
            {
                s_TargetPinPosition = ScreenToWorld(current_pos, image_position);
            }
			else if (select_down && s_DragContext.Type == DragType::Vertex && s_DragContext.Index >= 0 && s_DragContext.Index < s_SourceGraph.Vertices.size())
			{
				const ImVec2 world = ScreenToWorld(current_pos, image_position);
				s_SourceGraph.Vertices[s_DragContext.Index].Position = world;
				RegenerateGraph();
			}
        }
        else
		{
			s_DragContext = {};
        }
    }

    constexpr ImVec2 ButtonSize = { 50.0f, 50.0f };
    auto& style = ImGui::GetStyle();
    const ImVec2 windowPosition = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorScreenPos(ImVec2(windowPosition.x + windowSize.x - style.WindowPadding.x - 3.0f * (ButtonSize.x + style.FramePadding.x * 2.0f), windowPosition.y + style.WindowPadding.y));

    if (DrawBigTextButton("##Random", FA_ABACUS, ButtonSize))
        s_GenerationType = GenerationType::Random;
    ImGui::SameLine();
    if (DrawBigTextButton("##Text", FA_TEXT_SIZE, ButtonSize))
        s_GenerationType = GenerationType::Text;
    ImGui::SameLine();
    if (DrawBigTextButton("##City", FA_CITY, ButtonSize))
        s_GenerationType = GenerationType::City;

    ImGui::End();

    // Generation Popup
    if (s_GenerationType != GenerationType::None)
    {
        ImGui::OpenPopup(FA_GEAR " Graph Generation Options");

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImVec2 center = viewport->GetCenter();
		const ImVec2 size(viewport->Size.x * 0.5f, viewport->Size.y * 0.75f);

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(size);

		if (ImGui::BeginPopupModal(FA_GEAR " Graph Generation Options", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize))
		{
            if (ImGui::IsWindowAppearing())
            {
                s_GenerationData = {};
            }

            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
            ImGui::Text("%s Graph Generation", s_GenerationType == GenerationType::City ? "City Network" : s_GenerationType == GenerationType::Text ? "Text" : "Random");
            ImGui::PopFont();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (s_GenerationType == GenerationType::City)
			{
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
				ImGui::Text(FA_MAP_LOCATION " Addresses");
				ImGui::PopFont();

				ImGui::Spacing();

				// Child window for scrollable address entries
				const float childHeight = size.y * 0.35f;
				ImGui::BeginChild("AddressListChild", ImVec2(0, childHeight), true);

				for (size_t i = 0; i < s_GenerationData.Addresses.size(); i++)
				{
					auto& addr = s_GenerationData.Addresses[i];

					ImGui::PushID((int)i);

					ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
					ImGui::Text("Address %zu", i + 1);
					ImGui::PopFont();
					ImGui::Spacing();

                    const ImVec2 startPos = ImGui::GetCursorScreenPos();

					ImGui::BeginGroup();
                    {

                        // Column layout: labels left, inputs right
                        ImGui::Columns(2, nullptr, false);
                        ImGui::SetColumnWidth(0, 90.0f); // fixed label width

                        // Suburb
                        ImGui::Text(FA_HOUSE " Suburb");
                        ImGui::NextColumn();
                        ImGui::InputText("##Suburb", &addr.Suburb);
                        ImGui::NextColumn();

                        // City
                        ImGui::Text(FA_CITY " City");
                        ImGui::NextColumn();
                        ImGui::InputText("##City", &addr.City);
                        ImGui::NextColumn();

                        // State
                        ImGui::Text(FA_MAP " State");
                        ImGui::NextColumn();
                        ImGui::InputText("##State", &addr.State);
                        ImGui::NextColumn();

                        // Country
                        ImGui::Text(FA_EARTH_AMERICAS " Country");
                        ImGui::NextColumn();
                        ImGui::InputText("##Country", &addr.Country);

                        ImGui::Columns(1);
                    }
					ImGui::EndGroup();

                    const ImVec2 endPos = ImGui::GetItemRectMax();
					const float midY = (startPos.y + endPos.y) * 0.5f;
					const float iconSize = ImGui::GetFontSize() * 1.3f;

					ImVec2 iconPos(
						ImGui::GetWindowPos().x +
						ImGui::GetWindowContentRegionMax().x - iconSize * 1.5f,
						midY - iconSize * 0.5f
					);

					ImGui::SetCursorScreenPos(iconPos);

					ImGui::BeginDisabled(s_GenerationData.Addresses.size() <= 1);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

					if (ImGui::Button(FA_TRASH "##remove", ImVec2(iconSize, iconSize)))
					{
						s_GenerationData.Addresses.erase(s_GenerationData.Addresses.begin() + i);
						ImGui::PopStyleColor(3);
						ImGui::EndDisabled();
						ImGui::PopID();
						break;
					}

					ImGui::PopStyleColor(3);
					ImGui::EndDisabled();

                    ImGui::Dummy(ImVec2(0.0f, 35.0f));

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::PopID();
				}

				ImGui::EndChild();

				ImGui::Spacing();

				// Add new address button (centered)
				{
					float addWidth = 180.0f;
					float x = (size.x - addWidth) * 0.5f;
					ImGui::SetCursorPosX(x);
					if (ImGui::Button(FA_PLUS " Add Address", ImVec2(addWidth, 0)))
					{
						s_GenerationData.Addresses.push_back({ "", "Sydney", "NSW", "Australia" });
					}
				}

				ImGui::Spacing();
				ImGui::Spacing();

				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
				ImGui::Text(FA_CITY " Network Options");
				ImGui::PopFont();
				ImGui::Spacing();

				// City type combo box

				const char* cityTypes[] = { FA_CAR " Drive", FA_PERSON_WALKING " Walk", FA_BICYCLE " Bike", FA_BACKPACK " All" };
                int current = (int)s_GenerationData.TravelType;

                ImGui::Indent();

				ImGui::TextUnformatted(FA_SUITCASE " Travel Options");
				ImGui::SameLine();

				if (ImGui::BeginCombo("##CityTypeCombo", cityTypes[current]))
				{
					for (int i = 0; i < 4; i++)
					{
						bool selected = (i == current);
						if (ImGui::Selectable(cityTypes[i], selected))
							s_GenerationData.TravelType = (GenerationData::CityTravelType)i;
						if (selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::TextUnformatted(FA_SHAPES " Simplified Graph");
				ImGui::SameLine();
				ImGui::Checkbox("##SimpleGraph", &s_GenerationData.SimplifiedGraph);

                ImGui::Unindent();

				ImGui::Spacing();
				ImGui::Spacing();
			}
            else if (s_GenerationType == GenerationType::Random)
            {

            }
			else if (s_GenerationType == GenerationType::Text)
			{

			}

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
			ImGui::Text(FA_SLIDERS " Graph Options");
			ImGui::PopFont();
			ImGui::Spacing();

			{
                ImGui::Indent();
                ImGui::TextUnformatted(FA_EXPAND " Scale");
                ImGui::SameLine();
				ImGui::DragFloat("##ScaleInput", &s_GenerationData.Scale, 0.2f, 0.1f, 10.0f, "%.2f");

				ImGui::TextUnformatted(FA_LINK " Append to Existing");
				ImGui::SameLine();
				ImGui::Checkbox("##AppendGraph", &s_GenerationData.Append);

                ImGui::Unindent();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			const float buttonWidth = 120.0f;
			const float spacing = 40.0f;
			const float totalWidth = buttonWidth * 2 + spacing;

            ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 50.0f));

			ImGui::SetCursorPosX((size.x - totalWidth) * 0.5f);


            if (ImGui::Button(FA_CIRCLE_XMARK " Cancel", ImVec2(buttonWidth, 0)))
			{
				s_GenerationType = GenerationType::None;
				ImGui::CloseCurrentPopup();
            }

			ImGui::SameLine();

			if (ImGui::Button(FA_CHART_NETWORK " Generate", ImVec2(buttonWidth, 0)))
			{
                if (!s_GenerationData.Append)
                    NewGraph();

                LoadCityGraph(s_GenerationData.Addresses, s_GenerationData.TravelType, s_GenerationData.SimplifiedGraph, s_GenerationData.Scale);
                RegenerateGraph();

				s_GenerationType = GenerationType::None;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
    }

    // Graph Outliner
    ImGui::Begin("Outline", nullptr, window_flags);

    for (size_t index = 0; index < AlgorithmTypeCount; index++)
    {
        ImGui::PushID(index);

        if (DrawTextButton(s_AlgorithmVisible[index] ? FA_EYE : FA_EYE_SLASH))
        {
            s_AlgorithmVisible[index] = !s_AlgorithmVisible[index];
            RecomputeTraversalGPUData();
        }

        ImGui::SameLine();

        if (ImGui::ColorEdit4("##color", (float*)&s_AlgorithmCompletedColors[index], ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
            RecomputeTraversalGPUData();

        ImGui::SameLine();
        s_AlgorithmEnabled[index] && s_AlgorithmVisible[index] ? ImGui::TextUnformatted(AlgorithmTypeToString((AlgorithmType)index)) : ImGui::TextDisabled(AlgorithmTypeToString((AlgorithmType)index));
		ImGui::SameLine();

        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 225.0f, 0.0f));
        ImGui::SameLine();

        const bool disabled = !s_AlgorithmTrackMemory[index];
        if (disabled)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
		if (DrawTextButton(FA_MEMORY))
            s_AlgorithmTrackMemory[index] = !s_AlgorithmTrackMemory[index];
        if (disabled)
            ImGui::PopStyleColor();
        ImGui::SetItemTooltip(FA_MEMORY " Memory tracking for %s is %s.", AlgorithmTypeToString((AlgorithmType)index), s_AlgorithmTrackMemory[index] ? "enabled" : "disabled");

		ImGui::SameLine();

		ImGui::TextDisabled(FA_PEN);
		ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_FrameBg, {});
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {});
        ImGui::SetNextItemWidth(35.0f);
        if (ImGui::DragFloat("##Thickness", &s_AlgorithmThickness[index], 0.2f, 0.1f, 10.0f, "%.2f"))
            RecomputeTraversalGPUData();
        ImGui::PopStyleColor(2);
        
        ImGui::SameLine();

		ImGui::TextDisabled(FA_CIRCLE_HALF_STROKE);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, {});
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, {});
		ImGui::SetNextItemWidth(35.0f);
        if (ImGui::DragFloat("##Opacity", &s_AlgorithmCompletedColors[index].w, 0.05f, 0.0f, 1.0f, "%.2f"))
            RecomputeTraversalGPUData();
		ImGui::PopStyleColor(2);

		ImGui::SameLine();

        if (DrawTextButton(s_AlgorithmEnabled[index] ? FA_XMARK : FA_CHECK))
            s_AlgorithmEnabled[index] = !s_AlgorithmEnabled[index];

        ImGui::PopID();
    }

    ImGui::End();

    // Control Panel
    ImGui::Begin("Controls", nullptr, window_flags);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
    ImGui::TextUnformatted(FA_SLIDERS " Controls");
    ImGui::PopFont();

    ImGui::Text(FA_CIRCLE " Vertex count: %zu", s_SourceGraph.Vertices.size());
    ImGui::Text(FA_DASH " Edge count: %zu", s_SourceGraph.Edges.size());

    if (ImGui::Button(FA_REDO " Reset View"))
    {
        viewport_offset = ImVec2(0.0f, 0.0f);
        viewport_zoom = 1.0f;
    }

    ImGui::Text(FA_ARROWS_UP_DOWN_LEFT_RIGHT " Offset: %.1f, %.1f", viewport_offset.x, viewport_offset.y);

    ImGui::Spacing();
    ImGui::Spacing();
	ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Statistics Menu
	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
	ImGui::TextUnformatted(FA_CHART_LINE " Statistics");
	ImGui::PopFont();

    bool any_valid = false;

	if (ImGui::BeginTable("##StatisticsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable))
    {
		// Column headers
		ImGui::TableSetupColumn(FA_DIAGRAM_PROJECT " Algorithm");
		ImGui::TableSetupColumn(FA_TIMER " Execution Time");
		ImGui::TableSetupColumn(FA_RULER " Distance");
		ImGui::TableSetupColumn(FA_MEMORY " Peak Memory Usage");
		ImGui::TableSetupColumn(FA_CODE_BRANCH " Graph Traversed");
		ImGui::TableHeadersRow();

		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
		{
			if (sort_specs->SpecsDirty)
			{
				g_SortSpecs = sort_specs;
				qsort(s_SortedIndices.data(), s_SortedIndices.size(), sizeof(size_t), CompareAlgorithms);
				sort_specs->SpecsDirty = false;
			}
		}

		// Rows
		for (size_t index : s_SortedIndices)
        {
            const auto& metadata = s_DrawGraph.Metadata[index];
            if (!metadata.Valid)
                continue;

            any_valid = true;

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", AlgorithmTypeToString((AlgorithmType)index));

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.0f ms", metadata.Duration / 1'000'000.0f);

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.0f units", metadata.TotalDistance);

			ImGui::TableSetColumnIndex(3);
            metadata.MemoryTrackingData.empty() ? ImGui::TextDisabled(" " FA_DASH " ") : ImGui::Text("%zu KiB", metadata.PeakMemoryUsage / 1024);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.0f%%", metadata.GraphTraversalPercentage * 100.0f);
		}

		ImGui::EndTable();
	}

    if (!any_valid)
    {
        const char* message = "Generate a route to view statistics here.";
        ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(message).x) * 0.5f, 0.0f));
        ImGui::SameLine();
        ImGui::TextDisabled(message);
    }

    ImGui::End();

    // Timeline
    ImGui::Begin("Timeline", nullptr, window_flags);
    ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvail().x - (450.0f + ImGui::GetStyle().FramePadding.x * 4.0f)) * 0.5f, 0.0f));
    ImGui::SameLine();
    if (ImGui::Button(FA_BACKWARD_STEP, ImVec2(50, 0)))
    {
        s_Time = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button(s_Paused ? FA_PLAY : FA_PAUSE, ImVec2(50, 0)))
        s_Paused = !s_Paused;
    ImGui::SameLine();
    if (s_Loop)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
    const bool clicked = ImGui::Button(FA_REPEAT, ImVec2(50, 0));
    if (s_Loop)
        ImGui::PopStyleColor();
    if (clicked)
        s_Loop = !s_Loop;
    ImGui::SameLine();
    if (ImGui::Button(FA_FORWARD_STEP, ImVec2(50, 0)))
    {
		s_Time = GetPlaybackDuration();
		s_Loop = false;
    }

    ImGui::SameLine();
    ImGui::Dummy(ImVec2(50.0f, 0.0f));
    ImGui::SameLine();
    ImGui::TextDisabled(FA_TURTLE);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200.0f);
    SliderFloat("##PlaybackSpeedSlider", &s_PlaybackSpeed, 0.01f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SameLine();
    ImGui::TextDisabled(FA_RABBIT);

    ImGuiSequencer(s_Time, 0.0f, GetPlaybackDuration(), ImGui::GetContentRegionAvail().y);
    ImGui::End();
}

static AdjacencyMatrix BuildAdjacencyMatrix(const SourceGraph& graph)
{
	const size_t N = graph.Vertices.size();

	// Initialize N�N matrix with {0.0f, -1} meaning "no edge"
	AdjacencyMatrix matrix(N, std::vector<std::pair<float, int>>(N, { 0.0f, -1 }));

	// Populate matrix with edges
	for (uint32_t index = 0; index < graph.Edges.size(); index++)
	{
		const auto& e = graph.Edges[index];

        const auto& vertexA = graph.Vertices[e.IndexA];
        const auto& vertexB = graph.Vertices[e.IndexB];

        const float weight = Distance(vertexA.Position, vertexB.Position);
		matrix[e.IndexA][e.IndexB] = { weight, index };
		matrix[e.IndexB][e.IndexA] = { weight, index };
	}

	return matrix;
}

static DrawGraph CreateDrawGraph(const SourceGraph& graph)
{
    DrawGraph drawGraph;
	drawGraph.Vertices = graph.Vertices;

	for (const auto& edge : graph.Edges)
	{
		const auto& A = graph.Vertices[edge.IndexA].Position;
		const auto& B = graph.Vertices[edge.IndexB].Position;

		const ImVec2 to = B - A;
		const float length = sqrtf(to.x * to.x + to.y * to.y);
		if (length < 1e-4f)
			continue;

		const ImVec2 direction = to / length;
		const ImVec2 normal = { -direction.y, direction.x };

		// push two triangles
		drawGraph.EdgeVertices.emplace_back(A,  normal);
		drawGraph.EdgeVertices.emplace_back(A, -normal);
		drawGraph.EdgeVertices.emplace_back(B, normal);

		drawGraph.EdgeVertices.emplace_back(B,  normal);
		drawGraph.EdgeVertices.emplace_back(A, -normal);
		drawGraph.EdgeVertices.emplace_back(B, -normal);
	}

    // Default duration (just so it's not zero)
    drawGraph.Duration = 10.0f;

    return drawGraph;
}

static void AddTimedDrawGraphEntry(const AlgorithmType algorithmType, Algorithm* algorithm, const SourceGraph& graph, DrawGraph& drawGraph, const AdjacencyMatrix& adjacencyMatrix, uint32_t source, uint32_t destination)
{
    if (!s_AlgorithmEnabled[(size_t)algorithmType])
        return;

    const bool tracking = s_TrackMemory && s_AlgorithmTrackMemory[(size_t)algorithmType];
    if (tracking)
        START_MEMORY_TRACKING(s_MemoryTrackingInterval);

	const auto start = std::chrono::high_resolution_clock::now();
	algorithm->FindPath(adjacencyMatrix, source, destination);
	const auto end = std::chrono::high_resolution_clock::now();

    std::vector<size_t> memory = tracking ? END_MEMORY_TRACKING() : std::vector<size_t>{};

	TraversalResult result = algorithm->GetResult();

	const double elapsed = std::chrono::duration<double, std::nano>(end - start).count();
	const double totalSteps = result.TraversedEdges.size();

	for (size_t step = 0; step < result.TraversedEdges.size(); step++)
	{
		const uint32_t edgeIndex = result.TraversedEdges[step];
		const double traversalTime = ((double)step / totalSteps) * elapsed;

		for (uint32_t vertex = 0; vertex < 6; vertex++)
			drawGraph.EdgeVertices[edgeIndex * 6 + vertex].TraversalTimes[(size_t)algorithmType] = traversalTime;
	}

	for (const auto edgeIndex : result.FinalEdges)
	{
		for (uint32_t vertex = 0; vertex < 6; vertex++)
			drawGraph.EdgeVertices[edgeIndex * 6 + vertex].CompletionTimes[(size_t)algorithmType] = elapsed;
	}

    if (elapsed > drawGraph.Duration)
        drawGraph.Duration = elapsed;

    auto& metadata = drawGraph.Metadata[(size_t)algorithmType];
    metadata.Valid = true;
    metadata.Duration = elapsed;

    std::unordered_set<uint32_t> uniqueEdges(result.TraversedEdges.begin(), result.TraversedEdges.end());
    metadata.GraphTraversalPercentage = static_cast<double>(uniqueEdges.size()) / static_cast<double>(s_SourceGraph.Edges.size());

    metadata.PeakMemoryUsage = memory.empty() ? 0 : memory.back();

    metadata.TotalDistance = 0.0f;
	for (const auto edgeIndex : result.FinalEdges)
    {
        if (edgeIndex < 0 || edgeIndex >= s_SourceGraph.Edges.size())
            continue;

        const auto& edge = s_SourceGraph.Edges[edgeIndex];
        const auto& v0 = s_SourceGraph.Vertices[edge.IndexA].Position;
		const auto& v1 = s_SourceGraph.Vertices[edge.IndexB].Position;
        double edgeLength = Distance(v0, v1);
		metadata.TotalDistance += edgeLength;
	}

    metadata.MemoryTrackingData = std::move(memory);
}

#define TIME_ALGORITHM(name)                        \
    name name##_algorithm;                                 \
    AddTimedDrawGraphEntry(AlgorithmType::name, &name##_algorithm, graph, drawGraph, adjacencyMatrix, source, destination);

static DrawGraph CreateTimedDrawGraph(uint32_t source, uint32_t destination, const SourceGraph& graph)
{
    DrawGraph drawGraph = CreateDrawGraph(graph);
    drawGraph.Duration = 0.0f;

    const AdjacencyMatrix adjacencyMatrix = BuildAdjacencyMatrix(graph);

    TIME_ALGORITHM(BFS);
    TIME_ALGORITHM(DFS);
    TIME_ALGORITHM(DijkstraArray);
    TIME_ALGORITHM(DijkstraQueue);
    TIME_ALGORITHM(DEsopoPape);
    TIME_ALGORITHM(BellmanFord);
    TIME_ALGORITHM(FloydWarshall);
    
    return drawGraph;
}

#undef TIME_ALGORITHM
