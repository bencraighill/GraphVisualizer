// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <glad/glad.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
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

#include "TextSymbols.h"

#include <vector>
#include <queue>
#include <chrono>
#include <cmath>

#include <yaml-cpp/yaml.h>
#include <iostream>

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

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 16.0f);

    ImFontConfig config;
    config.MergeMode = true;
    config.GlyphMinAdvanceX = 25.0f;
    //static const ImWchar icon_ranges[] = { min_range, max_range, 0 };
    ImWchar* icon_ranges = new ImWchar[]{ (ImWchar)0xE000, (ImWchar)0xF8FF, 0 };
    ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 25.0f, &config, icon_ranges);

    ImWchar* other_ranges = new ImWchar[]{ (ImWchar)0xf041, (ImWchar)0xf041, (ImWchar)0xf3c5,(ImWchar)0xf3c5, 0 };
    io.Fonts->AddFontFromFileTTF("Resources/Fonts/fontawesome/Font Awesome 6 Pro-Solid-900.otf", 60, nullptr, other_ranges);

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
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
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
static float s_EdgeThickness = 1.0f;

static float s_PreviousIOTime;
static float s_Time = 0.0f;
static bool s_Paused = true;
static bool s_Loop = true;

enum class DragType
{
    None,
    Viewport,
    Source,
    Target,
};

static ImVec2 viewport_offset = { 0.0f, 0.0f };
static float viewport_zoom = 1.0f;
static ImVec2 drag_start_pos;
static ImVec2 drag_start_offset;
static DragType is_dragging = DragType::None;

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
    float TraversalTime = -1.0f;
    float CompletionTime = -1.0f;
};

struct Edge
{
    uint32_t IndexA;
    uint32_t IndexB;
    // Note: weight is simply the distance between the two points
};

struct SourceGraph
{
    std::vector<VertexInstance> Vertices;
    std::vector<Edge> Edges;
};

struct DrawGraph
{
    std::vector<VertexInstance> Vertices;
    std::vector<EdgeVertex> EdgeVertices;
    float Duration;
};

static SourceGraph s_SourceGraph;
static DrawGraph s_DrawGraph;

static float Distance(const ImVec2& a, const ImVec2& b)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return sqrtf(dx * dx + dy * dy);
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
        layout(location = 2) in float a_TraversalTime;
        layout(location = 3) in float a_CompletionTime;

        layout(location = 0) out vec3 v_Color;
        
        uniform vec2 u_ViewportSize;
        uniform vec2 u_ViewportOffset;
        uniform float u_ViewportZoom;
        uniform float u_EdgeThickness;
        uniform float u_Time;

        void main()
        {
            bool in_complete = (a_CompletionTime >= 0.0) && (u_Time >= a_CompletionTime);
            bool in_traversed = (a_TraversalTime >= 0.0) && (u_Time >= a_TraversalTime);

            float thickness = max((in_complete ? 2.0 : in_traversed ? 1.0 : 0.5) * u_EdgeThickness, 0.5);
            v_Color = in_complete ? vec3(0.922, 0.812, 0.192) : in_traversed ? vec3(0.557, 0.667, 0.878) : vec3(0.5);

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

    glEnableVertexAttribArray(2); // a_TraversalTime
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)offsetof(EdgeVertex, TraversalTime));

    glEnableVertexAttribArray(3); // a_CompletionTime
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(EdgeVertex), (void*)offsetof(EdgeVertex, CompletionTime));

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

static DrawGraph TimeAlgorithm(uint32_t source, uint32_t destination, const SourceGraph& graph);

