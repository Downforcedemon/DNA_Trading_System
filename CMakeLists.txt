cmake_minimum_required(VERSION 3.20)
project(DNA_Trading_System VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Where to find header files
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/external)
include_directories(${PROJECT_SOURCE_DIR}/external/ibkr)

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add external libraries
set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/external/simdjson.cpp)

# Add IBKR API source files and protobuf
file(GLOB IBKR_SOURCES "${PROJECT_SOURCE_DIR}/external/ibkr/*.cpp")
file(GLOB IBKR_PROTOBUF_SOURCES "${PROJECT_SOURCE_DIR}/external/ibkr/protobufUnix/*.cc")
set(SOURCES ${SOURCES} ${IBKR_SOURCES} ${IBKR_PROTOBUF_SOURCES})

# Add protobuf include directory
include_directories(${PROJECT_SOURCE_DIR}/external/ibkr/protobufUnix)

# Create the executable with ALL sources
add_executable(dna_system ${SOURCES})

# Find and link protobuf
find_package(Protobuf REQUIRED)

# Link pthread, protobuf, and Intel DFP library (required by IBKR API)
target_link_libraries(dna_system pthread ${Protobuf_LIBRARIES} ${PROJECT_SOURCE_DIR}/external/libbid.a)

# --- Test executables ---
# Common sources for tests (everything except main.cpp)
set(TEST_SOURCES ${SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Test 1: Strategy pipeline smoke test
add_executable(test_strategy tests/test_strategy.cpp ${TEST_SOURCES})
target_link_libraries(test_strategy pthread ${Protobuf_LIBRARIES} ${PROJECT_SOURCE_DIR}/external/libbid.a)
