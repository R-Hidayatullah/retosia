#include "scene_builder.hpp"
#include "scene.hpp"
#include "ipf_manager.hpp"
#include "ies.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <mutex>
#include <cfloat>
#include "thread_pool.hpp"

// ---------------------- Globals ----------------------
std::vector<scene::Scene> g_loaded_scenes;
std::mutex g_scene_mutex;
bool g_ready_to_render = false;

// Camera
glm::vec3 cam_target(0.0f);
float cam_distance = 10.0f;
float cam_yaw = 0.0f;
float cam_pitch = 0.3f;

// Mouse
bool left_drag = false;
bool middle_drag = false;
double last_x = 0, last_y = 0;

// GPU Mesh
struct MeshGPU
{
    GLuint vao, vbo, ebo;
    GLsizei index_count;
};

// Cube placeholder
struct CubeGPU
{
    GLuint vao, vbo, ebo;
    GLsizei index_count;
} cube;

// Scene placeholder
glm::vec3 scene_center(0.0f);
float scene_radius = 5.0f;

// ---------------------- OpenGL Helpers ----------------------
MeshGPU upload_mesh_to_gpu(const scene::SubMesh &sub)
{
    MeshGPU gpu{};
    glGenVertexArrays(1, &gpu.vao);
    glGenBuffers(1, &gpu.vbo);
    glGenBuffers(1, &gpu.ebo);

    glBindVertexArray(gpu.vao);

    std::vector<float> vertex_data;
    for (auto &v : sub.positions)
        vertex_data.push_back(v.x), vertex_data.push_back(v.y), vertex_data.push_back(v.z);

    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sub.indices.size() * sizeof(uint32_t), sub.indices.data(), GL_STATIC_DRAW);
    gpu.index_count = static_cast<GLsizei>(sub.indices.size());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    glBindVertexArray(0);
    return gpu;
}

GLuint compile_shader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        throw std::runtime_error(buf);
    }
    return shader;
}

GLuint create_shader_program()
{
    const char *vert_src = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform mat4 uMVP;
        void main() { gl_Position = uMVP * vec4(aPos,1.0); })";

    const char *frag_src = R"(
        #version 330 core
        out vec4 FragColor;
        void main() { FragColor = vec4(0.8,0.8,0.8,1.0); })";

    GLuint vert = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    glDeleteShader(vert);
    glDeleteShader(frag);

    int success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char buf[512];
        glGetProgramInfoLog(prog, 512, nullptr, buf);
        throw std::runtime_error(buf);
    }
    return prog;
}

// ---------------------- Callbacks ----------------------
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        left_drag = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
        middle_drag = (action == GLFW_PRESS);

    glfwGetCursorPos(window, &last_x, &last_y);
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    double dx = xpos - last_x;
    double dy = ypos - last_y;

    if (left_drag)
    {
        cam_yaw += static_cast<float>(dx) * 0.005f;
        cam_pitch += static_cast<float>(dy) * 0.005f;
        cam_pitch = glm::clamp(cam_pitch, -1.5f, 1.5f);
    }

    if (middle_drag)
    {
        // true panning (vertical + horizontal)
        glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0),
                                                    glm::normalize(cam_target - glm::vec3(0, 0, 0))));
        glm::vec3 up = glm::vec3(0, 1, 0);
        cam_target -= right * static_cast<float>(dx) * 0.01f;
        cam_target += up * static_cast<float>(dy) * 0.01f; // vertical pan keeps drag direction correct
    }

    last_x = xpos;
    last_y = ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    cam_distance *= (1.0f - static_cast<float>(yoffset) * 0.1f);
    cam_distance = glm::max(cam_distance, 0.1f);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ---------------------- Cube Init ----------------------
void init_cube()
{
    float vertices[] = {
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f};
    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0};
    cube.index_count = 36;

    glGenVertexArrays(1, &cube.vao);
    glGenBuffers(1, &cube.vbo);
    glGenBuffers(1, &cube.ebo);

    glBindVertexArray(cube.vao);
    glBindBuffer(GL_ARRAY_BUFFER, cube.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindVertexArray(0);
}