static void RegenerateGraph()
{
    if (s_SourceGraph.Vertices.empty())
        return;

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

    s_DrawGraph = TimeAlgorithm(source, target, s_SourceGraph);

    // Update what the GPU data sees
    glBindVertexArray(line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, line_vbo);
    glBufferData(GL_ARRAY_BUFFER, s_DrawGraph.EdgeVertices.size() * sizeof(EdgeVertex), s_DrawGraph.EdgeVertices.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, s_DrawGraph.Vertices.size() * sizeof(VertexInstance), s_DrawGraph.Vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void Init()
{
	std::cout << "Current working directory: "
		<< std::filesystem::current_path() << "\n";

    s_PreviousIOTime = ImGui::GetTime();

    circle_shader = CreateCircleShader();
    CreateCircleGeometry();

    line_shader = CreateLineShader();
    CreateLineGeometry();

    s_SourceGraph.Vertices.clear();
    s_SourceGraph.Edges.clear();

    YAML::Node config = YAML::LoadFile("network.yaml");
    for (const auto& node : config["nodes"])
    {
        const float x = node["x"].as<float>();
        const float y = node["y"].as<float>();
        s_SourceGraph.Vertices.push_back({ ImVec2(x, -y) });
    }

    for (const auto& edge : config["edges"])
    {
        const uint32_t source = edge["source"].as<uint32_t>();
        const uint32_t target = edge["target"].as<uint32_t>();
        s_SourceGraph.Edges.push_back({ source, target });
    }

    std::cout << "Loaded " << s_SourceGraph.Vertices.size() << " nodes and " << s_SourceGraph.Edges.size() << " edges\n";

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

    if (!s_Paused)
        s_Time += (deltaTime / PlaybackCycleTime) * duration;

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
    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

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
    float timeline_top = canvas_pos.y + 25.0f;
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
        snprintf(label, sizeof(label), "%.2f", t);
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

    // Draw playhead
    float playhead_x = canvas_pos.x + ((currentTime - minTime) / range) * canvas_width;
    playhead_x = ImClamp(playhead_x, canvas_pos.x, canvas_pos.x + canvas_width);

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
    snprintf(time_label, sizeof(time_label), "Time: %.2f", currentTime);
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
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { exit(0); }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem(FA_ROUTE " Generate"))
            RegenerateGraph();

        if (ImGui::BeginMenu(FA_MAGNIFYING_GLASS " View"))
        {
            ImGui::DragFloat(FA_CIRCLE " Vertex Radius", &s_VertexRadius, 0.05f, 0.0f, 10.0f);
            ImGui::DragFloat(FA_DASH " Edge Thickness", &s_EdgeThickness, 0.05f, 0.25f, 5.0f);
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
        ImGuiID dock_viewport, dock_controls, dock_timeline;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.8f, &dock_viewport, &dock_controls);
        ImGui::DockBuilderSplitNode(dock_viewport, ImGuiDir_Up, 0.9f, &dock_viewport, &dock_timeline);

        // Dock windows
        ImGui::DockBuilderDockWindow("Viewport", dock_viewport);
        ImGui::DockBuilderDockWindow("Controls", dock_controls);
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

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    const ImVec2 pin_size = ImGui::CalcTextSize(FA_LOCATION_PIN);
    ImGui::GetWindowDrawList()->AddText(WorldToScreen(s_SourcePinPosition, image_position) - ImVec2(pin_size.x * 0.5f, pin_size.y), IM_COL32(200, 130, 150, 255), FA_LOCATION_PIN);

    const ImVec2 dot_size = ImGui::CalcTextSize(FA_LOCATION_DOT);
    ImGui::GetWindowDrawList()->AddText(WorldToScreen(s_TargetPinPosition, image_position) - ImVec2(dot_size.x * 0.5f, dot_size.y), IM_COL32(150, 130, 200, 255), FA_LOCATION_DOT);
    ImGui::PopFont();

    // Handle viewport dragging
    if (ImGui::IsItemHovered())
    {
        // Start dragging
        const bool select_clicked = ImGui::IsMouseClicked(SelectMouseButton);
        const bool drag_clicked = ImGui::IsMouseClicked(DragMouseButton);
        if (drag_clicked || select_clicked)
        {
            drag_start_pos = ImGui::GetMousePos();
            drag_start_offset = viewport_offset;

            if (drag_clicked)
                is_dragging = DragType::Viewport;
            else if (select_clicked)
            {
                const ImVec2 source_pin_screen = WorldToScreen(s_SourcePinPosition, image_position);
                const ImVec2 target_pin_screen = WorldToScreen(s_TargetPinPosition, image_position);
                is_dragging = Distance(ImGui::GetMousePos(), source_pin_screen) < PinDragThreshold ? DragType::Source : Distance(ImGui::GetMousePos(), target_pin_screen) < PinDragThreshold ? DragType::Target : DragType::None;
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

    // Continue dragging
    if (is_dragging != DragType::None)
    {
        const bool select_down = ImGui::IsMouseDown(SelectMouseButton);
        const bool drag_down = ImGui::IsMouseDown(DragMouseButton);
        if (select_down || drag_down)
        {
            const ImVec2 current_pos = ImGui::GetMousePos();
            const ImVec2 delta = ImVec2(current_pos.x - drag_start_pos.x, current_pos.y - drag_start_pos.y);

            if (drag_down)
                viewport_offset = ImVec2(drag_start_offset.x + delta.x, drag_start_offset.y + delta.y);
            else if (select_down && is_dragging == DragType::Source)
            {
                s_SourcePinPosition = ScreenToWorld(current_pos, image_position);
            }
            else if (select_down && is_dragging == DragType::Target)
            {
                s_TargetPinPosition = ScreenToWorld(current_pos, image_position);
            }
        }
        else
        {
            is_dragging = DragType::None;
        }
    }

    ImGui::End();

    ImGui::Begin("Controls", nullptr, window_flags);

    ImGui::Text("Vertex count: %zu", s_SourceGraph.Vertices.size());
    ImGui::Text("Vertex count: %zu", s_SourceGraph.Edges.size());

    ImGui::Separator();

    if (ImGui::Button("Reset View"))
    {
        viewport_offset = ImVec2(0.0f, 0.0f);
        viewport_zoom = 1.0f;
    }

    ImGui::Text("Offset: %.1f, %.1f", viewport_offset.x, viewport_offset.y);
    ImGui::End();

    ImGui::Begin("Timeline", nullptr, window_flags);
    ImGui::Dummy(ImVec2((ImGui::GetContentRegionAvail().x - (100.0f + ImGui::GetStyle().FramePadding.x * 2.0f)) * 0.5f, 0.0f));
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
    ImGuiSequencer(s_Time, 0.0f, GetPlaybackDuration(), ImGui::GetContentRegionAvail().y);
    ImGui::End();
}

struct TraversalResult
{
    std::vector<uint32_t> TraversedEdges;
    std::vector<uint32_t> FinalEdges;
};

using AdjacencyList = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>;

static AdjacencyList BuildAdjacencyList(const SourceGraph& graph)
{
    AdjacencyList adj(graph.Vertices.size());

    for (uint32_t index = 0; index < graph.Edges.size(); index++)
    {
        adj[graph.Edges[index].IndexA].push_back({ graph.Edges[index].IndexB, index });
        adj[graph.Edges[index].IndexB].push_back({ graph.Edges[index].IndexA, index });
    }

    return adj;
}

TraversalResult BFS(uint32_t source, uint32_t destination, const AdjacencyList& adj);

static DrawGraph TimeAlgorithm(uint32_t source, uint32_t destination, const SourceGraph& graph)
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
        drawGraph.EdgeVertices.push_back({ A,  normal, -1.0f, -1.0f });
        drawGraph.EdgeVertices.push_back({ A, -normal, -1.0f, -1.0f });
        drawGraph.EdgeVertices.push_back({ B,  normal, -1.0f, -1.0f });

        drawGraph.EdgeVertices.push_back({ B,  normal, -1.0f, -1.0f });
        drawGraph.EdgeVertices.push_back({ A, -normal, -1.0f, -1.0f });
        drawGraph.EdgeVertices.push_back({ B, -normal, -1.0f, -1.0f });
    }

    const AdjacencyList adjacencyList = BuildAdjacencyList(graph);

    const auto start = std::chrono::high_resolution_clock::now();
    TraversalResult result = BFS(source, destination, adjacencyList);
    const auto end = std::chrono::high_resolution_clock::now();

    const double elapsed = std::chrono::duration<double, std::nano>(end - start).count();
    const double totalSteps = result.TraversedEdges.size();

    for (size_t step = 0; step < result.TraversedEdges.size(); step++)
    {
        const uint32_t edgeIndex = result.TraversedEdges[step];
        const double traversalTime = ((double)step / totalSteps) * elapsed;

        for (uint32_t vertex = 0; vertex < 6; vertex++)
            drawGraph.EdgeVertices[edgeIndex * 6 + vertex].TraversalTime = traversalTime;
    }

    for (const auto edgeIndex : result.FinalEdges)
    {
        for (uint32_t vertex = 0; vertex < 6; vertex++)
            drawGraph.EdgeVertices[edgeIndex * 6 + vertex].CompletionTime = elapsed;
    }

    drawGraph.Duration = elapsed;
    return drawGraph;
}

TraversalResult BFS(uint32_t source, uint32_t destination, const AdjacencyList& adj)
{
    int n = adj.size();
    std::vector<int> parent(n, -1);
    std::vector<bool> visited(n, false);
    std::vector<uint32_t> traversedEdges;

    std::queue<int> q;
    q.push(source);
    visited[source] = true;
    int totalVisited = 1;

    while (!q.empty())
    {
        int u = q.front(); q.pop();

        for (auto& [v, edgeIndex] : adj[u])
        {
            if (!visited[v])
            {
                visited[v] = true;
                parent[v] = u;
                traversedEdges.push_back(edgeIndex);
                q.push(v);
                totalVisited++;
            }
        }
    }

    // Reconstruct shortest path
    std::vector<uint32_t> finalPathEdges;
    if (visited[destination])
    {
        int v = destination;
        while (parent[v] != -1)
        {
            int u = parent[v];

            // Find the edge between u and v
            for (auto& [w, edgeIndex] : adj[u])
            {
                if (w == v)
                {
                    finalPathEdges.push_back(edgeIndex);
                    break;
                }
            }

            v = u;
        }
        std::reverse(finalPathEdges.begin(), finalPathEdges.end());
    }

    return { traversedEdges, finalPathEdges };
}

