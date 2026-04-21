cmake_minimum_required(VERSION 3.20)
project(DNA_Trading_System VERSION 2.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# TEACHING NOTE: vcpkg toolchain
#
# vcpkg is a C++ package manager. When you install packages
# with vcpkg (like protobuf, glfw3, etc.), it puts them in
# C:/vcpkg/installed/<triplet>/ with include/ and lib/ folders.
#
# CMake needs to know WHERE to find those packages. That's what
# the toolchain file does — it modifies CMAKE_PREFIX_PATH so
# find_package() searches vcpkg's install directory.
#
# This ONLY applies on Windows. On Linux, system packages are
# found differently (usually in /usr/lib and /usr/include).
# ============================================================
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

# ============================================================
# INCLUDE PATHS
# ============================================================
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)
include_directories(${PROJECT_SOURCE_DIR}/external/ibkr)
include_directories(${PROJECT_SOURCE_DIR}/external/ibkr/protobufUnix)

# ============================================================
# TEACHING NOTE: Platform-conditional compilation
#
# CMAKE has built-in variables for platform detection:
#   WIN32   → true on any Windows (including MinGW, MSVC)
#   UNIX    → true on Linux and macOS
#   MSVC    → true ONLY when using Microsoft's compiler (cl.exe)
#
# We use these to toggle platform-specific flags and libraries.
# A file compiled on Windows with MinGW will have WIN32=true
# but MSVC=false. That's fine — we only care about the OS.
# ============================================================

# ============================================================
# IBKR PLATFORM DEFINES
#
# TEACHING NOTE: The IBKR API has two code paths:
#   IB_WIN32 → uses Windows API (CRITICAL_SECTION, CreateEvent)
#   IB_POSIX → uses pthreads (pthread_mutex, pthread_cond)
#
# The IBKR source file platformspecific.h auto-sets these
# via #ifdef _WIN32 or #ifdef _POSIX_THREADS. However, with
# MinGW on Windows, _MSC_VER is NOT defined, so the auto-detection
# in StdAfx.h only activates for MSVC. We must define IB_WIN32
# ourselves for MinGW to use the Windows code path.
#
# add_compile_definitions() adds -DIB_WIN32 to all gcc commands.
# ============================================================
if(WIN32)
    add_compile_definitions(IB_WIN32)
else()
    add_compile_definitions(IB_POSIX)
endif()

# ============================================================
# SOURCE FILES
# ============================================================
# Core sources (everything except main files)
# This glob collects all .cpp files in src/core, src/adapters, src/strategy
file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
    "src/adapters/*.cpp"
    "src/strategy/*.cpp"
)

# External: simdjson (JSON parser)
# simdjson is compiled from external/simdjson.cpp
set(CORE_SOURCES ${CORE_SOURCES} ${PROJECT_SOURCE_DIR}/external/simdjson.cpp)

# IBKR API source files
# The IBKR C++ API is in external/ibkr/*.cpp
file(GLOB IBKR_SOURCES "${PROJECT_SOURCE_DIR}/external/ibkr/*.cpp")

# IBKR protobuf generated files
# These are pre-generated .cc files from IBKR's protobuf definitions
file(GLOB IBKR_PROTOBUF_SOURCES "${PROJECT_SOURCE_DIR}/external/ibkr/protobufUnix/*.cc")

# Combine all source files
set(ALL_SOURCES ${CORE_SOURCES} ${IBKR_SOURCES} ${IBKR_PROTOBUF_SOURCES})

