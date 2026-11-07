# Project configuration.
cmake_minimum_required(VERSION 3.27)
project(gpgpu_physics VERSION 1.0.0 LANGUAGES C CXX)

include(infra/ProjectOptions.cmake)
include(infra/CompilerFlags.cmake)
include(infra/SourceFingerprint.cmake)
include(infra/Dependencies.cmake)
include(infra/Backends.cmake)
include(infra/Scenarios.cmake)

# [COMMENTED FOR COLLISION FOCUS] MMO Networking & Gameplay
# if(BUILD_MMO)
# 	include(infra/Networking.cmake)
# endif()

if(BUILD_VIEWER)
	include(infra/Viewer.cmake)
endif()

include(infra/Executables.cmake)
include(infra/Tests.cmake)
