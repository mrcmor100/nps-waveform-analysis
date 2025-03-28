cmake_minimum_required(VERSION 3.16)
project(RootWaveformAnalysis LANGUAGES CXX)

# Set C++ standard and compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3)

# Ensure CMake modules are available
include(FetchContent)

# --- Find ROOT ---
find_package(ROOT REQUIRED COMPONENTS RIO Hist Tree ROOTVecOps ROOTDataFrame MathMore)

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

# Add the Dictionary containing the definition of DataBlockFloat for streaming.
set(DICT_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/DataBlockDict.cpp)

add_custom_command(
    OUTPUT ${DICT_OUTPUT}
    COMMAND rootcling -f ${DICT_OUTPUT} -c ${CMAKE_CURRENT_SOURCE_DIR}/include/DataTypes.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/DataTypes.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h
    COMMENT "Generating ROOT dictionary for DataBlock"
)

# Add the dictionary source file to a library target.
add_library(DataBlockDict SHARED ${DICT_OUTPUT})
target_include_directories(DataBlockDict PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(DataBlockDict ${ROOT_LIBRARIES})

# --- Project Source Files ---
add_executable(RootWaveformAnalysis main.cpp src/ConfigManager.cpp src/BranchManager.cpp 
  src/ReferenceManager.cpp src/FileManager.cpp src/GlobalManager.cpp src/DataAnalysisManager.cpp
  src/ApplicationManager.cpp src/WaveformFitter.cpp)

add_custom_command(
  TARGET RootWaveformAnalysis POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/RootWaveformAnalysis
  ${CMAKE_SOURCE_DIR}/RootWaveformAnalysis
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/libDataBlockDict.so
  ${CMAKE_SOURCE_DIR}/libDataBlockDict.so
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_BINARY_DIR}/DataBlockDict_rdict.pcm
  ${CMAKE_SOURCE_DIR}/DataBlockDict_rdict.pcm
  COMMENT "Move Executable and libDataBlockDict and .pcm file to SOURCE_DIR"
)
# --- Include Directories ---
target_include_directories(RootWaveformAnalysis PRIVATE ${ROOT_INCLUDE_DIRS} include)

# --- Link Libraries ---
target_link_libraries(RootWaveformAnalysis PRIVATE ${ROOT_LIBRARIES} DataBlockDict nlohmann_json::nlohmann_json)