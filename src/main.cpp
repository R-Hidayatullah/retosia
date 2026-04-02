// ============================================================
//  main.cpp  –  IPF loader + OpenGL 3D cube, C++20
// ============================================================
//  Architecture:
//    • ThreadPool with (hw_concurrency - 1) workers → IPF parsing
//    • Main thread                                  → OpenGL render loop
//
//  The render window opens immediately; IPF parsing runs in the
//  background.  The title bar shows a live status message.
//
//  Build (MSVC example):
//    cl /std:c++20 /EHsc main.cpp glad.c -I. -Iinclude
//       /link glfw3.lib opengl32.lib zlib.lib
//
//  Build (GCC/Clang example):
//    g++ -std=c++20 -O2 main.cpp glad.c -o app
//        -lglfw -lGL -lz -Iinclude
// ============================================================

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>

#include "thread_pool.hpp"   // C++20 thread pool  (single header)
#include "ipf.hpp"           // IPF parser         (uses thread_pool.hpp)
#include "cube_renderer.hpp" // OpenGL 3D cube     (GLAD + GLFW + GLM)

int main()
{
    // ── 1. Build the thread pool ──────────────────────────────────────────
    //  Reserve 1 core for the OpenGL render thread (main thread).
    //  Minimum of 1 worker so the app still works on single-core machines.
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const unsigned ipf_workers = std::max(1u, hw - 1u);

    tp::ThreadPool pool(ipf_workers);

    std::cout << "[Init] Hardware threads : " << hw << '\n'
              << "[Init] IPF pool workers : " << ipf_workers << '\n'
              << "[Init] Render thread    : main (1)\n\n";

    // ── 2. Submit IPF parsing to the pool (non-blocking) ─────────────────
    //  parse_game_ipfs_latest_streamed() uses the pool internally so every
    //  IPF file gets its own task rather than wasting a single future on
    //  the whole directory scan.
    const std::filesystem::path game_root =
        R"(C:\Program Files (x86)\Steam\steamapps\common\TreeOfSavior)";

    // We wrap the whole call in one pool task so main() stays free for GL.
    // Internally ipf.hpp submits per-file tasks back into the same pool.
    auto ipf_future = pool.submit(
        [&game_root, &pool]() -> std::unordered_map<std::string, ipf::IPFFileTable>
        {
            return ipf::parse_game_ipfs_latest_streamed(game_root, pool);
        });

    std::cout << "[IPF] Async parse started – main thread free for rendering\n\n";

    // ── 3. Open the OpenGL window on the MAIN thread ──────────────────────
    //  OpenGL contexts are single-thread; always create + use on main.
    renderer::CubeRenderer cube(1024, 768,
                                "Loading IPF assets… | Left-drag=Orbit  Scroll=Zoom  ESC=Quit");

    // ── 4. Main loop – render + check IPF status each frame ──────────────
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();

    std::unordered_map<std::string, ipf::IPFFileTable> latest_files;
    bool ipf_done = false;
    bool file_found = false;

    while (!cube.should_close())
    {
        cube.tick(); // poll events + render + swap

        // Non-blocking check: is the IPF future ready?
        if (!ipf_done)
        {
            auto status = ipf_future.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready)
            {
                latest_files = ipf_future.get();
                ipf_done = true;

                auto elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
                std::cout << "[IPF] Indexed " << latest_files.size()
                          << " files in " << elapsed << "s\n";

                // ── 5. Example: extract a specific asset ──────────────
                const std::string target = "ies_client/xac.ies";
                auto it = latest_files.find(target);
                if (it != latest_files.end())
                {
                    file_found = true;
                    auto t_ex0 = Clock::now();
                    auto data = it->second.extract_data();
                    auto t_ex1 = Clock::now();

                    std::cout << "[IPF] Extracted '" << target << "'\n"
                              << "       size    = " << data.size() << " bytes\n"
                              << "       time    = "
                              << std::chrono::duration<double>(t_ex1 - t_ex0).count()
                              << "s\n"
                              << "       archive = " << it->second.container_name << '\n';
                }
                else
                {
                    std::cout << "[IPF] '" << target << "' not found in archives\n";
                }

                // Update window title with final status
                std::string status_str =
                    "[Done] " + std::to_string(latest_files.size()) + " IPF files | " + (file_found ? "Asset extracted OK" : "Asset not found") + " | Left-drag=Orbit  Scroll=Zoom  ESC=Quit";
                cube.set_status(status_str);
            }
            else
            {
                // Update title with a live elapsed counter while loading
                auto secs = std::chrono::duration<double>(Clock::now() - t0).count();
                std::string loading = "[Loading IPF... " + std::to_string(static_cast<int>(secs)) + "s] " + std::to_string(pool.pending_count()) + " tasks pending" + " | Left-drag=Orbit  Scroll=Zoom  ESC=Quit";
                cube.set_status(loading);
            }
        }
    }

    // ── 6. Graceful shutdown ──────────────────────────────────────────────
    //  Pool destructor calls shutdown() automatically, but we can be
    //  explicit: make sure any in-flight IPF tasks finish before exiting.
    std::cout << "\n[Shutdown] Waiting for pool tasks...\n";
    pool.shutdown();
    std::cout << "[Shutdown] Done.\n";

    return 0;
}