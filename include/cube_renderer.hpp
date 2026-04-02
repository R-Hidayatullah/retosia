#pragma once
#ifndef CUBE_RENDERER_HPP
#define CUBE_RENDERER_HPP

// ============================================================
//  cube_renderer.hpp  –  OpenGL 3.3 core interactive 3D cube
// ============================================================
//  Link with: glad, glfw3, glm (header-only)
//  Requires: C++20
//
//  Controls:
//    Left-drag   → orbit (arcball camera)
//    Scroll      → zoom in / out
//    ESC / Q     → close window
// ============================================================

#include <glad/glad.h> // must precede GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace renderer
{

    // ============================================================
    //  Shader
    // ============================================================
    class Shader
    {
    public:
        Shader(const char *vert_src, const char *frag_src)
        {
            GLuint vert = compile(GL_VERTEX_SHADER, vert_src);
            GLuint frag = compile(GL_FRAGMENT_SHADER, frag_src);
            id_ = glCreateProgram();
            glAttachShader(id_, vert);
            glAttachShader(id_, frag);
            glLinkProgram(id_);
            check_link(id_);
            glDeleteShader(vert);
            glDeleteShader(frag);
        }

        ~Shader()
        {
            if (id_)
                glDeleteProgram(id_);
        }

        Shader(const Shader &) = delete;
        Shader &operator=(const Shader &) = delete;
        Shader(Shader &&o) noexcept : id_(std::exchange(o.id_, 0)) {}

        void use() const noexcept { glUseProgram(id_); }

        void set_mat4(const char *n, const glm::mat4 &m) const
        {
            glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m));
        }

        void set_mat3(const char *n, const glm::mat3 &m) const
        {
            glUniformMatrix3fv(loc(n), 1, GL_FALSE, glm::value_ptr(m));
        }

        void set_vec3(const char *n, const glm::vec3 &v) const
        {
            glUniform3fv(loc(n), 1, glm::value_ptr(v));
        }

        void set_float(const char *n, float f) const
        {
            glUniform1f(loc(n), f);
        }

    private:
        GLuint id_{};

        [[nodiscard]] GLint loc(const char *name) const noexcept
        {
            return glGetUniformLocation(id_, name);
        }

        static GLuint compile(GLenum type, const char *src)
        {
            GLuint sh = glCreateShader(type);
            glShaderSource(sh, 1, &src, nullptr);
            glCompileShader(sh);
            GLint ok;
            glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char log[1024];
                glGetShaderInfoLog(sh, 1024, nullptr, log);
                glDeleteShader(sh);
                throw std::runtime_error(std::string("[Shader] compile:\n") + log);
            }
            return sh;
        }

        static void check_link(GLuint prog)
        {
            GLint ok;
            glGetProgramiv(prog, GL_LINK_STATUS, &ok);
            if (!ok)
            {
                char log[1024];
                glGetProgramInfoLog(prog, 1024, nullptr, log);
                throw std::runtime_error(std::string("[Shader] link:\n") + log);
            }
        }
    };

    // ============================================================
    //  CubeMesh  –  colour-per-face with per-face normals
    // ============================================================
    class CubeMesh
    {
    public:
        CubeMesh()
        {
            // Vertex layout: position(3) | normal(3) | colour(3) = 9 floats
            // 24 vertices (4 unique per face) so normals and colours stay flat.
            // clang-format off
        static constexpr float V[] = {
            // Front  (+Z) — red
            -0.5f,-0.5f, 0.5f,  0,0, 1,  0.93f,0.27f,0.27f,
             0.5f,-0.5f, 0.5f,  0,0, 1,  0.93f,0.27f,0.27f,
             0.5f, 0.5f, 0.5f,  0,0, 1,  0.93f,0.27f,0.27f,
            -0.5f, 0.5f, 0.5f,  0,0, 1,  0.93f,0.27f,0.27f,
            // Back   (-Z) — teal
             0.5f,-0.5f,-0.5f,  0,0,-1,  0.27f,0.85f,0.85f,
            -0.5f,-0.5f,-0.5f,  0,0,-1,  0.27f,0.85f,0.85f,
            -0.5f, 0.5f,-0.5f,  0,0,-1,  0.27f,0.85f,0.85f,
             0.5f, 0.5f,-0.5f,  0,0,-1,  0.27f,0.85f,0.85f,
            // Left   (-X) — lime
            -0.5f,-0.5f,-0.5f, -1,0,0,  0.40f,0.87f,0.37f,
            -0.5f,-0.5f, 0.5f, -1,0,0,  0.40f,0.87f,0.37f,
            -0.5f, 0.5f, 0.5f, -1,0,0,  0.40f,0.87f,0.37f,
            -0.5f, 0.5f,-0.5f, -1,0,0,  0.40f,0.87f,0.37f,
            // Right  (+X) — violet
             0.5f,-0.5f, 0.5f,  1,0,0,  0.78f,0.40f,0.96f,
             0.5f,-0.5f,-0.5f,  1,0,0,  0.78f,0.40f,0.96f,
             0.5f, 0.5f,-0.5f,  1,0,0,  0.78f,0.40f,0.96f,
             0.5f, 0.5f, 0.5f,  1,0,0,  0.78f,0.40f,0.96f,
            // Top    (+Y) — amber
            -0.5f, 0.5f, 0.5f,  0,1,0,  0.98f,0.82f,0.26f,
             0.5f, 0.5f, 0.5f,  0,1,0,  0.98f,0.82f,0.26f,
             0.5f, 0.5f,-0.5f,  0,1,0,  0.98f,0.82f,0.26f,
            -0.5f, 0.5f,-0.5f,  0,1,0,  0.98f,0.82f,0.26f,
            // Bottom (-Y) — sky-blue
            -0.5f,-0.5f,-0.5f,  0,-1,0, 0.27f,0.60f,0.93f,
             0.5f,-0.5f,-0.5f,  0,-1,0, 0.27f,0.60f,0.93f,
             0.5f,-0.5f, 0.5f,  0,-1,0, 0.27f,0.60f,0.93f,
            -0.5f,-0.5f, 0.5f,  0,-1,0, 0.27f,0.60f,0.93f,
        };

        static constexpr unsigned int I[] = {
             0, 1, 2,  2, 3, 0,   // front
             4, 5, 6,  6, 7, 4,   // back
             8, 9,10, 10,11, 8,   // left
            12,13,14, 14,15,12,   // right
            16,17,18, 18,19,16,   // top
            20,21,22, 22,23,20,   // bottom
        };
            // clang-format on

            index_count_ = static_cast<GLsizei>(std::size(I));

            glGenVertexArrays(1, &vao_);
            glGenBuffers(1, &vbo_);
            glGenBuffers(1, &ebo_);

            glBindVertexArray(vao_);

            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(V), V, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(I), I, GL_STATIC_DRAW);

            constexpr GLsizei stride = 9 * sizeof(float);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<const void *>(0));
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<const void *>(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                                  reinterpret_cast<const void *>(6 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }

        ~CubeMesh()
        {
            glDeleteVertexArrays(1, &vao_);
            glDeleteBuffers(1, &vbo_);
            glDeleteBuffers(1, &ebo_);
        }

        CubeMesh(const CubeMesh &) = delete;
        CubeMesh &operator=(const CubeMesh &) = delete;

        void draw() const
        {
            glBindVertexArray(vao_);
            glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

    private:
        GLuint vao_{}, vbo_{}, ebo_{};
        GLsizei index_count_{};
    };

    // ============================================================
    //  Camera  –  spherical arcball orbit around world origin
    // ============================================================
    class Camera
    {
    public:
        float yaw = -35.0f;  // horizontal angle (degrees)
        float pitch = 25.0f; // vertical angle   (degrees)
        float distance = 3.5f;
        glm::vec3 target{0.0f};

        /// Accumulate pixel-space drag
        void orbit(float screen_dx, float screen_dy) noexcept
        {
            yaw += screen_dx * 0.30f;
            pitch += screen_dy * 0.30f;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
        }

        /// Positive delta = zoom in
        void zoom(float scroll_delta) noexcept
        {
            distance -= scroll_delta * 0.30f;
            distance = glm::clamp(distance, 0.5f, 40.0f);
        }

        [[nodiscard]] glm::vec3 position() const noexcept
        {
            const float r = distance;
            const float p = glm::radians(pitch);
            const float y = glm::radians(yaw);
            return target + glm::vec3(r * std::cos(p) * std::cos(y),
                                      r * std::sin(p),
                                      r * std::cos(p) * std::sin(y));
        }

        [[nodiscard]] glm::mat4 view_matrix() const noexcept
        {
            return glm::lookAt(position(), target, {0.f, 1.f, 0.f});
        }

        [[nodiscard]] glm::mat4 proj_matrix(float aspect) const noexcept
        {
            return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 200.0f);
        }
    };

    // ============================================================
    //  GLSL source strings (OpenGL 3.3 core, embedded)
    // ============================================================
    namespace glsl
    {
        inline constexpr const char *VERT = R"glsl(
#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_color;

out vec3 v_world_pos;
out vec3 v_normal;
out vec3 v_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normal_mat;   // transpose(inverse(model))

void main()
{
    vec4 wp     = u_model * vec4(a_pos, 1.0);
    v_world_pos = wp.xyz;
    v_normal    = normalize(u_normal_mat * a_normal);
    v_color     = a_color;
    gl_Position = u_proj * u_view * wp;
}
)glsl";

        // Blinn-Phong with key light + fill light + rim darkening
        inline constexpr const char *FRAG = R"glsl(
