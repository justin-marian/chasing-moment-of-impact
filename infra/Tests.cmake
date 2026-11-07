include(CTest)

if(BUILD_TESTING)
	# Unit test binaries.
	if(NOT REFERENCE_DOUBLE)
		add_executable(impact_tests tests/unit/impact_test.cpp src/client/render_gl/impact_history.cpp src/physics/scenarios/impact_demo.cpp)
		target_link_libraries(impact_tests PRIVATE backend_cpu backend_cuda io_config)
		strict(impact_tests)
		add_test(NAME collision_impact_visual_telemetry COMMAND impact_tests)
	endif()

	# [COMMENTED FOR COLLISION FOCUS] Gameplay tests
	# if(BUILD_MMO)
	# 	add_executable(gameplay_tests tests/unit/gameplay_test.cpp)
	# 	target_link_libraries(gameplay_tests PRIVATE gameplay_model client_replication)
	# 	strict(gameplay_tests)
	# 	add_test(NAME gameplay_controller_and_interpolation COMMAND gameplay_tests)
	# endif()

	if(BUILD_VIEWER)
		add_executable(slider_tests tests/unit/slider_test.cpp)
		target_link_libraries(slider_tests PRIVATE render_gl)
		strict(slider_tests)
		add_test(NAME slider_native_interaction COMMAND slider_tests)

		# [COMMENTED FOR COLLISION FOCUS] Animation tests
		# if(BUILD_MMO)
		# 	add_executable(animation_tests tests/unit/animation_test.cpp)
		# 	target_link_libraries(animation_tests PRIVATE animation)
		# 	strict(animation_tests)
		# 	add_test(NAME gltf_animation_and_skinning COMMAND animation_tests ${CMAKE_SOURCE_DIR}/assets/models/avatars)
		# endif()
	endif()

	add_executable(physics_tests tests/unit/test_main.cpp)
	target_link_libraries(physics_tests PRIVATE backend_cpu backend_cuda io_config)
	target_compile_definitions(physics_tests PRIVATE PROJECT_DIR="${CMAKE_SOURCE_DIR}")
	strict(physics_tests)

	# Registered test runs against physics_tests.
	add_test(NAME core_course_invariants COMMAND physics_tests)

	if(ENABLE_CUDA)
		add_test(NAME cuda_parity_and_determinism COMMAND physics_tests --cuda)
		set_tests_properties(cuda_parity_and_determinism PROPERTIES LABELS "gpu")
	endif()

	add_test(NAME long_running_invariants COMMAND physics_tests --long)
	set_tests_properties(long_running_invariants PROPERTIES LABELS "long" TIMEOUT 900)

	# Python-driven contract and integration tests.
	find_package(Python3 COMPONENTS Interpreter)
	
	# [COMMENTED FOR COLLISION FOCUS] MMO Python Integration Tests
	# if(BUILD_MMO AND Python3_FOUND)
	# 	if(BUILD_VIEWER)
	# 		add_test(NAME relocated_viewer_launch
	# 				 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/integration/test_relocated_viewer.py $<TARGET_FILE:physics_viewer>)
	# 		set_tests_properties(relocated_viewer_launch PROPERTIES TIMEOUT 90)
	# 	endif()
	#
	# 	add_test(NAME architecture_contract
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/scripts/test_architecture_contract.py ${CMAKE_SOURCE_DIR})
	# 	add_test(NAME file_organization_contract
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/scripts/test_file_organization.py ${CMAKE_SOURCE_DIR})
	# 	add_test(NAME source_readability_contract
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/scripts/test_source_readability.py ${CMAKE_SOURCE_DIR})
	#
	# 	add_test(NAME network_integration
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/networking/test_network.py $<TARGET_FILE:physics_server>)
	# 	set_tests_properties(network_integration PROPERTIES TIMEOUT 30)
	#
	# 	add_test(NAME multiplayer_profiles_and_combat
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/networking/test_multiplayer_profiles.py $<TARGET_FILE:physics_server>)
	# 	set_tests_properties(multiplayer_profiles_and_combat PROPERTIES TIMEOUT 40)
	#
	# 	add_test(NAME extracted_server_services
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/networking/test_cache_services.py $<TARGET_FILE:physics_server>)
	# 	set_tests_properties(extracted_server_services PROPERTIES TIMEOUT 30)
	#
	# 	add_test(NAME environment_asset_contract
	# 			 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/integration/test_environment_assets.py ${CMAKE_SOURCE_DIR})
	# endif()
endif()