// ---------------------- Main ----------------------
int main()
{
    try
    {
        if (!glfwInit())
            return -1;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1280, 720, "Scene Viewer", nullptr, nullptr);
        if (!window)
            return -1;
        glfwMakeContextCurrent(window);

        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetCursorPosCallback(window, cursor_position_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            return -1;

        glEnable(GL_DEPTH_TEST);
        glfwSwapInterval(1);

        GLuint shader = create_shader_program();
        init_cube();

        tp::ThreadPool pool;

        std::shared_ptr<loader::IPFManager> ipf_manager_ptr;
        pool.submit([&]()
                    {
            try {
                ipf_manager_ptr = std::make_shared<loader::IPFManager>(
                    R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)", pool);

                auto data = ipf_manager_ptr->extract("ies_client/xac.ies");
                auto ies_root = ies::IESRoot::from_bytes(data);
                auto mesh_map = ies_root.extract_mesh_path_map();

                loader::SceneLoader loader(ipf_manager_ptr, mesh_map);
                std::string path = "bg/hi_entity/barrack.3dworld";
                auto data3d = ipf_manager_ptr->extract(path);
                auto scenes = loader.load(path, data3d);

                std::lock_guard<std::mutex> lock(g_scene_mutex);
                g_loaded_scenes = std::move(scenes);
                g_ready_to_render = true;
            } catch(const std::exception &e) {
                std::cerr << "Load error: " << e.what() << "\n";
            } });

        std::vector<MeshGPU> gpu_meshes;
        float cube_angle = 0.0f;

        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(shader);
            GLint loc = glGetUniformLocation(shader, "uMVP");

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                              float(width) / float(height), 0.1f, 1000.0f);

            if (!g_ready_to_render)
            {
                // Placeholder cube
                cube_angle += 0.01f;

                glm::vec3 cam_pos = cam_target + glm::vec3(
                                                     cos(cam_yaw) * cos(cam_pitch),
                                                     sin(cam_pitch),
                                                     sin(cam_yaw) * cos(cam_pitch)) *
                                                     cam_distance;

                glm::mat4 view = glm::lookAt(cam_pos, cam_target, glm::vec3(0, 1, 0));
                glm::mat4 model = glm::rotate(glm::mat4(1.0f), cube_angle, glm::vec3(0, 1, 0));
                glm::mat4 mvp = proj * view * model;

                glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);
                glBindVertexArray(cube.vao);
                glDrawElements(GL_TRIANGLES, cube.index_count, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
            else
            {
                std::lock_guard<std::mutex> lock(g_scene_mutex);

                if (gpu_meshes.empty())
                {
                    glm::vec3 min(FLT_MAX), max(-FLT_MAX);
                    for (auto &scene : g_loaded_scenes)
                        for (auto &node : scene.root_nodes)
                            if (node.model)
                                for (auto &sub : node.model->submeshes)
                                    for (auto &v : sub.positions)
                                    {
                                        glm::vec3 p(v.x, v.y, v.z);
                                        if (node.scale)
                                            p *= glm::vec3(node.scale->x, node.scale->y, node.scale->z);
                                        if (node.position)
                                            p += glm::vec3(node.position->x, node.position->y, node.position->z);
                                        min = glm::min(min, p);
                                        max = glm::max(max, p);
                                    }
                    scene_center = (min + max) * 0.5f;
                    scene_radius = glm::length(max - scene_center);
                    cam_distance = scene_radius * 2.0f;

                    for (auto &scene : g_loaded_scenes)
                        for (auto &node : scene.root_nodes)
                            if (node.model)
                                for (auto &sub : node.model->submeshes)
                                    gpu_meshes.push_back(upload_mesh_to_gpu(sub));
                }

                glm::vec3 cam_pos = cam_target + glm::vec3(
                                                     cos(cam_yaw) * cos(cam_pitch),
                                                     sin(cam_pitch),
                                                     sin(cam_yaw) * cos(cam_pitch)) *
                                                     cam_distance;

                glm::mat4 view = glm::lookAt(cam_pos, cam_target, glm::vec3(0, 1, 0));
                glm::mat4 vp = proj * view;

                size_t idx = 0;
                for (auto &scene : g_loaded_scenes)
                    for (auto &node : scene.root_nodes)
                    {
                        glm::mat4 model(1.0f);
                        if (node.position)
                            model = glm::translate(model, glm::vec3(node.position->x, node.position->y, node.position->z));
                        if (node.rotation)
                            model *= glm::mat4(glm::quat(node.rotation->w, node.rotation->x, node.rotation->y, node.rotation->z));
                        if (node.scale)
                            model = glm::scale(model, glm::vec3(node.scale->x, node.scale->y, node.scale->z));

                        glm::mat4 mvp = vp * model;
                        glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);

                        if (node.model)
                            for (auto &sub : node.model->submeshes)
                            {
                                auto &gpu = gpu_meshes[idx++];
                                glBindVertexArray(gpu.vao);
                                glDrawElements(GL_TRIANGLES, gpu.index_count, GL_UNSIGNED_INT, 0);
                            }
                    }
            }

            glfwSwapBuffers(window);
        }

        pool.wait_all();
        glfwTerminate();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
    }
}