# ============================================================
# FIND DEPENDENCIES
#
# TEACHING NOTE: find_package() searches for a package.
# When vcpkg toolchain is active (on Windows), it modifies
# CMAKE_PREFIX_PATH to include C:/vcpkg/installed/<triplet>/
# so CMake finds packages installed via vcpkg.
#
# On Linux, find_package() searches system paths like
# /usr/lib/cmake, /usr/local/lib/cmake, etc.
# ============================================================
# ============================================================
# PROTOBUF — Windows local build or Linux system package
#
# On Windows: we built protobuf from source and copied the
# .lib files + headers into external/protobuf/. We link them
# directly instead of relying on find_package().
#
# On Linux: protobuf is installed system-wide, find_package()
# finds it automatically.
# ============================================================
if(WIN32)
    set(PROTOBUF_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/external/protobuf/include")
    set(PROTOBUF_LIBRARY "${PROJECT_SOURCE_DIR}/external/protobuf/lib/libprotobuf.a")
    include_directories(${PROTOBUF_INCLUDE_DIR})
else()
    find_package(Protobuf REQUIRED)
    include_directories(${Protobuf_INCLUDE_DIRS})
    set(PROTOBUF_LIBRARY ${Protobuf_LIBRARIES})
endif()

# ============================================================
# PLATFORM-SPECIFIC LINK LIBRARIES
#
# TEACHING NOTE:
#   On Linux:
#     - pthread for POSIX threading (IB_POSIX code path uses pthreads)
#     - libbid.a is the Intel DFP library, ELF format (Linux)
#
#   On Windows (MinGW):
#     - ws2_32 (Winsock2) for TCP sockets — IBKR connects to
#       IB Gateway over TCP. Winsock is Windows-only.
#     - NO pthread needed — IBKR uses Windows API (IB_WIN32)
#     - libbid_win.a is the Intel DFP library, PE/COFF format
#       (compiled on Windows just now)
#
# NOTE: IBKR's code has:
#   #pragma comment(lib, "ws2_32.lib")
# That pragma only works with MSVC (cl.exe). With MinGW/GCC
# that pragma is ignored, so we must link it manually here.
# ============================================================
if(WIN32)
    set(PLATFORM_LIBS
        ws2_32                                                         # Winsock2 — TCP sockets for IBKR
        ${PROJECT_SOURCE_DIR}/external/libbid_win.a                   # Intel DFP — rebuilt for MinGW/Windows
    )
else()
    set(PLATFORM_LIBS
        pthread                                                        # POSIX threads
        ${PROJECT_SOURCE_DIR}/external/libbid.a                       # Intel DFP — Linux ELF build
    )
endif()

# ============================================================
# EXECUTABLE 1: Console app (V1 — headless debug/test tool)
#
# TEACHING NOTE:
# This builds dna_console from src/main_console.cpp.
# It uses the V1 architecture: text-based dashboard, IBKR connection,
# strategy engine. Useful for testing when you don't want GUI overhead.
# ============================================================
add_executable(dna_console
    src/main_console.cpp
    ${ALL_SOURCES}
)
target_link_libraries(dna_console
    ${PLATFORM_LIBS}
    ${PROTOBUF_LIBRARY}
)

# ============================================================
# EXECUTABLE 2: GUI app (V2 — the real platform)
#
# TEACHING NOTE:
# This is commented out because we haven't added GLFW/ImGui/ImPlot yet.
# Once vcpkg installs them and we create src/main.cpp (the GUI entry point),
# we uncomment this and add the UI source files (src/ui/*.cpp).
#
# For now, only dna_console will build. That's fine — it lets us test
# the Windows port with V1 before adding the GUI complexity.
# ============================================================
# TODO: Uncomment when ready for GUI (after adding GLFW + ImGui + ImPlot)
#
# find_package(glfw3 REQUIRED)
# find_package(OpenGL REQUIRED)
#
# add_executable(dna_trader
#     src/main.cpp
#     ${ALL_SOURCES}
#     # src/ui/*.cpp will go here
# )
# target_link_libraries(dna_trader
#     ${PLATFORM_LIBS}
#     ${PROTOBUF_LIBRARY}
#     glfw
#     OpenGL::GL
#     # imgui implot — added once vcpkg installs them
# )

# ============================================================
# TEST EXECUTABLE
# ============================================================
add_executable(test_strategy
    tests/test_strategy.cpp
    ${ALL_SOURCES}
)
target_link_libraries(test_strategy
    ${PLATFORM_LIBS}
    ${PROTOBUF_LIBRARY}
)