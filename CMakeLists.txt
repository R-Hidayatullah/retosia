cmake_minimum_required(VERSION 3.10.0)
project(retosia VERSION 0.1.0 LANGUAGES C CXX)

# =========================
# C++20 (modern global setting)
# =========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable static linking on GCC/Clang
if(MSVC)
    # On Windows MSVC, /MT links CRT statically
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    # On Linux/Unix, link libc++/libstdc++ statically
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()


include_directories(include)

# =========================
# Source files (recursive)
# =========================
file(GLOB_RECURSE RETOSIA_SOURCES
    src/*.cpp
)


# =========================
# Executable
# =========================
add_executable(retosia
    src/main.cpp
    ${RETOSIA_SOURCES}
)

# =========================
# C++20 (modern way)
# =========================
target_compile_features(retosia PRIVATE cxx_std_20)

# =========================
# External includes
# =========================
target_include_directories(retosia PRIVATE
    external/glm-1.0.3/glm
    external/glad33extcore/include
    external/glfw-3.4.bin.WIN64/include
    external/dr_libs-master
    external/miniaudio-0.11.25
    external/tinyxml2-11.0.0
    external/zlib-1.3.2
)

# =========================
# STB (build from source)
# =========================
add_library(stb
    external/stb-master/stb_vorbis.c
)

target_include_directories(stb PUBLIC
    external/stb-master
)

target_link_libraries(retosia PRIVATE stb)


# =========================
# GLAD (build from source)
# =========================
add_library(glad
    external/glad33extcore/src/glad.c
)

target_include_directories(glad PUBLIC
    external/glad33extcore/include
)

target_link_libraries(retosia PRIVATE glad)

# =========================
# TinyXML2 (build from source)
# =========================
add_library(tinyxml2
    external/tinyxml2-11.0.0/tinyxml2.cpp
)

target_include_directories(tinyxml2 PUBLIC
    external/tinyxml2-11.0.0
)

target_link_libraries(retosia PRIVATE tinyxml2)

# =========================
# GLFW (prebuilt)
# =========================
target_link_directories(retosia PRIVATE
    external/glfw-3.4.bin.WIN64/lib-mingw-w64
)

target_link_libraries(retosia PRIVATE
    glfw3
    opengl32
)

# ---- zlib ----
set(ZLIB_SRC
    external/zlib-1.3.2/adler32.c
    external/zlib-1.3.2/compress.c
    external/zlib-1.3.2/crc32.c
    external/zlib-1.3.2/deflate.c
    external/zlib-1.3.2/gzclose.c
    external/zlib-1.3.2/gzlib.c
    external/zlib-1.3.2/gzread.c
    external/zlib-1.3.2/gzwrite.c
    external/zlib-1.3.2/infback.c
    external/zlib-1.3.2/inffast.c
    external/zlib-1.3.2/inflate.c
    external/zlib-1.3.2/inftrees.c     
    external/zlib-1.3.2/trees.c
    external/zlib-1.3.2/uncompr.c
    external/zlib-1.3.2/zutil.c
)
add_library(zlib STATIC ${ZLIB_SRC})
target_include_directories(zlib PUBLIC external/zlib-1.3.2)

target_link_libraries(retosia PRIVATE
    zlib
)

# =========================
# FreeType (use its CMake)
# =========================
add_subdirectory(external/freetype-2.14.3)

target_include_directories(retosia PRIVATE
    external/freetype-2.14.3/include
)

target_link_libraries(retosia PRIVATE freetype)