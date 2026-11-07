add_library(project_files src/shared/utils/files.cpp)
target_link_libraries(project_files PUBLIC sim_core)
strict(project_files)

add_library(backend_cpu
	src/physics/backend/cpu/step.cpp
	src/physics/backend/cpu/bounds.cpp
	src/physics/backend/cpu/broad_phase.cpp
	src/physics/backend/cpu/contacts.cpp
	src/physics/backend/cpu/diagnostics.cpp
	src/physics/backend/cpu/forces.cpp
	src/physics/backend/cpu/position.cpp
	src/physics/backend/cpu/sleep.cpp
	src/physics/backend/cpu/solver.cpp
	src/physics/backend/cpu/velocity.cpp
	src/shared/sim/state.cpp)

target_link_libraries(backend_cpu PUBLIC sim_core)
strict(backend_cpu)

# CUDA backend.
if(ENABLE_CUDA)
	enable_language(CUDA)
	find_package(CUDAToolkit 12.4 REQUIRED)

	set(CMAKE_CUDA_STANDARD 20)
	set(CMAKE_CUDA_STANDARD_REQUIRED ON)
	
	if(NOT CMAKE_CUDA_ARCHITECTURES)
		# Turing, Ampere consumer, and Ampere data-center.
		set(CMAKE_CUDA_ARCHITECTURES 75 86 89)
	endif()

	add_library(backend_cuda
		src/physics/backend/cuda/step.cu
		src/physics/backend/cuda/bounds.cu
		src/physics/backend/cuda/broad_phase.cu
		src/physics/backend/cuda/contacts.cu
		src/physics/backend/cuda/diagnostics.cu
		src/physics/backend/cuda/forces.cu
		src/physics/backend/cuda/position.cu
		src/physics/backend/cuda/sleep.cu
		src/physics/backend/cuda/solver.cu
		src/physics/backend/cuda/velocity.cu)

	target_link_libraries(backend_cuda PUBLIC sim_core CUDA::cudart)

	# --fmad=false keeps floating-point results reproducible across compilers,
	# matching the CPU build's -ffp-contract=off. The MSVC-only flags are
	# guarded by the generator expression so they do not reach nvcc on Linux.
	target_compile_options(backend_cuda PRIVATE
		$<$<COMPILE_LANGUAGE:CUDA>:
			--expt-relaxed-constexpr
			--fmad=false
			-Xcompiler=/W4
			-Xcompiler=/Zc:preprocessor
			-Xcompiler=/WX
			-Xcompiler=/wd4324
			-Xcompiler=/wd4211
			--diag-suppress=108,174,1835>)

	if(NOT MSVC)
		set_property(TARGET backend_cuda PROPERTY COMPILE_OPTIONS
			--expt-relaxed-constexpr --fmad=false)
	endif()

	target_compile_definitions(backend_cuda PUBLIC SIM_HAS_CUDA)
else()
	add_library(backend_cuda src/physics/backend/cuda/unavailable.cpp)
	target_link_libraries(backend_cuda PUBLIC sim_core)
endif()
