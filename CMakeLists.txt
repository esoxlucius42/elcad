cmake_minimum_required(VERSION 3.20)
project(elcad VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

# ── OpenGL ───────────────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)

# ── OpenCASCADE ──────────────────────────────────────────────────────────────
# Resolution order:
#  1. Explicit override:  cmake -DOCCT_DIR=/path
#  2. Already built by a previous ExternalProject run (build/occt-install)
#  3. System install
#  4. Build from source via ExternalProject (first-time, slow)

set(ELCAD_HAVE_OCCT OFF)
set(OCCT_INSTALL_DIR ${CMAKE_BINARY_DIR}/occt-install)

# 1. Explicit override
if(DEFINED OCCT_DIR)
    find_package(OpenCASCADE REQUIRED PATHS "${OCCT_DIR}" NO_DEFAULT_PATH)
    set(ELCAD_HAVE_OCCT ON)
    message(STATUS "Using explicit OCCT_DIR: ${OCCT_DIR}")
endif()

# 2. Previously built by ExternalProject
if(NOT ELCAD_HAVE_OCCT AND EXISTS "${OCCT_INSTALL_DIR}/lib/cmake/opencascade/OpenCASCADEConfig.cmake")
    set(OpenCASCADE_DIR "${OCCT_INSTALL_DIR}/lib/cmake/opencascade" CACHE PATH "" FORCE)
    find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH)
    set(ELCAD_HAVE_OCCT ON)
    message(STATUS "Using local OCCT build: ${OCCT_INSTALL_DIR} (v${OpenCASCADE_VERSION})")
endif()

# 3. System install
if(NOT ELCAD_HAVE_OCCT)
    find_package(OpenCASCADE QUIET)
    if(OpenCASCADE_FOUND)
        set(ELCAD_HAVE_OCCT ON)
        message(STATUS "Found system OpenCASCADE: ${OpenCASCADE_VERSION}")
    endif()
endif()

# 4. Build from source
if(NOT ELCAD_HAVE_OCCT)
    message(STATUS "OpenCASCADE not found — building from source via ExternalProject (first time only)…")
    include(ExternalProject)

    ExternalProject_Add(occt_external
        GIT_REPOSITORY  https://github.com/Open-Cascade-SAS/OCCT.git
        GIT_TAG         V7_9_0
        GIT_SHALLOW     TRUE
        GIT_PROGRESS    TRUE
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${OCCT_INSTALL_DIR}
            -DCMAKE_BUILD_TYPE=Release
            -DBUILD_MODULE_Visualization=OFF
            -DBUILD_MODULE_Draw=OFF
            -DBUILD_MODULE_FoundationClasses=ON
            -DBUILD_MODULE_ModelingData=ON
            -DBUILD_MODULE_ModelingAlgorithms=ON
            -DBUILD_MODULE_ApplicationFramework=ON
            -DBUILD_MODULE_DataExchange=ON
            -DUSE_FREETYPE=OFF
            -DUSE_TBB=OFF
            -DUSE_OPENGL=OFF
            -DUSE_GLES2=OFF
            -DUSE_VTK=OFF
            -DUSE_RAPIDJSON=OFF
            -DUSE_DRACO=OFF
            -DBUILD_DOC_Overview=OFF
            -DBUILD_SAMPLES=OFF
            -DCMAKE_CXX_STANDARD=17
        BUILD_COMMAND   ${CMAKE_COMMAND} --build . --parallel 32
        INSTALL_COMMAND ${CMAKE_COMMAND} --install .
        LOG_DOWNLOAD  ON
        LOG_CONFIGURE ON
        LOG_BUILD     ON
        LOG_INSTALL   ON
    )

    # Stubs so elcad can declare the dependency — real targets appear after build
    set(ELCAD_OCCT_STUB_INCLUDES ${OCCT_INSTALL_DIR}/include/opencascade)
    file(MAKE_DIRECTORY ${ELCAD_OCCT_STUB_INCLUDES})

    set(ELCAD_OCCT_LIBS_LIST
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKMesh
        TKG3d TKGeomBase TKGeomAlgo TKBO TKBool TKShHealing
        TKFeat TKFillet TKOffset
        TKDESTEP TKXSBase TKXCAF TKLCAF TKDE TKDECascade
    )
    foreach(lib IN LISTS ELCAD_OCCT_LIBS_LIST)
        add_library(${lib} SHARED IMPORTED)
        set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION             ${OCCT_INSTALL_DIR}/lib/lib${lib}.so
            INTERFACE_INCLUDE_DIRECTORIES ${ELCAD_OCCT_STUB_INCLUDES}
        )
        add_dependencies(${lib} occt_external)
    endforeach()

    set(ELCAD_HAVE_OCCT ON)
    set(ELCAD_OCCT_FIRST_BUILD ON)
    message(STATUS "OCCT will be built on first 'cmake --build'. Re-run cmake after to pick up installed targets.")