#version 330 core

in  vec3 v_world_pos;
in  vec3 v_normal;
in  vec3 v_color;
out vec4 frag_color;

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;    // key light position (world space)
uniform vec3 u_light_color;
uniform vec3 u_fill_dir;     // fill light direction (toward light, normalised)

void main()
{
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_light_pos - v_world_pos); // key light dir
    vec3 V = normalize(u_cam_pos   - v_world_pos); // view dir
    vec3 H = normalize(L + V);                     // half-vector

    float ambient  = 0.12;
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), 80.0) * 0.55;
    float fill     = max(dot(N, u_fill_dir), 0.0)   * 0.22;
    float rim      = pow(1.0 - max(dot(N, V), 0.0), 3.0) * 0.18;

    vec3 col = v_color * u_light_color * (ambient + diffuse + fill)
             + u_light_color * specular
             - vec3(rim);

    // Gamma correction (sRGB approximation)
    col = pow(clamp(col, 0.0, 1.0), vec3(1.0 / 2.2));
    frag_color = vec4(col, 1.0);
}
)glsl";
    } // namespace glsl

    // ============================================================
    //  CubeRenderer  –  owns window, context, shaders, mesh
    // ============================================================
    class CubeRenderer
    {
    public:
        explicit CubeRenderer(int width = 1024,
                              int height = 768,
                              const char *title = "3D Cube – OpenGL 3.3")
            : width_(width), height_(height)
        {
            init_glfw(title);
            init_glad();

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_MULTISAMPLE); // 4× MSAA
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            shader_ = std::make_unique<Shader>(glsl::VERT, glsl::FRAG);
            mesh_ = std::make_unique<CubeMesh>();
        }

        ~CubeRenderer()
        {
            mesh_.reset();
            shader_.reset();
            if (window_)
                glfwDestroyWindow(window_);
            glfwTerminate();
        }

        // Non-copyable, non-movable (GLFW callbacks hold raw this-pointer)
        CubeRenderer(const CubeRenderer &) = delete;
        CubeRenderer &operator=(const CubeRenderer &) = delete;

        // ── Blocking loop ────────────────────────────────────────────
        void run()
        {
            while (!glfwWindowShouldClose(window_))
            {
                glfwPollEvents();
                render_frame();
                glfwSwapBuffers(window_);
            }
        }

        // ── Single-tick API (integrate into an outer loop) ───────────
        [[nodiscard]] bool should_close() const noexcept
        {
            return static_cast<bool>(glfwWindowShouldClose(window_));
        }

        void tick()
        {
            glfwPollEvents();
            render_frame();
            glfwSwapBuffers(window_);
        }

        // ── Title-bar status overlay ─────────────────────────────────
        void set_status(const std::string &msg) noexcept
        {
            glfwSetWindowTitle(window_, msg.c_str());
        }

        [[nodiscard]] Camera &camera() noexcept { return camera_; }
        [[nodiscard]] const Camera &camera() const noexcept { return camera_; }

    private:
        // ── GLFW / context initialisation ────────────────────────────
        void init_glfw(const char *title)
        {
            if (!glfwInit())
                throw std::runtime_error("[GLFW] init failed");

            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

            window_ = glfwCreateWindow(width_, height_, title, nullptr, nullptr);
            if (!window_)
            {
                glfwTerminate();
                throw std::runtime_error("[GLFW] window failed");
            }

            glfwMakeContextCurrent(window_);
            glfwSetWindowUserPointer(window_, this);
            glfwSwapInterval(1); // vsync

            // ── Register callbacks ────────────────────────────────────

            glfwSetFramebufferSizeCallback(window_,
                                           [](GLFWwindow *w, int wd, int ht)
                                           {
                                               glViewport(0, 0, wd, ht);
                                               auto *s = ptr(w);
                                               s->width_ = wd;
                                               s->height_ = ht;
                                           });

            glfwSetMouseButtonCallback(window_,
                                       [](GLFWwindow *w, int btn, int action, int)
                                       {
                                           if (btn != GLFW_MOUSE_BUTTON_LEFT)
                                               return;
                                           auto *s = ptr(w);
                                           s->lmb_down_ = (action == GLFW_PRESS);
                                           if (s->lmb_down_)
                                           {
                                               double x, y;
                                               glfwGetCursorPos(w, &x, &y);
                                               s->last_mouse_ = {static_cast<float>(x), static_cast<float>(y)};
                                           }
                                       });

            glfwSetCursorPosCallback(window_,
                                     [](GLFWwindow *w, double xp, double yp)
                                     {
                                         auto *s = ptr(w);
                                         if (!s->lmb_down_)
                                             return;
                                         float dx = static_cast<float>(xp) - s->last_mouse_.x;
                                         float dy = static_cast<float>(yp) - s->last_mouse_.y;
                                         s->last_mouse_ = {static_cast<float>(xp), static_cast<float>(yp)};
                                         s->camera_.orbit(dx, dy);
                                     });

            glfwSetScrollCallback(window_,
                                  [](GLFWwindow *w, double, double yoff)
                                  { ptr(w)->camera_.zoom(static_cast<float>(yoff)); });

            glfwSetKeyCallback(window_,
                               [](GLFWwindow *w, int key, int, int action, int)
                               {
                                   if (action == GLFW_PRESS &&
                                       (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q))
                                       glfwSetWindowShouldClose(w, GLFW_TRUE);
                               });
        }

        void init_glad()
        {
            if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
                throw std::runtime_error("[GLAD] failed to load GL functions");
        }

        // ── Per-frame render ─────────────────────────────────────────
        void render_frame()
        {
            glClearColor(0.09f, 0.09f, 0.14f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const float aspect = (height_ > 0)
                                     ? static_cast<float>(width_) / static_cast<float>(height_)
                                     : 1.0f;

            // Idle rotation (paused while user drags)
            if (!lmb_down_)
                idle_angle_ += 0.25f; // deg/frame ≈ 15°/s @60fps

            const glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                                glm::radians(idle_angle_),
                                                glm::vec3(0.0f, 1.0f, 0.0f));
            const glm::mat3 nmat = glm::mat3(glm::transpose(glm::inverse(model)));

            shader_->use();
            shader_->set_mat4("u_model", model);
            shader_->set_mat4("u_view", camera_.view_matrix());
            shader_->set_mat4("u_proj", camera_.proj_matrix(aspect));
            shader_->set_mat3("u_normal_mat", nmat);
            shader_->set_vec3("u_cam_pos", camera_.position());
            shader_->set_vec3("u_light_pos", {4.0f, 6.0f, 4.0f});
            shader_->set_vec3("u_light_color", {1.0f, 0.97f, 0.92f});
            shader_->set_vec3("u_fill_dir", glm::normalize(glm::vec3(-1.f, -0.6f, -1.f)));

            mesh_->draw();
        }

        static CubeRenderer *ptr(GLFWwindow *w) noexcept
        {
            return static_cast<CubeRenderer *>(glfwGetWindowUserPointer(w));
        }

        // ── State ────────────────────────────────────────────────────
        GLFWwindow *window_{};
        int width_{}, height_{};

        std::unique_ptr<Shader> shader_;
        std::unique_ptr<CubeMesh> mesh_;
        Camera camera_{};

        bool lmb_down_{false};
        glm::vec2 last_mouse_{};
        float idle_angle_{0.0f};
    };

} // namespace renderer

#endif // CUBE_RENDERER_HPP