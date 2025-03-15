cmake_minimum_required(VERSION 3.16)
project(RootWaveformAnalysis LANGUAGES CXX)

# Set C++ standard and compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3)

# Ensure CMake modules are available
include(FetchContent)

# --- Find ROOT ---
find_package(ROOT REQUIRED COMPONENTS RIO Hist Tree ROOTVecOps MathMore)

# Define ROOT include directories
include("${ROOT_USE_FILE}")

# --- Dependency Management: nlohmann JSON ---
find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
  message(STATUS "Nlohmann JSON not found; fetching via FetchContent.")
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# --- Project Source Files ---
add_executable(RootWaveformAnalysis main.cpp src/ConfigManager.cpp src/BranchManager.cpp 
  src/ReferenceManager.cpp src/FileManager.cpp src/GlobalManager.cpp)

# --- Include Directories ---
target_include_directories(RootWaveformAnalysis PRIVATE ${ROOT_INCLUDE_DIRS} include)

# --- Link Libraries ---
target_link_libraries(RootWaveformAnalysis PRIVATE ${ROOT_LIBRARIES} nlohmann_json::nlohmann_json)