endif()


# ── spdlog ───────────────────────────────────────────────────────────────────
include(FetchContent)
set(SPDLOG_BUILD_EXAMPLE  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(spdlog)

# ── GLM (header-only, optional for now) ─────────────────────────────────────
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp)
    if(NOT GLM_INCLUDE_DIR)
        set(GLM_INCLUDE_DIR "")   # Not required until we use GLM types
    endif()
endif()

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE ELCAD_SOURCES
    "src/*.cpp"
    "src/*.h"
)

qt6_add_resources(ELCAD_RESOURCES src/resources.qrc)

add_executable(elcad
    ${ELCAD_SOURCES}
    ${ELCAD_RESOURCES}
)

target_include_directories(elcad PRIVATE src)
if(GLM_INCLUDE_DIR)
    target_include_directories(elcad PRIVATE ${GLM_INCLUDE_DIR})
endif()

target_link_libraries(elcad PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    spdlog::spdlog_header_only
)

# Compile all log levels into the binary; runtime sink level controls output.
target_compile_definitions(elcad PRIVATE SPDLOG_ACTIVE_LEVEL=0)

# ── Link OCCT ─────────────────────────────────────────────────────────────────
if(ELCAD_HAVE_OCCT AND NOT ELCAD_OCCT_FIRST_BUILD)
    # find_package populated proper IMPORTED targets — link by name directly
    target_link_libraries(elcad PRIVATE
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKMesh
        TKG3d TKGeomBase TKGeomAlgo TKBO TKBool TKShHealing
        TKFeat TKFillet TKOffset
        TKDESTEP TKXSBase TKXCAF TKLCAF TKDE TKDECascade TKCDF TKCAF
    )
    target_compile_definitions(elcad PRIVATE ELCAD_HAVE_OCCT)
elseif(ELCAD_OCCT_FIRST_BUILD)
    # ExternalProject hasn't installed yet — stubs are set up, just declare the dep
    # After 'cmake --build' completes, re-run cmake to pick up real targets
    target_include_directories(elcad PRIVATE ${ELCAD_OCCT_STUB_INCLUDES})
    target_link_libraries(elcad PRIVATE
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKMesh
        TKG3d TKGeomBase TKGeomAlgo TKBO TKBool TKShHealing
        TKFeat TKFillet TKOffset
        TKDESTEP TKXSBase TKXCAF TKLCAF TKDE TKDECascade TKCDF TKCAF
    )
    target_compile_definitions(elcad PRIVATE ELCAD_HAVE_OCCT)
else()
    message(WARNING "Building without OpenCASCADE — geometry features disabled")
endif()

# ── Compiler warnings ─────────────────────────────────────────────────────────
target_compile_options(elcad PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS elcad DESTINATION bin)

# Simple command-line tool to measure tessellation triangle counts for a sphere
add_executable(sphere_tess_count tools/sphere_tess_count.cpp)
target_include_directories(sphere_tess_count PRIVATE src)

if(ELCAD_HAVE_OCCT AND NOT ELCAD_OCCT_FIRST_BUILD)
    target_link_libraries(sphere_tess_count PRIVATE
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKMesh
        TKG3d TKGeomBase TKGeomAlgo TKBO TKBool TKShHealing
        TKFeat TKFillet TKOffset
        TKDESTEP TKXSBase TKXCAF TKLCAF TKDE TKDECascade TKCDF TKCAF
    )
    target_compile_definitions(sphere_tess_count PRIVATE ELCAD_HAVE_OCCT)
elseif(ELCAD_OCCT_FIRST_BUILD)
    target_include_directories(sphere_tess_count PRIVATE ${ELCAD_OCCT_STUB_INCLUDES})
    target_link_libraries(sphere_tess_count PRIVATE
        TKernel TKMath TKBRep TKPrim TKTopAlgo TKMesh
        TKG3d TKGeomBase TKGeomAlgo TKBO TKBool TKShHealing
        TKFeat TKFillet TKOffset
        TKDESTEP TKXSBase TKXCAF TKLCAF TKDE TKDECascade TKCDF TKCAF
    )
    target_compile_definitions(sphere_tess_count PRIVATE ELCAD_HAVE_OCCT)
else()
    message(WARNING "sphere_tess_count requires OpenCASCADE — target will be empty")
endif()
