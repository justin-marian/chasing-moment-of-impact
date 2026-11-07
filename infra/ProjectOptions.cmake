# C++20 across every target, no GNU extensions, so the code stays portable
# between GCC, Clang, and MSVC.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build switches. Flip any of these with -D<name>=ON|OFF on the command line.
option(ENABLE_CUDA       "Build the real CUDA backend"                     ON)
# [COMMENTED FOR COLLISION FOCUS] 3D MMO and client/server
# option(BUILD_MMO         "Build the 3D MMO and its client/server"           OFF)
option(BUILD_VIEWER      "Build the OpenGL viewer"                         ON)
option(REFERENCE_DOUBLE  "Float64 CPU tolerance reference (CUDA is OFF)"   OFF)
option(ENABLE_SANITIZERS "Address/undefined sanitizers on GCC/Clang"       OFF)

# Every target lands in exports/builds/<platform>, so a build on any host
# produces the same directory structure. The -reference suffix separates the
# float64 tolerance build from the float32 default.
if(WIN32)
	set(outputPlatform "windows-x64")
else()
	set(outputPlatform "linux-${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(REFERENCE_DOUBLE)
	string(APPEND outputPlatform "-reference")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exports/builds/${outputPlatform}")

# Header-only simulation interface. Every target that touches the simulation
# links this. It contributes an include directory and, when the reference
# build is selected, the SIM_REFERENCE_DOUBLE define that switches Real from float to double.
add_library(sim_core INTERFACE)
target_include_directories(sim_core INTERFACE src)

if(REFERENCE_DOUBLE)
	target_compile_definitions(sim_core INTERFACE SIM_REFERENCE_DOUBLE)
	# The float64 reference build has no CUDA path, so disable it here rather
	# than making every caller of sim_core check REFERENCE_DOUBLE itself.
	set(ENABLE_CUDA OFF CACHE BOOL "Disabled for reference build" FORCE)
endif()
